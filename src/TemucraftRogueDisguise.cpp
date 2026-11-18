/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * mod-temucraft-rogue-disguise: rogues can disguise themselves as other humanoids.
 *
 * - Any rogue trainer teaches the first disguise from level 2: a human peasant for the Horde, an orcish peon for the
 *   Alliance. The others are taught by disguise contacts out in the world, standing among the people they imitate
 *   (Defias, Stonesplinter trogg, Syndicate, Dalaran wizard, Southsea pirate, Dark Iron dwarf), for gold.
 * - Wearing one costs its materials (cloth) and changes your look. Creatures of that group, and for peasant / peon
 *   the people of that side, take you for one of their own: they don't attack, and you can walk among them, talk
 *   to them and trade with them.
 * - The disguise is fragile: it falls off when you deal or take damage, enter combat or cast a hostile spell (Pick
 *   Pocket excepted), die, or log out.
 * - `.disguise list | <name> | off`, or the Disguise button that the Temucraft interface shows rogues next to the
 *   minimap (addon messages, prefix TRD).
 * Text comes from module_string (module "mod-temucraft-rogue-disguise"), with English built in as a fallback.
 */
#include "Chat.h"
#include "ChatCommand.h"
#include "CommandScript.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "StringFormat.h"
#include "TemucraftAddonMessage.h"
#include "TemucraftModuleText.h"
#include "Tokenize.h"
#include "UnitScript.h"
#include "WorldScript.h"
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

using namespace Acore::ChatCommands;

namespace
{
    constexpr char const* MODULE = "mod-temucraft-rogue-disguise";
    constexpr char const* PREFIX = "TRD";
    constexpr uint32 SENDER_DISGUISE = 100;
    constexpr uint32 SPELL_PICK_POCKET = 921;
    enum Side : uint8 { SIDE_NONE = 0, SIDE_ALLIANCE = 1, SIDE_HORDE = 2 };

    enum Str : uint32
    {
        S_OFF = 1, S_NOT_ROGUE, S_LEARN_BASIC, S_LEARNED, S_LEVEL, S_MONEY, S_KNOWN_ALREADY, S_NOT_KNOWN, S_REAGENTS,
        S_COMBAT, S_DISGUISED, S_BROKEN, S_REMOVED, S_LIST_HEAD, S_LIST_LINE, S_USAGE, S_LEARN_HERE, S_COOLDOWN, S_NONE
    };

    struct Disguise
    {
        uint32 id = 0;
        std::string name;
        uint32 modelEntry = 0, display = 0, faction = 0;  // faction: Faction.dbc id of the model creature
        uint8 side = SIDE_NONE, level = 1;
        uint32 cost = 0, reagent = 0, reagentCount = 0, contact = 0;
        bool basic = false;
        std::vector<std::string> prefixes;
    };
    struct Settings { bool enable = true; uint32 cooldown = 10; bool fooledGuards = true; } cfg;

    std::map<uint32, Disguise> disguises;
    std::unordered_map<ObjectGuid::LowType, std::set<uint32>> known;      // by player
    std::unordered_map<ObjectGuid::LowType, uint32> worn;                  // player -> disguise
    std::unordered_map<ObjectGuid::LowType, uint32> brokenAt;              // player -> time the last one fell off

    template<typename T>
    char const* Text(T who, uint32 id, char const* fallback) { return TemucraftModuleText(MODULE, who, id, fallback); }
    uint32 Now() { return uint32(GameTime::GetGameTime().count()); }

    void Load()
    {
        disguises.clear();
        if (QueryResult r = WorldDatabase.Query("SELECT id, name, model_entry, side, required_level, cost, reagent, reagent_count, name_prefixes, basic, contact_entry FROM mod_temucraft_disguise"))
            do
            {
                Field* f = r->Fetch(); Disguise d;
                d.id = f[0].Get<uint32>(); d.name = f[1].Get<std::string>(); d.modelEntry = f[2].Get<uint32>(); d.side = f[3].Get<uint8>();
                d.level = f[4].Get<uint8>(); d.cost = f[5].Get<uint32>(); d.reagent = f[6].Get<uint32>(); d.reagentCount = f[7].Get<uint32>();
                std::string prefixes = f[8].Get<std::string>();
                for (std::string_view p : Acore::Tokenize(prefixes, ',', false)) d.prefixes.emplace_back(p);
                d.basic = f[9].Get<bool>(); d.contact = f[10].Get<uint32>();
                if (CreatureTemplate const* t = sObjectMgr->GetCreatureTemplate(d.modelEntry))
                {
                    if (CreatureModel const* m = t->GetFirstVisibleModel()) d.display = m->CreatureDisplayID;
                    if (FactionTemplateEntry const* ft = sFactionTemplateStore.LookupEntry(t->faction)) d.faction = ft->faction;
                }
                if (!d.display) { LOG_ERROR("module", "mod-temucraft-rogue-disguise: disguise {} has no model (creature {})", d.id, d.modelEntry); continue; }
                disguises[d.id] = d;
            } while (r->NextRow());
    }

    void LoadKnown(Player* player)
    {
        std::set<uint32>& k = known[player->GetGUID().GetCounter()];
        k.clear();
        if (QueryResult r = CharacterDatabase.Query("SELECT disguise FROM mod_temucraft_disguise_known WHERE owner = {}", player->GetGUID().GetCounter()))
            do k.insert(r->Fetch()[0].Get<uint32>()); while (r->NextRow());
    }
    bool Knows(Player* player, uint32 id) { auto it = known.find(player->GetGUID().GetCounter()); return it != known.end() && it->second.count(id); }

    Disguise const* Find(std::string const& name)
    {
        std::string want = name; std::transform(want.begin(), want.end(), want.begin(), ::tolower);
        for (auto const& [id, d] : disguises)
        {
            std::string have = d.name; std::transform(have.begin(), have.end(), have.begin(), ::tolower);
            if (have == want || have.find(want) != std::string::npos) return &d;
        }
        if (Optional<uint32> id = Acore::StringTo<uint32>(name)) if (disguises.count(*id)) return &disguises[*id];
        return nullptr;
    }

    Disguise const* BasicFor(Player* player)
    {
        uint8 wanted = player->GetTeamId() == TEAM_ALLIANCE ? SIDE_HORDE : SIDE_ALLIANCE;   // you dress as the other side
        for (auto const& [id, d] : disguises) if (d.basic && d.side == wanted) return &d;
        return nullptr;
    }

    void SendList(Player* player);

    void Learn(Player* player, Disguise const& d, bool free)
    {
        ChatHandler h(player->GetSession());
        if (Knows(player, d.id)) { h.SendSysMessage(Text(player, S_KNOWN_ALREADY, "You already know that disguise.")); return; }
        if (player->GetLevel() < d.level) { h.PSendSysMessage(Text(player, S_LEVEL, "You need to be level {} to learn that disguise."), d.level); return; }
        if (!free && d.cost)
        {
            if (!player->HasEnoughMoney(d.cost)) { h.PSendSysMessage(Text(player, S_MONEY, "That costs {} gold."), d.cost / 10000.0f); return; }
            player->ModifyMoney(-int32(d.cost));
        }
        known[player->GetGUID().GetCounter()].insert(d.id);
        CharacterDatabase.Execute("REPLACE INTO mod_temucraft_disguise_known (owner, disguise) VALUES ({}, {})", player->GetGUID().GetCounter(), d.id);
        h.PSendSysMessage(Text(player, S_LEARNED, "|cff66ccffDisguise:|r you learned {}. Use .disguise or the Disguise button by the minimap."), d.name);
        SendList(player);
    }

    void TakeOff(Player* player, bool broken)
    {
        auto it = worn.find(player->GetGUID().GetCounter());
        if (it == worn.end()) return;
        worn.erase(it);
        player->RestoreDisplayId();
        if (broken) brokenAt[player->GetGUID().GetCounter()] = Now();
        if (player->GetSession())
            ChatHandler(player->GetSession()).SendSysMessage(broken ? Text(player, S_BROKEN, "|cff66ccffDisguise:|r your disguise falls apart!")
                                                                    : Text(player, S_REMOVED, "|cff66ccffDisguise:|r you take off your disguise."));
        TemucraftAddonSend(player, PREFIX, "A~0");
    }

    void Wear(Player* player, Disguise const& d)
    {
        ChatHandler h(player->GetSession());
        if (!Knows(player, d.id)) { h.SendSysMessage(Text(player, S_NOT_KNOWN, "You don't know that disguise.")); return; }
        if (player->IsInCombat()) { h.SendSysMessage(Text(player, S_COMBAT, "You can't put on a disguise in combat.")); return; }
        auto cd = brokenAt.find(player->GetGUID().GetCounter());
        if (cd != brokenAt.end() && Now() - cd->second < cfg.cooldown)
        {
            h.PSendSysMessage(Text(player, S_COOLDOWN, "You need {} more seconds to put a new disguise together."), cfg.cooldown - (Now() - cd->second));
            return;
        }
        if (d.reagent && d.reagentCount && !player->HasItemCount(d.reagent, d.reagentCount))
        {
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(d.reagent);
            h.PSendSysMessage(Text(player, S_REAGENTS, "That disguise needs {} x{}."), item ? item->Name1 : "?", d.reagentCount);
            return;
        }
        if (d.reagent && d.reagentCount) player->DestroyItemCount(d.reagent, d.reagentCount, true);
        if (player->IsMounted()) player->Dismount();
        worn[player->GetGUID().GetCounter()] = d.id;
        player->SetDisplayId(d.display);
        h.PSendSysMessage(Text(player, S_DISGUISED, "|cff66ccffDisguise:|r you now look like {}. Any hostile act gives you away."), d.name);
        TemucraftAddonSend(player, PREFIX, Acore::StringFormat("A~{}", d.id));
    }

    // does a creature take this disguise for one of its own?
    bool Fools(Disguise const& d, Creature const* c)
    {
        if (c->IsGuard() && !cfg.fooledGuards) return false;
        if (c->isWorldBoss() || c->IsDungeonBoss()) return false;
        FactionTemplateEntry const* ft = c->GetFactionTemplateEntry();
        if (ft && d.faction && ft->faction == d.faction) return true;
        if (ft && d.side == SIDE_ALLIANCE && (ft->ourMask & FACTION_MASK_ALLIANCE)) return true;
        if (ft && d.side == SIDE_HORDE && (ft->ourMask & FACTION_MASK_HORDE)) return true;
        std::string const& name = c->GetName();
        for (std::string const& p : d.prefixes) if (!p.empty() && name.compare(0, p.size(), p) == 0) return true;
        return false;
    }

    void SendList(Player* player)
    {
        if (player->getClass() != CLASS_ROGUE) return;
        TemucraftAddonSend(player, PREFIX, "BEGIN");
        for (auto const& [id, d] : disguises)
        {
            ItemTemplate const* item = d.reagent ? sObjectMgr->GetItemTemplate(d.reagent) : nullptr;
            TemucraftAddonSend(player, PREFIX, Acore::StringFormat("D~{}~{}~{}~{}~{}~{}", d.id, TemucraftAddonClean(d.name), Knows(player, d.id) ? 1 : 0,
                d.level, item ? TemucraftAddonClean(item->Name1) : "", d.reagentCount));
        }
        auto w = worn.find(player->GetGUID().GetCounter());
        TemucraftAddonSend(player, PREFIX, Acore::StringFormat("A~{}", w != worn.end() ? w->second : 0));
        TemucraftAddonSend(player, PREFIX, "END");
    }

    // ------------------------------------------------------------------------------------------- hooks
    class temucraft_disguise_unit : public UnitScript
    {
    public:
        temucraft_disguise_unit() : UnitScript("temucraft_disguise_unit", true, { UNITHOOK_IF_NORMAL_REACTION, UNITHOOK_ON_DAMAGE }) { }

        bool IfNormalReaction(Unit const* unit, Unit const* target, ReputationRank& rep) override
        {
            if (worn.empty() || !cfg.enable || !unit || !target || !unit->IsCreature() || !target->IsPlayer()) return true;
            auto it = worn.find(target->GetGUID().GetCounter());
            if (it == worn.end()) return true;
            auto d = disguises.find(it->second);
            if (d == disguises.end() || !Fools(d->second, unit->ToCreature())) return true;
            rep = REP_NEUTRAL;
            return false;
        }

        void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
        {
            if (worn.empty() || !damage) return;
            if (attacker && attacker->IsPlayer() && worn.count(attacker->GetGUID().GetCounter())) TakeOff(attacker->ToPlayer(), true);
            if (victim && victim->IsPlayer() && worn.count(victim->GetGUID().GetCounter())) TakeOff(victim->ToPlayer(), true);
        }
    };

    class temucraft_disguise_player : public PlayerScript
    {
    public:
        temucraft_disguise_player() : PlayerScript("temucraft_disguise_player",
            { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_DELETE, PLAYERHOOK_ON_PLAYER_JUST_DIED, PLAYERHOOK_ON_PLAYER_ENTER_COMBAT,
              PLAYERHOOK_ON_SPELL_CAST, PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT }) { }

        void OnPlayerLogin(Player* player) override { if (player->getClass() == CLASS_ROGUE) LoadKnown(player); }
        void OnPlayerLogout(Player* player) override
        {
            ObjectGuid::LowType me = player->GetGUID().GetCounter();
            worn.erase(me); known.erase(me); brokenAt.erase(me);
        }
        void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
        {
            CharacterDatabase.Execute("DELETE FROM mod_temucraft_disguise_known WHERE owner = {}", guid.GetCounter());
        }
        void OnPlayerJustDied(Player* player) override { TakeOff(player, true); }
        void OnPlayerEnterCombat(Player* player, Unit* /*enemy*/) override { TakeOff(player, true); }
        void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
        {
            if (worn.empty() || !spell || !worn.count(player->GetGUID().GetCounter())) return;
            SpellInfo const* info = spell->GetSpellInfo();
            if (info && info->Id != SPELL_PICK_POCKET && !info->IsPositive()) TakeOff(player, true);
        }

        using PlayerScript::OnPlayerCanUseChat;
        bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver) override
        {
            std::string body;
            if (!TemucraftAddonParse(player, type, language, msg, receiver, PREFIX, body)) return true;
            if (!cfg.enable || player->getClass() != CLASS_ROGUE) return false;
            std::vector<std::string> a = TemucraftAddonFields(body);
            if (a.empty()) return false;
            if (a[0] == "LIST" || a[0] == "HELLO") SendList(player);
            else if (a[0] == "OFF") TakeOff(player, false);
            else if (a[0] == "USE" && a.size() >= 2)
                if (Optional<uint32> id = Acore::StringTo<uint32>(a[1]); id && disguises.count(*id)) Wear(player, disguises[*id]);
            return false;
        }
    };

    // rogue trainers: "Teach me the art of disguise" (the first disguise, free, from level 2)
    class npc_temucraft_disguise_trainer : public CreatureScript
    {
    public:
        npc_temucraft_disguise_trainer() : CreatureScript("npc_temucraft_disguise_trainer") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            player->PrepareGossipMenu(creature, creature->GetCreatureTemplate()->GossipMenuId, true);
            if (cfg.enable && player->getClass() == CLASS_ROGUE)
                if (Disguise const* d = BasicFor(player); d && !Knows(player, d->id))
                    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, Acore::StringFormat(Text(player, S_LEARN_BASIC, "Teach me the art of disguise ({})."), d->name), SENDER_DISGUISE, d->id);
            player->SendPreparedGossip(creature);
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 sender, uint32 action) override
        {
            if (sender != SENDER_DISGUISE) return false;   // the trainer's own options
            CloseGossipMenuFor(player);
            if (disguises.count(action)) Learn(player, disguises[action], true);
            return true;
        }
    };

    // disguise contacts out in the world
    class npc_temucraft_disguise_contact : public CreatureScript
    {
    public:
        npc_temucraft_disguise_contact() : CreatureScript("npc_temucraft_disguise_contact") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            ClearGossipMenuFor(player);
            if (cfg.enable && player->getClass() == CLASS_ROGUE)
                for (auto const& [id, d] : disguises)
                    if (d.contact == creature->GetEntry() && !Knows(player, id))
                        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, Acore::StringFormat(Text(player, S_LEARN_HERE, "Teach me to pass for {} (level {}, {} gold)."), d.name, d.level, d.cost / 10000), SENDER_DISGUISE, id, "", d.cost, false);
            SendGossipMenuFor(player, creature->GetEntry(), creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 sender, uint32 action) override
        {
            CloseGossipMenuFor(player);
            if (sender == SENDER_DISGUISE && disguises.count(action)) Learn(player, disguises[action], false);
            return true;
        }
    };

    class temucraft_disguise_world : public WorldScript
    {
    public:
        temucraft_disguise_world() : WorldScript("temucraft_disguise_world", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }
        void OnAfterConfigLoad(bool /*reload*/) override
        {
            cfg.enable = sConfigMgr->GetOption<bool>("Temucraft.RogueDisguise.Enable", true);
            cfg.cooldown = sConfigMgr->GetOption<uint32>("Temucraft.RogueDisguise.Cooldown", 10);
            cfg.fooledGuards = sConfigMgr->GetOption<bool>("Temucraft.RogueDisguise.FoolsGuards", true);
        }
        void OnStartup() override { Load(); }
    };

    class temucraft_disguise_commands : public CommandScript
    {
    public:
        temucraft_disguise_commands() : CommandScript("temucraft_disguise_commands") { }
        ChatCommandTable GetCommands() const override
        {
            static ChatCommandTable root = { { "disguise", Handle, SEC_PLAYER, Console::No } };
            return root;
        }
        static bool Handle(ChatHandler* handler, Optional<Tail> arg)
        {
            Player* p = handler->GetPlayer();
            if (!p) return true;
            if (!cfg.enable) { handler->SendSysMessage(Text(handler, S_OFF, "Disguises are turned off.")); return true; }
            if (p->getClass() != CLASS_ROGUE) { handler->SendSysMessage(Text(handler, S_NOT_ROGUE, "Only rogues know how to disguise themselves.")); return true; }
            std::string what = arg ? std::string(*arg) : "list";
            if (what == "list")
            {
                handler->SendSysMessage(Text(handler, S_LIST_HEAD, "Disguises (.disguise <name>, .disguise off):"));
                bool any = false;
                for (auto const& [id, d] : disguises)
                    if (Knows(p, id))
                    {
                        any = true;
                        ItemTemplate const* item = d.reagent ? sObjectMgr->GetItemTemplate(d.reagent) : nullptr;
                        handler->PSendSysMessage(Text(handler, S_LIST_LINE, "  {} - {} x{}"), d.name, item ? item->Name1 : "-", d.reagentCount);
                    }
                if (!any) handler->SendSysMessage(Text(handler, S_NONE, "  none yet - ask a rogue trainer."));
                return true;
            }
            if (what == "off") { TakeOff(p, false); return true; }
            if (Disguise const* d = Find(what)) Wear(p, *d);
            else handler->SendSysMessage(Text(handler, S_USAGE, "Usage: .disguise list | <name> | off"));
            return true;
        }
    };
}

void AddTemucraftRogueDisguiseScripts()
{
    new temucraft_disguise_world();
    new temucraft_disguise_unit();
    new temucraft_disguise_player();
    new npc_temucraft_disguise_trainer();
    new npc_temucraft_disguise_contact();
    new temucraft_disguise_commands();
}

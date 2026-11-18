# mod-temucraft-rogue-disguise

Rogues can disguise themselves as other humanoids, walk among them and trade with them, until they give themselves
away.

## How it plays

- **Learning**: from level 2 every rogue trainer offers "Teach me the art of disguise": a human peasant for Horde
  rogues, an orcish peon for Alliance rogues. The other disguises are taught (for gold) by **disguise contacts** who
  stand among the people they imitate:

| Disguise | Level | Cost | Wearing costs | Contact | Where |
|---|---|---|---|---|---|
| Human Peasant | 2 | free | Linen Cloth | rogue trainers (Horde) | |
| Orcish Peon | 2 | free | Linen Cloth | rogue trainers (Alliance) | |
| Defias Bandit | 12 | 50 s | 2 Linen Cloth | Vic the Quiet | Elwynn Forest, by the Defias camp |
| Stonesplinter Trogg | 16 | 1 g | 2 Wool Cloth | Grubbs | Loch Modan, Stonesplinter Valley |
| Syndicate Highwayman | 22 | 2 g | 3 Wool Cloth | Marla Twofaces | Hillsbrad / Alterac foothills |
| Dalaran Wizard | 30 | 5 g | 2 Silk Cloth | Apprentice Corwin | Alterac, the Dalaran crater |
| Southsea Pirate | 40 | 8 g | 2 Mageweave Cloth | One-Eye Sully | Tanaris, Lost Rigger Cove |
| Dark Iron Dwarf | 45 | 12 g | 3 Mageweave Cloth | Brakka Soot | Searing Gorge |

- **Wearing**: `.disguise <name>` or the **Disguise** button next to the minimap (built into the Temucraft interface
  by [mod-temucraft-client-version](https://github.com/Temucraft/mod-temucraft-client-version); `/disguise` opens the same menu). You take the look of that group; its members
  (same faction, or a name starting with the group's name) take you for one of their own: they don't attack, you can
  talk to them and trade. The peasant fools the Alliance's people, the peon the Horde's (guards too, unless
  `FoolsGuards = 0`). Bosses are never fooled.
- **Breaking**: dealing or taking damage, entering combat, casting a hostile spell (Pick Pocket is fine), dying and
  logging out end it; a new one can be put on after `Cooldown` seconds. `.disguise off` takes it off.

## Data

- `mod_temucraft_disguise` (world): the disguises: look (a creature entry; its faction is the one fooled), side,
  level, cost, reagent, extra name prefixes, whether rogue trainers teach it, the contact. Add your own rows.
- `mod_temucraft_disguise_known` (characters): what each rogue has learned.
- Contacts 608820-608825 (spawns 6088200-6088205) with their texts; rogue trainers without a script get
  `npc_temucraft_disguise_trainer` (their own menu stays).

## Install

Put this folder in `modules/`, copy the conf, re-run CMake and build; the SQL is applied at the next start. Texts are
in `module_string` (module `mod-temucraft-rogue-disguise`), translations in `module_string_locale`.

Part of the Temucraft modules for AzerothCore 3.3.5a.

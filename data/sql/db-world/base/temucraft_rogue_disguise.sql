-- mod-temucraft-rogue-disguise: disguises, the contacts who teach them, rogue trainers and the texts

CREATE TABLE IF NOT EXISTS `mod_temucraft_disguise` (
  `id` INT UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL,
  `model_entry` INT UNSIGNED NOT NULL COMMENT 'creature whose look (and faction) the disguise copies',
  `side` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 = the Alliance''s people take you for one of theirs, 2 = the Horde''s',
  `required_level` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `cost` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'copper, paid to the contact',
  `reagent` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'used up each time you put it on',
  `reagent_count` INT UNSIGNED NOT NULL DEFAULT 0,
  `name_prefixes` VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'creatures whose names start with one of these (comma separated) are fooled too',
  `basic` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'taught by rogue trainers (the side you are not)',
  `contact_entry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'the contact creature who teaches it',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- reagents: Linen Cloth 2589, Wool Cloth 2592, Silk Cloth 4306, Mageweave Cloth 4338
DELETE FROM `mod_temucraft_disguise`;
INSERT INTO `mod_temucraft_disguise` (`id`, `name`, `model_entry`, `side`, `required_level`, `cost`, `reagent`, `reagent_count`, `name_prefixes`, `basic`, `contact_entry`) VALUES
(1, 'Human Peasant',      351,   1,  2,      0, 2589, 1, '',              1, 0),
(2, 'Orcish Peon',        14901, 2,  2,      0, 2589, 1, '',              1, 0),
(3, 'Defias Bandit',      116,   0, 12,   5000, 2589, 2, 'Defias',        0, 608820),
(4, 'Stonesplinter Trogg',1161,  0, 16,  10000, 2592, 2, 'Stonesplinter', 0, 608821),
(5, 'Syndicate Highwayman',2586, 0, 22,  20000, 2592, 3, 'Syndicate',     0, 608822),
(6, 'Dalaran Wizard',     1867,  0, 30,  50000, 4306, 2, 'Dalaran',       0, 608823),
(7, 'Southsea Pirate',    7856,  0, 40,  80000, 4338, 2, 'Southsea',      0, 608824),
(8, 'Dark Iron Dwarf',    8566,  0, 45, 120000, 4338, 3, 'Dark Iron',     0, 608825);

-- the contacts: rogues in disguise among the people they imitate (they look like them)
DELETE FROM `creature_template` WHERE `entry` BETWEEN 608820 AND 608825;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`,
  `speed_walk`, `speed_run`, `rank`, `unit_class`, `unit_flags`, `type`, `type_flags`, `AIName`, `MovementType`, `HealthModifier`, `RegenHealth`,
  `flags_extra`, `ScriptName`) VALUES
(608820, 'Vic the Quiet',     'Disguise Contact', '', 0, 20, 20, 0, 35, 1, 1, 1.14286, 0, 1, 768, 7, 0, '', 0, 1, 1, 2, 'npc_temucraft_disguise_contact'),
(608821, 'Grubbs',            'Disguise Contact', '', 0, 25, 25, 0, 35, 1, 1, 1.14286, 0, 1, 768, 7, 0, '', 0, 1, 1, 2, 'npc_temucraft_disguise_contact'),
(608822, 'Marla Twofaces',    'Disguise Contact', '', 0, 30, 30, 0, 35, 1, 1, 1.14286, 0, 1, 768, 7, 0, '', 0, 1, 1, 2, 'npc_temucraft_disguise_contact'),
(608823, 'Apprentice Corwin', 'Disguise Contact', '', 0, 40, 40, 0, 35, 1, 1, 1.14286, 0, 8, 768, 7, 0, '', 0, 1, 1, 2, 'npc_temucraft_disguise_contact'),
(608824, 'One-Eye Sully',     'Disguise Contact', '', 0, 50, 50, 0, 35, 1, 1, 1.14286, 0, 1, 768, 7, 0, '', 0, 1, 1, 2, 'npc_temucraft_disguise_contact'),
(608825, 'Brakka Soot',       'Disguise Contact', '', 0, 55, 55, 0, 35, 1, 1, 1.14286, 0, 1, 768, 7, 0, '', 0, 1, 1, 2, 'npc_temucraft_disguise_contact');
DELETE FROM `creature_template_model` WHERE `CreatureID` BETWEEN 608820 AND 608825;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT 608820, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 116 AND `Idx` = 0 UNION ALL
SELECT 608821, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 1161 AND `Idx` = 0 UNION ALL
SELECT 608822, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 2586 AND `Idx` = 0 UNION ALL
SELECT 608823, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 1867 AND `Idx` = 0 UNION ALL
SELECT 608824, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 7856 AND `Idx` = 0 UNION ALL
SELECT 608825, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 8566 AND `Idx` = 0;

-- spawned next to their people: Elwynn/Westfall border (Defias), Loch Modan (Stonesplinter), Hillsbrad (Syndicate),
-- Alterac / Dalaran crater (Dalaran), Tanaris (Southsea), Searing Gorge (Dark Iron)
DELETE FROM `creature` WHERE `guid` BETWEEN 6088200 AND 6088205;
INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`) VALUES
(6088200, 608820, 0, 1, 1, -9742.70, -813.93, 42.01, 1.29, 300),
(6088201, 608821, 0, 1, 1, -5782.19, -2877.61, 363.96, 1.93, 300),
(6088202, 608822, 0, 1, 1, -846.40, -1979.73, 35.97, 1.37, 300),
(6088203, 608823, 0, 1, 1, -130.73, 1285.73, 50.71, 2.26, 300),
(6088204, 608824, 1, 1, 1, -8013.19, -5280.62, 0.66, 3.37, 300),
(6088205, 608825, 0, 1, 1, -7028.76, -1274.01, 242.33, 0.89, 300);

DELETE FROM `npc_text` WHERE `ID` BETWEEN 608820 AND 608825;
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `Probability0`) VALUES
(608820, 'Keep your voice down. Nobody in Moonbrook asks questions of a man in a red bandana, and neither should you.', '', 1),
(608821, 'Hunch your back, grunt a lot, smell of the mine. That''s all a trogg is, really.', '', 1),
(608822, 'The Syndicate trusts a good coat and a confident walk. I can teach you both, for a price.', '', 1),
(608823, 'A robe, a staff and a bored expression. Dalaran''s exiles never look closer than that.', '', 1),
(608824, 'Arr. The trick to being a pirate is never to stop being one where the others can see.', '', 1),
(608825, 'Soot on the face, a hammer on the belt and a grudge against everyone. Welcome to the Dark Irons.', '', 1);

-- every rogue trainer teaches the first disguise (only trainers without a script of their own)
UPDATE `creature_template` SET `ScriptName` = 'npc_temucraft_disguise_trainer'
WHERE `ScriptName` = '' AND `entry` IN (SELECT `CreatureId` FROM `creature_default_trainer` WHERE `TrainerId` IN (SELECT `Id` FROM `trainer` WHERE `Type` = 0 AND `Requirement` = 4));

DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-rogue-disguise';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-rogue-disguise', 1, 'Disguises are turned off.'),
('mod-temucraft-rogue-disguise', 2, 'Only rogues know how to disguise themselves.'),
('mod-temucraft-rogue-disguise', 3, 'Teach me the art of disguise ({}).'),
('mod-temucraft-rogue-disguise', 4, '|cff66ccffDisguise:|r you learned {}. Use .disguise or the Disguise button by the minimap.'),
('mod-temucraft-rogue-disguise', 5, 'You need to be level {} to learn that disguise.'),
('mod-temucraft-rogue-disguise', 6, 'That costs {} gold.'),
('mod-temucraft-rogue-disguise', 7, 'You already know that disguise.'),
('mod-temucraft-rogue-disguise', 8, 'You don''t know that disguise.'),
('mod-temucraft-rogue-disguise', 9, 'That disguise needs {} x{}.'),
('mod-temucraft-rogue-disguise', 10, 'You can''t put on a disguise in combat.'),
('mod-temucraft-rogue-disguise', 11, '|cff66ccffDisguise:|r you now look like {}. Any hostile act gives you away.'),
('mod-temucraft-rogue-disguise', 12, '|cff66ccffDisguise:|r your disguise falls apart!'),
('mod-temucraft-rogue-disguise', 13, '|cff66ccffDisguise:|r you take off your disguise.'),
('mod-temucraft-rogue-disguise', 14, 'Disguises (.disguise <name>, .disguise off):'),
('mod-temucraft-rogue-disguise', 15, '  {} - {} x{}'),
('mod-temucraft-rogue-disguise', 16, 'Usage: .disguise list | <name> | off'),
('mod-temucraft-rogue-disguise', 17, 'Teach me to pass for {} (level {}, {} gold).'),
('mod-temucraft-rogue-disguise', 18, 'You need {} more seconds to put a new disguise together.'),
('mod-temucraft-rogue-disguise', 19, '  none yet - ask a rogue trainer.');
-- Translations go in `module_string_locale` (`module`, `id`, `locale`, `string`).

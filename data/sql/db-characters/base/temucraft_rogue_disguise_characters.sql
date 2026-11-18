-- mod-temucraft-rogue-disguise: the disguises each rogue knows
CREATE TABLE IF NOT EXISTS `mod_temucraft_disguise_known` (
  `owner` INT UNSIGNED NOT NULL COMMENT 'character guid',
  `disguise` INT UNSIGNED NOT NULL COMMENT 'mod_temucraft_disguise.id',
  PRIMARY KEY (`owner`, `disguise`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

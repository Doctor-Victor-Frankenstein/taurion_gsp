CREATE TABLE IF NOT EXISTS `characters` (
  `id` INTEGER PRIMARY KEY,

  -- The Xaya name that owns this character (and is thus allowed to send
  -- moves for it).
  `owner` TEXT NOT NULL,

  -- The name of the character in the game world.  This has to be unique
  -- among all characters and is used mainly for display purposes.  It is
  -- tracked directly in the database (rather than the encoded proto) so
  -- that we can verify uniqueness when creating a new character.
  `name` TEXT NOT NULL UNIQUE,

  -- Current position of the character on the map.  We need this in the table
  -- so that we can look characters up based on "being near" a given other
  -- coordinate.  Coordinates are in the axial system (as everywhere else
  -- in the backend).
  `x` INTEGER NOT NULL,
  `y` INTEGER NOT NULL,

  -- Flag indicating if the character is currently moving.  This is set
  -- based on the encoded protocol buffer when updating the table, and is
  -- used so that we can efficiently retrieve only those characters that are
  -- moving when we do move updates.
  `ismoving` INTEGER NOT NULL,

  -- Additional data encoded as a Character protocol buffer.
  `proto` BLOB NOT NULL
);

-- Non-unique indices for the characters table.
CREATE INDEX IF NOT EXISTS `characters_owner` ON `characters` (`owner`);
CREATE INDEX IF NOT EXISTS `characters_pos` ON `characters` (`x`, `y`);
CREATE INDEX IF NOT EXISTS `characters_ismoving` ON `characters` (`ismoving`);
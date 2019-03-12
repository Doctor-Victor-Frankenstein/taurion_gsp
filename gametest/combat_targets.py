#!/usr/bin/env python

from pxtest import PXTest, offsetCoord

"""
Tests the target selection aspect of combat.
"""


class CombatTargetTest (PXTest):

  def getTargetCharacter (self, owner):
    """
    Looks up the target of the given character (by owner).  Returns None
    if there is none.  If there is one, verify that it refers to a character
    and return that character's name (rather than ID).
    """

    chars = self.getCharacters ()
    assert owner in chars
    c = chars[owner]

    if "combat" not in c.data:
      return None
    if "target" not in c.data["combat"]:
      return None

    t = c.data["combat"]["target"]
    self.assertEqual (t["type"], "character")

    charId = t["id"]
    for nm, val in chars.iteritems ():
      if val.getId () == charId:
        return nm

    raise AssertionError ("Character with id %d not found" % charId)

  def stepCharacterUntil (self, owner, pos):
    """
    Generates new blocks (one by one) until the character with the given
    owner is at the given position.

    Returns the number of generated blocks.
    """

    pos = offsetCoord (pos, self.offset, False)

    cnt = 0
    while True:
      c = self.getCharacters ()[owner]
      if c.getPosition () == pos:
        return cnt
      assert c.isMoving ()
      self.generate (1)
      cnt += 1

  def run (self):
    self.generate (101);

    self.mainLogger.info ("Creating test characters...")
    self.createCharacter ("a", "r")
    self.createCharacter ("b", "g")
    self.createCharacter ("c", "g")
    self.generate (1)

    # We use the starting coordinate of bar as offset for coordinates,
    # so that the test is independent of the actual starting positions
    # of the characters.
    c = self.getCharacters ()["b"]
    self.offset = c.getPosition ()

    # Move all other characters to the same origin position before
    # continuing with the rest of the test.
    self.moveCharactersTo ({
      "a": self.offset,
      "c": self.offset,
    })

    self.mainLogger.info ("Testing randomised target selection...")
    cnts = {"b": 0, "c": 0}
    rolls = 10
    for _ in range (rolls):
      self.generate (1)
      self.assertEqual (self.getTargetCharacter ("b"), "a")
      self.assertEqual (self.getTargetCharacter ("c"), "a")
      fooTarget = self.getTargetCharacter ("a")
      assert fooTarget is not None
      assert fooTarget in cnts
      cnts[fooTarget] += 1
      # Invalidate the last block so that we reroll the randomisation
      # with the next generated block.
      self.rpc.xaya.invalidateblock (self.rpc.xaya.getbestblockhash ())
    for key, cnt in cnts.iteritems ():
      self.log.info ("Target %s selected %d / %d times" % (key, cnt, rolls))
      assert cnt > 0

    self.mainLogger.info ("Testing target selection when moving into range...")
    c = self.getCharacters ()["a"]
    wp = [{"x": 20, "y": 0}, {"x": 0, "y": 0}]
    wp = [offsetCoord (p, self.offset, False) for p in wp]
    c.sendMove ({"wp": wp})
    self.generate (35)
    self.stepCharacterUntil ("a", {"x": 11, "y": 0})
    assert self.getTargetCharacter ("b") is None
    self.stepCharacterUntil ("a", {"x": 10, "y": 0})
    self.assertEqual (self.getTargetCharacter ("b"), "a")


if __name__ == "__main__":
  CombatTargetTest ().main ()

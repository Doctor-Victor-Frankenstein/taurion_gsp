#include "gamestatejson.hpp"

#include "protoutils.hpp"

#include "database/character.hpp"
#include "database/dbtest.hpp"
#include "proto/character.pb.h"

#include <gtest/gtest.h>

#include <json/json.h>

#include <sstream>
#include <string>

namespace pxd
{
namespace
{

class GameStateJsonTests : public DBTestWithSchema
{

protected:

  /**
   * Expects that the current state equals to the given one, after parsing
   * the expected state's string as JSON.  In other words, not strings are
   * compared, but the resulting JSON objects.
   */
  void
  ExpectStateJson (const std::string& expectedStr)
  {
    Json::Value expected;
    std::istringstream in(expectedStr);
    in >> expected;

    const Json::Value actual = GameStateToJson (*db);
    VLOG (1) << "Actual JSON for the game state:\n" << actual;
    ASSERT_EQ (actual, expected);
  }

};

/* ************************************************************************** */

class CharacterJsonTests : public GameStateJsonTests
{

protected:

  CharacterTable tbl;

  CharacterJsonTests ()
    : tbl(*db)
  {}

};

TEST_F (CharacterJsonTests, Basic)
{
  auto c = tbl.CreateNew ("domob", "foo");
  c->SetPosition (HexCoord (-5, 2));
  c.reset ();

  tbl.CreateNew ("andy", u8"äöü");

  ExpectStateJson (u8R"({
    "characters":
      [
        {
          "id": 1, "name": "foo", "owner": "domob",
          "position": {"x": -5, "y": 2}
        },
        {
          "id": 2, "name": "äöü", "owner": "andy",
          "position": {"x": -0, "y": 0}
        }
      ]
  })");
}

TEST_F (CharacterJsonTests, Waypoints)
{
  auto c = tbl.CreateNew ("domob", "foo");
  c->SetPartialStep (5);
  auto* wp = c->MutableProto ().mutable_movement ()->mutable_waypoints ();
  *wp->Add () = CoordToProto (HexCoord (-3, 0));
  *wp->Add () = CoordToProto (HexCoord (0, 42));
  c.reset ();

  ExpectStateJson (R"({
    "characters":
      [
        {
          "id": 1, "name": "foo", "owner": "domob",
          "position": {"x": 0, "y": 0},
          "movement":
            {
              "partialstep": 5,
              "waypoints": [{"x": -3, "y": 0}, {"x": 0, "y": 42}]
            }
        }
      ]
  })");
}

TEST_F (CharacterJsonTests, OnlyOneStep)
{
  auto c = tbl.CreateNew ("domob", "foo");
  c->SetPosition (HexCoord (2, 3));
  auto* mvProto = c->MutableProto ().mutable_movement ();
  *mvProto->mutable_waypoints ()->Add () = CoordToProto (HexCoord (42, -42));
  *mvProto->mutable_steps ()->Add () = CoordToProto (HexCoord (2, 3));
  c.reset ();

  ExpectStateJson (R"({
    "characters":
      [
        {
          "id": 1, "name": "foo", "owner": "domob",
          "position": {"x": 2, "y": 3},
          "movement":
            {
              "waypoints": [{"x": 42, "y": -42}],
              "steps": [{"x": 42, "y": -42}]
            }
        }
      ]
  })");
}

TEST_F (CharacterJsonTests, PositionIsLastStep)
{
  auto c = tbl.CreateNew ("domob", "foo");
  c->SetPosition (HexCoord (2, 3));
  auto* mvProto = c->MutableProto ().mutable_movement ();
  *mvProto->mutable_waypoints ()->Add () = CoordToProto (HexCoord (42, -42));
  *mvProto->mutable_steps ()->Add () = CoordToProto (HexCoord (-5, 10));
  *mvProto->mutable_steps ()->Add () = CoordToProto (HexCoord (2, 3));
  c.reset ();

  ExpectStateJson (R"({
    "characters":
      [
        {
          "id": 1, "name": "foo", "owner": "domob",
          "position": {"x": 2, "y": 3},
          "movement":
            {
              "waypoints": [{"x": 42, "y": -42}],
              "steps": [{"x": 42, "y": -42}]
            }
        }
      ]
  })");
}

TEST_F (CharacterJsonTests, MultipleStep)
{
  auto c = tbl.CreateNew ("domob", "foo");
  c->SetPosition (HexCoord (2, 3));
  auto* mvProto = c->MutableProto ().mutable_movement ();
  *mvProto->mutable_waypoints ()->Add () = CoordToProto (HexCoord (42, -42));
  *mvProto->mutable_steps ()->Add () = CoordToProto (HexCoord (-5, 10));
  *mvProto->mutable_steps ()->Add () = CoordToProto (HexCoord (2, 3));
  *mvProto->mutable_steps ()->Add () = CoordToProto (HexCoord (7, 8));
  c.reset ();

  ExpectStateJson (R"({
    "characters":
      [
        {
          "id": 1, "name": "foo", "owner": "domob",
          "position": {"x": 2, "y": 3},
          "movement":
            {
              "waypoints": [{"x": 42, "y": -42}],
              "steps":
                [
                  {"x": 7, "y": 8},
                  {"x": 42, "y": -42}
                ]
            }
        }
      ]
  })");
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

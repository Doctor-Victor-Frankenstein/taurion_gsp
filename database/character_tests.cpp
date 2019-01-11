#include "character.hpp"

#include "dbtest.hpp"

#include "proto/character.pb.h"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

class CharacterTests : public DBTestWithSchema
{

protected:

  /** CharacterTable instance for tests.  */
  CharacterTable tbl;

  CharacterTests ()
    : tbl(*db)
  {}

};

TEST_F (CharacterTests, Creation)
{
  const HexCoord pos(5, -2);

  auto c = tbl.CreateNew  ("domob", "abc");
  c->SetPosition (pos);
  const unsigned id1 = c->GetId ();
  c.reset ();

  c = tbl.CreateNew ("domob", u8"äöü");
  const unsigned id2 = c->GetId ();
  c->MutableProto ().mutable_movement ();
  c.reset ();

  auto res = tbl.QueryAll ();

  ASSERT_TRUE (res.Step ());
  c = tbl.GetFromResult (res);
  ASSERT_EQ (c->GetId (), id1);
  EXPECT_EQ (c->GetOwner (), "domob");
  EXPECT_EQ (c->GetName (), "abc");
  EXPECT_EQ (c->GetPosition (), pos);
  EXPECT_FALSE (c->GetProto ().has_movement ());

  ASSERT_TRUE (res.Step ());
  c = tbl.GetFromResult (res);
  ASSERT_EQ (c->GetId (), id2);
  EXPECT_EQ (c->GetOwner (), "domob");
  EXPECT_EQ (c->GetName (), u8"äöü");
  EXPECT_TRUE (c->GetProto ().has_movement ());

  ASSERT_FALSE (res.Step ());
}

TEST_F (CharacterTests, Modification)
{
  const HexCoord pos1(5, -2);
  const HexCoord pos2(-2, 5);

  auto c = tbl.CreateNew ("domob", "foo");
  c->SetPosition (pos1);
  c.reset ();

  auto res = tbl.QueryAll ();
  ASSERT_TRUE (res.Step ());
  c = tbl.GetFromResult (res);
  EXPECT_EQ (c->GetOwner (), "domob");
  EXPECT_EQ (c->GetPosition (), pos1);
  EXPECT_FALSE (c->GetProto ().has_movement ());
  ASSERT_FALSE (res.Step ());

  c->SetOwner ("andy");
  c->SetPosition (pos2);
  c->MutableProto ().mutable_movement ();
  c.reset ();

  res = tbl.QueryAll ();
  ASSERT_TRUE (res.Step ());
  c = tbl.GetFromResult (res);
  EXPECT_EQ (c->GetOwner (), "andy");
  EXPECT_EQ (c->GetPosition (), pos2);
  EXPECT_TRUE (c->GetProto ().has_movement ());
  ASSERT_FALSE (res.Step ());
}

TEST_F (CharacterTests, EmptyNameNotAllowed)
{
  EXPECT_DEATH ({
    tbl.CreateNew ("domob", "");
  }, "name");
}

using CharacterTableTests = CharacterTests;

TEST_F (CharacterTableTests, QueryForOwner)
{
  tbl.CreateNew ("domob", "abc");
  tbl.CreateNew ("domob", "foo");
  tbl.CreateNew ("andy", "test");

  auto res = tbl.QueryForOwner ("domob");
  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (tbl.GetFromResult (res)->GetName (), "abc");
  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (tbl.GetFromResult (res)->GetName (), "foo");
  ASSERT_FALSE (res.Step ());

  res = tbl.QueryForOwner ("not there");
  ASSERT_FALSE (res.Step ());
}

TEST_F (CharacterTableTests, IsValidName)
{
  tbl.CreateNew ("domob", "abc");

  EXPECT_FALSE (tbl.IsValidName (""));
  EXPECT_FALSE (tbl.IsValidName ("abc"));
  EXPECT_TRUE (tbl.IsValidName ("foo"));
}

} // anonymous namespace
} // namespace pxd
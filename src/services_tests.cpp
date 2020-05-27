/*
    GSP for the Taurion blockchain game
    Copyright (C) 2020  Autonomous Worlds Ltd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "services.hpp"

#include "testutils.hpp"

#include "database/dbtest.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

/* ************************************************************************** */

class ServicesTests : public DBTestWithSchema
{

private:

  TestRandom rnd;

protected:

  /** ID of an ancient building with all services.  */
  static constexpr Database::IdT ANCIENT_BUILDING = 100;

  ContextForTesting ctx;

  AccountsTable accounts;
  BuildingsTable buildings;
  BuildingInventoriesTable inv;
  CharacterTable characters;
  ItemCounts itemCounts;
  OngoingsTable ongoings;

  ServicesTests ()
    : accounts(db), buildings(db), inv(db), characters(db),
      itemCounts(db), ongoings(db)
  {
    accounts.CreateNew ("domob", Faction::RED)->AddBalance (100);

    db.SetNextId (ANCIENT_BUILDING);
    auto b = buildings.CreateNew ("ancient1", "", Faction::ANCIENT);
    CHECK_EQ (b->GetId (), ANCIENT_BUILDING);
    b->SetCentre (HexCoord (42, 10));

    /* We use refining for most general tests, thus it makes sense to set up
       basic resources for it already here.  */
    inv.Get (ANCIENT_BUILDING, "domob")
        ->GetInventory ().AddFungibleCount ("test ore", 10);
  }

  /**
   * Calls TryServiceOperation with the given account and data parsed from
   * a JSON literal string.  Returns true if the operation was valid.
   */
  bool
  Process (const std::string& name, const std::string& dataStr)
  {
    auto a = accounts.GetByName (name);
    auto op = ServiceOperation::Parse (*a, ParseJson (dataStr),
                                       ctx, accounts, buildings,
                                       inv, characters, itemCounts, ongoings);

    if (op == nullptr || !op->IsFullyValid ())
      return false;

    op->Execute (rnd);
    return true;
  }

  /**
   * Parses the given operation and returns its associated pending JSON.
   */
  Json::Value
  GetPendingJson (const std::string& name, const std::string& dataStr)
  {
    auto a = accounts.GetByName (name);
    auto op = ServiceOperation::Parse (*a, ParseJson (dataStr),
                                       ctx, accounts, buildings,
                                       inv, characters, itemCounts, ongoings);
    CHECK (op != nullptr);
    return op->ToPendingJson ();
  }

};

constexpr Database::IdT ServicesTests::ANCIENT_BUILDING;

TEST_F (ServicesTests, BasicOperation)
{
  ASSERT_TRUE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 3
  })"));

  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 90);
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("test ore"), 7);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("bar"), 2);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("zerospace"), 1);
}

TEST_F (ServicesTests, InvalidFormat)
{
  EXPECT_FALSE (Process ("domob", "[]"));
  EXPECT_FALSE (Process ("domob", "null"));
  EXPECT_FALSE (Process ("domob", "\"foo\""));
  EXPECT_FALSE (Process ("domob", "{}"));

  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "i": "test ore",
    "n": 6
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": "invalid",
    "i": "test ore",
    "n": 6
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 42,
    "i": "test ore",
    "n": 6
  })"));

  EXPECT_FALSE (Process ("domob", R"({
    "b": 100,
    "i": "test ore",
    "n": 6
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "",
    "b": 100,
    "i": "test ore",
    "n": 6
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "invalid type",
    "b": 100,
    "i": "test ore",
    "n": 6
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": 42,
    "b": 100,
    "i": "test ore",
    "n": 6
  })"));
}

TEST_F (ServicesTests, InvalidOperation)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 5
  })"));
}

TEST_F (ServicesTests, UnsupportedBuilding)
{
  db.SetNextId (200);
  buildings.CreateNew ("checkmark", "", Faction::ANCIENT);
  buildings.CreateNew ("ancient1", "", Faction::ANCIENT)
      ->MutableProto ().set_foundation (true);
  inv.Get (200, "domob")->GetInventory ().AddFungibleCount ("test ore", 10);
  inv.Get (201, "domob")->GetInventory ().AddFungibleCount ("test ore", 10);

  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 200,
    "i": "test ore",
    "n": 3
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 201,
    "i": "test ore",
    "n": 3
  })"));
}

TEST_F (ServicesTests, InsufficientFunds)
{
  accounts.GetByName ("domob")->AddBalance (-91);

  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 3
  })"));
}

TEST_F (ServicesTests, PendingJson)
{
  accounts.CreateNew ("andy", Faction::RED);

  auto b = buildings.CreateNew ("ancient1", "andy", Faction::RED);
  ASSERT_EQ (b->GetId (), 101);
  b->MutableProto ().set_service_fee_percent (50);
  b.reset ();

  inv.Get (101, "domob")->GetInventory ().AddFungibleCount ("test ore", 10);

  EXPECT_TRUE (PartialJsonEqual (GetPendingJson ("domob", R"({
    "t": "ref",
    "b": 101,
    "i": "test ore",
    "n": 6
  })"), ParseJson (R"({
    "building": 101,
    "cost":
      {
        "base": 20,
        "fee": 10
      }
  })")));
}

/* ************************************************************************** */

class ServicesFeeTests : public ServicesTests
{

protected:

  ServicesFeeTests ()
  {
    /* For some fee tests, we need an account with just enough balance
       for the base cost.  This will be "andy" (as opposed to "domob" who
       has 100 coins).  */
    accounts.CreateNew ("andy", Faction::RED)->AddBalance (10);

    CHECK_EQ (buildings.CreateNew ("ancient1", "andy", Faction::RED)
                ->GetId (), 101);

    inv.Get (ANCIENT_BUILDING, "andy")
        ->GetInventory ().AddFungibleCount ("test ore", 10);
    inv.Get (101, "andy")
        ->GetInventory ().AddFungibleCount ("test ore", 10);
    inv.Get (101, "domob")
        ->GetInventory ().AddFungibleCount ("test ore", 10);
  }

};

TEST_F (ServicesFeeTests, NoFeeInAncientBuilding)
{
  ASSERT_TRUE (Process ("andy", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 3
  })"));
  EXPECT_EQ (accounts.GetByName ("andy")->GetBalance (), 0);
}

TEST_F (ServicesFeeTests, NoFeeInOwnBuilding)
{
  buildings.GetById (101)->MutableProto ().set_service_fee_percent (50);
  ASSERT_TRUE (Process ("andy", R"({
    "t": "ref",
    "b": 101,
    "i": "test ore",
    "n": 3
  })"));
  EXPECT_EQ (accounts.GetByName ("andy")->GetBalance (), 0);
}

TEST_F (ServicesFeeTests, InsufficientBalanceWithFee)
{
  auto b = buildings.CreateNew ("ancient1", "domob", Faction::RED);
  ASSERT_EQ (b->GetId (), 102);
  b->MutableProto ().set_service_fee_percent (50);
  b.reset ();

  inv.Get (102, "andy")
      ->GetInventory ().AddFungibleCount ("test ore", 10);

  ASSERT_FALSE (Process ("andy", R"({
    "t": "ref",
    "b": 102,
    "i": "test ore",
    "n": 3
  })"));
  EXPECT_EQ (accounts.GetByName ("andy")->GetBalance (), 10);
}

TEST_F (ServicesFeeTests, NormalFeePayment)
{
  buildings.GetById (101)->MutableProto ().set_service_fee_percent (50);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "ref",
    "b": 101,
    "i": "test ore",
    "n": 3
  })"));
  EXPECT_EQ (accounts.GetByName ("andy")->GetBalance (), 15);
  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 85);
}

TEST_F (ServicesFeeTests, ZeroFeePossible)
{
  buildings.GetById (101)->MutableProto ().set_service_fee_percent (0);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "ref",
    "b": 101,
    "i": "test ore",
    "n": 3
  })"));
  EXPECT_EQ (accounts.GetByName ("andy")->GetBalance (), 10);
  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 90);
}

TEST_F (ServicesFeeTests, FeeRoundedUp)
{
  buildings.GetById (101)->MutableProto ().set_service_fee_percent (1);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "ref",
    "b": 101,
    "i": "test ore",
    "n": 3
  })"));
  EXPECT_EQ (accounts.GetByName ("andy")->GetBalance (), 11);
  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 89);
}

/* ************************************************************************** */

using RefiningTests = ServicesTests;

TEST_F (RefiningTests, InvalidFormat)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 3,
    "x": false
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": 42,
    "n": 3
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": -3
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": "x"
  })"));
}

TEST_F (RefiningTests, InvalidItemType)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "invalid item",
    "n": 3
  })"));
}

TEST_F (RefiningTests, ItemNotRefinable)
{
  inv.Get (ANCIENT_BUILDING, "domob")
      ->GetInventory ().AddFungibleCount ("foo", 10);

  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "foo",
    "n": 3
  })"));
}

TEST_F (RefiningTests, InvalidAmount)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": -3
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 0
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 2
  })"));
}

TEST_F (RefiningTests, TooMuch)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 30
  })"));
}

TEST_F (RefiningTests, MultipleSteps)
{
  ASSERT_TRUE (Process ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 9
  })"));

  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 70);
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("test ore"), 1);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("bar"), 6);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("zerospace"), 3);
}

TEST_F (RefiningTests, PendingJson)
{
  EXPECT_TRUE (PartialJsonEqual (GetPendingJson ("domob", R"({
    "t": "ref",
    "b": 100,
    "i": "test ore",
    "n": 6
  })"), ParseJson (R"({
    "type": "refining",
    "input": {"test ore": 6},
    "output": {"bar": 4, "zerospace": 2}
  })")));
}

/* ************************************************************************** */

class RepairTests : public ServicesTests
{

protected:

  RepairTests ()
  {
    db.SetNextId (200);
    auto c = characters.CreateNew ("domob", Faction::RED);
    c->SetBuildingId (ANCIENT_BUILDING);
    c->MutableRegenData ().mutable_max_hp ()->set_armour (1'000);
    c->MutableHP ().set_armour (950);

    ctx.SetHeight (100);
  }

};

TEST_F (RepairTests, InvalidFormat)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200,
    "x": false
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": "foo"
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": -10
  })"));
}

TEST_F (RepairTests, NonExistantCharacter)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 12345
  })"));
}

TEST_F (RepairTests, NonOwnedCharacter)
{
  accounts.CreateNew ("andy", Faction::RED)->AddBalance (100);
  EXPECT_FALSE (Process ("andy", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));
}

TEST_F (RepairTests, NotInBuilding)
{
  characters.GetById (200)->SetPosition (HexCoord (1, 2));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));

  characters.GetById (200)->SetBuildingId (5);
  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));
}

TEST_F (RepairTests, NoMissingHp)
{
  characters.GetById (200)->MutableHP ().set_armour (1'000);
  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));
}

TEST_F (RepairTests, BasicExecution)
{
  ASSERT_TRUE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));

  auto c = characters.GetById (200);
  ASSERT_TRUE (c->IsBusy ());
  EXPECT_EQ (c->GetHP ().armour (), 950);

  auto op = ongoings.GetById (c->GetProto ().ongoing ());
  EXPECT_EQ (op->GetHeight (), 101);
  EXPECT_EQ (op->GetCharacterId (), c->GetId ());
  EXPECT_TRUE (op->GetProto ().has_armour_repair ());

  op.reset ();
  c.reset ();

  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 95);
}

TEST_F (RepairTests, SingleHpMissing)
{
  characters.GetById (200)->MutableHP ().set_armour (999);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));

  auto c = characters.GetById (200);
  ASSERT_TRUE (c->IsBusy ());
  auto op = ongoings.GetById (c->GetProto ().ongoing ());
  EXPECT_EQ (op->GetHeight (), 101);
  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 99);
}

TEST_F (RepairTests, MultipleBlocks)
{
  characters.GetById (200)->MutableHP ().set_armour (100);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));

  auto c = characters.GetById (200);
  ASSERT_TRUE (c->IsBusy ());
  auto op = ongoings.GetById (c->GetProto ().ongoing ());
  EXPECT_EQ (op->GetHeight (), 109);
  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 10);
}

TEST_F (RepairTests, AlreadyRepairing)
{
  ASSERT_TRUE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));

  auto c = characters.GetById (200);
  ASSERT_TRUE (c->IsBusy ());
  auto op = ongoings.GetById (c->GetProto ().ongoing ());
  EXPECT_EQ (op->GetHeight (), 101);

  EXPECT_FALSE (Process ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"));
}

TEST_F (RepairTests, PendingJson)
{
  EXPECT_TRUE (PartialJsonEqual (GetPendingJson ("domob", R"({
    "t": "fix",
    "b": 100,
    "c": 200
  })"), ParseJson (R"({
    "type": "armourrepair",
    "character": 200
  })")));
}

/* ************************************************************************** */

class RevEngTests : public ServicesTests
{

protected:

  RevEngTests ()
  {
    /* Regtest has better chances for reverse engineering (starting at 100%),
       so that is more suitable for testing.  */
    ctx.SetChain (xaya::Chain::REGTEST);

    inv.Get (ANCIENT_BUILDING, "domob")
        ->GetInventory ().AddFungibleCount ("test artefact", 3);
  }

};

TEST_F (RevEngTests, InvalidFormat)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": 1,
    "x": false
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": 42,
    "n": 1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": -1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": "x"
  })"));
}

TEST_F (RevEngTests, InvalidItemType)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "invalid item",
    "n": 1
  })"));
}

TEST_F (RevEngTests, ItemNotAnArtefact)
{
  inv.Get (ANCIENT_BUILDING, "domob")
      ->GetInventory ().AddFungibleCount ("foo", 10);

  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "foo",
    "n": 1
  })"));
}

TEST_F (RevEngTests, InvalidAmount)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": -3
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": 0
  })"));
}

TEST_F (RevEngTests, TooMuch)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": 30
  })"));
}

TEST_F (RevEngTests, OneItem)
{
  ASSERT_TRUE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": 1
  })"));

  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 90);
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("test artefact"), 2);
  const auto bow = i->GetInventory ().GetFungibleCount ("bow bpo");
  const auto sword = i->GetInventory ().GetFungibleCount ("sword bpo");
  EXPECT_EQ (bow + sword, 1);
  EXPECT_EQ (itemCounts.GetFound ("bow bpo"), bow);
  EXPECT_EQ (itemCounts.GetFound ("sword bpo"), sword);
}

TEST_F (RevEngTests, ManyTries)
{
  constexpr unsigned bowOffset = 10;

  accounts.GetByName ("domob")->AddBalance (1'000'000);
  inv.Get (ANCIENT_BUILDING, "domob")
      ->GetInventory ().AddFungibleCount ("test artefact", 1'000);
  for (unsigned i = 0; i < bowOffset; ++i)
    itemCounts.IncrementFound ("bow bpo");

  ASSERT_TRUE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": 1000
  })"));

  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  const auto bow = i->GetInventory ().GetFungibleCount ("bow bpo");
  const auto sword = i->GetInventory ().GetFungibleCount ("sword bpo");
  LOG (INFO) << "Found " << bow << " bows and " << sword << " swords";
  EXPECT_GT (bow, 0);
  EXPECT_GT (sword, bow);
  EXPECT_EQ (itemCounts.GetFound ("bow bpo"), bow + bowOffset);
  EXPECT_EQ (itemCounts.GetFound ("sword bpo"), sword);
}

TEST_F (RevEngTests, PendingJson)
{
  EXPECT_TRUE (PartialJsonEqual (GetPendingJson ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "test artefact",
    "n": 2
  })"), ParseJson (R"({
    "type": "reveng",
    "input": {"test artefact": 2}
  })")));
}

/* ************************************************************************** */

class BlueprintCopyTests : public ServicesTests
{

protected:

  BlueprintCopyTests ()
  {
    accounts.GetByName ("domob")->AddBalance (999'900);
    CHECK_EQ (accounts.GetByName ("domob")->GetBalance (), 1'000'000);
    inv.Get (ANCIENT_BUILDING, "domob")
        ->GetInventory ().AddFungibleCount ("sword bpo", 1);

    ctx.SetHeight (100);
  }

};

TEST_F (BlueprintCopyTests, InvalidFormat)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword bpo",
    "n": 1,
    "x": false
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": 42,
    "n": 1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword bpo",
    "n": -1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "rve",
    "b": 100,
    "i": "sword bpo",
    "n": "x"
  })"));
}

TEST_F (BlueprintCopyTests, InvalidItemType)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "invalid item",
    "n": 1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword",
    "n": 1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword bpc",
    "n": 1
  })"));
}

TEST_F (BlueprintCopyTests, InvalidAmount)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword bpo",
    "n": -3
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword bpo",
    "n": 0
  })"));
}

TEST_F (BlueprintCopyTests, NotOwned)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "bow bpo",
    "n": 1
  })"));
}

TEST_F (BlueprintCopyTests, Success)
{
  db.SetNextId (100);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword bpo",
    "n": 10
  })"));

  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 999'000);
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("sword bpo"), 0);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("sword bpc"), 0);

  auto op = ongoings.GetById (100);
  ASSERT_NE (op, nullptr);
  EXPECT_EQ (op->GetHeight (), 100 + 10 * 10);
  EXPECT_EQ (op->GetBuildingId (), ANCIENT_BUILDING);
  ASSERT_TRUE (op->GetProto ().has_blueprint_copy ());
  const auto& cp = op->GetProto ().blueprint_copy ();
  EXPECT_EQ (cp.account (), "domob");
  EXPECT_EQ (cp.original_type (), "sword bpo");
  EXPECT_EQ (cp.copy_type (), "sword bpc");
  EXPECT_EQ (cp.num_copies (), 10);
}

TEST_F (BlueprintCopyTests, PendingJson)
{
  EXPECT_TRUE (PartialJsonEqual (GetPendingJson ("domob", R"({
    "t": "cp",
    "b": 100,
    "i": "sword bpo",
    "n": 2
  })"), ParseJson (R"({
    "type": "bpcopy",
    "original": "sword bpo",
    "output": {"sword bpc": 2}
  })")));
}

/* ************************************************************************** */

class ConstructionTests : public ServicesTests
{

protected:

  ConstructionTests ()
  {
    accounts.GetByName ("domob")->AddBalance (999'900);
    CHECK_EQ (accounts.GetByName ("domob")->GetBalance (), 1'000'000);

    auto i = inv.Get (ANCIENT_BUILDING, "domob");
    i->GetInventory ().AddFungibleCount ("sword bpo", 1);
    i->GetInventory ().AddFungibleCount ("sword bpc", 1);
    i->GetInventory ().AddFungibleCount ("zerospace", 100);

    ctx.SetHeight (100);
  }

};

TEST_F (ConstructionTests, InvalidFormat)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpo",
    "n": 1,
    "x": false
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": 42,
    "n": 1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpo",
    "n": -1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpo",
    "n": "x"
  })"));
}

TEST_F (ConstructionTests, InvalidItemType)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "invalid item",
    "n": 1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword",
    "n": 1
  })"));
}

TEST_F (ConstructionTests, InvalidAmount)
{
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpo",
    "n": -3
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpo",
    "n": 0
  })"));
}

TEST_F (ConstructionTests, MissingResources)
{
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  i->GetInventory ().AddFungibleCount ("bow bpo", 1);
  i->GetInventory ().AddFungibleCount ("foo", 100);
  i->GetInventory ().AddFungibleCount ("bar", 2);
  i.reset ();

  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "bow bpo",
    "n": 3
  })"));
}

TEST_F (ConstructionTests, MissingBlueprints)
{
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  i->GetInventory ().AddFungibleCount ("bow bpc", 1);
  i->GetInventory ().AddFungibleCount ("foo", 100);
  i->GetInventory ().AddFungibleCount ("bar", 200);
  i.reset ();

  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "bow bpo",
    "n": 1
  })"));
  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "bow bpc",
    "n": 2
  })"));
}

TEST_F (ConstructionTests, RequiredServiceType)
{
  inv.Get (ANCIENT_BUILDING, "domob")
      ->GetInventory ().AddFungibleCount ("chariot bpo", 1);

  db.SetNextId (201);
  buildings.CreateNew ("itemmaker", "", Faction::ANCIENT);
  buildings.CreateNew ("carmaker", "", Faction::ANCIENT);

  for (const Database::IdT id : {201, 202})
    {
      auto i = inv.Get (id, "domob");
      i->GetInventory ().AddFungibleCount ("sword bpo", 1);
      i->GetInventory ().AddFungibleCount ("chariot bpo", 1);
      i->GetInventory ().AddFungibleCount ("zerospace", 10);
    }

  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 201,
    "i": "chariot bpo",
    "n": 1
  })"));
  EXPECT_TRUE (Process ("domob", R"({
    "t": "bld",
    "b": 201,
    "i": "sword bpo",
    "n": 1
  })"));

  EXPECT_FALSE (Process ("domob", R"({
    "t": "bld",
    "b": 202,
    "i": "sword bpo",
    "n": 1
  })"));
  EXPECT_TRUE (Process ("domob", R"({
    "t": "bld",
    "b": 202,
    "i": "chariot bpo",
    "n": 1
  })"));
}

TEST_F (ConstructionTests, FromOriginal)
{
  db.SetNextId (100);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpo",
    "n": 5
  })"));

  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 999'500);
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("sword bpo"), 0);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("sword bpc"), 1);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("zerospace"), 50);

  auto op = ongoings.GetById (100);
  ASSERT_NE (op, nullptr);
  EXPECT_EQ (op->GetHeight (), 100 + 5 * 10);
  EXPECT_EQ (op->GetBuildingId (), ANCIENT_BUILDING);
  ASSERT_TRUE (op->GetProto ().has_item_construction ());
  const auto& c = op->GetProto ().item_construction ();
  EXPECT_EQ (c.account (), "domob");
  EXPECT_EQ (c.output_type (), "sword");
  EXPECT_EQ (c.num_items (), 5);
  EXPECT_EQ (c.original_type (), "sword bpo");
}

TEST_F (ConstructionTests, FromCopy)
{
  inv.Get (ANCIENT_BUILDING, "domob")
      ->GetInventory ().AddFungibleCount ("sword bpc", 4);
  db.SetNextId (100);
  ASSERT_TRUE (Process ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpc",
    "n": 5
  })"));

  EXPECT_EQ (accounts.GetByName ("domob")->GetBalance (), 999'500);
  auto i = inv.Get (ANCIENT_BUILDING, "domob");
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("sword bpo"), 1);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("sword bpc"), 0);
  EXPECT_EQ (i->GetInventory ().GetFungibleCount ("zerospace"), 50);

  auto op = ongoings.GetById (100);
  ASSERT_NE (op, nullptr);
  EXPECT_EQ (op->GetHeight (), 100 + 10);
  EXPECT_EQ (op->GetBuildingId (), ANCIENT_BUILDING);
  ASSERT_TRUE (op->GetProto ().has_item_construction ());
  const auto& c = op->GetProto ().item_construction ();
  EXPECT_EQ (c.account (), "domob");
  EXPECT_EQ (c.output_type (), "sword");
  EXPECT_EQ (c.num_items (), 5);
  EXPECT_FALSE (c.has_original_type ());
}

TEST_F (ConstructionTests, PendingJson)
{
  EXPECT_TRUE (PartialJsonEqual (GetPendingJson ("domob", R"({
    "t": "bld",
    "b": 100,
    "i": "sword bpo",
    "n": 2
  })"), ParseJson (R"({
    "type": "construct",
    "blueprint": "sword bpo",
    "output": {"sword": 2}
  })")));
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd
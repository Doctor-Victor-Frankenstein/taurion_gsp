/*
    GSP for the Taurion blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

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

#include "dynobstacles.hpp"

#include "buildings.hpp"

#include "database/character.hpp"

namespace pxd
{

DynObstacles::DynObstacles (const xaya::Chain c)
  : chain(c), red(false), green(false), blue(false), buildings(false)
{}

DynObstacles::DynObstacles (Database& db, const Context& ctx)
  : chain(ctx.Chain ()),
    red(false), green(false), blue(false), buildings(false)
{
  {
    CharacterTable tbl(db);
    tbl.ProcessAllPositions ([this] (const Database::IdT id,
                                     const HexCoord& pos, const Faction f)
      {
        AddVehicle (pos, f);
      });
  }

  {
    BuildingsTable tbl(db);
    auto res = tbl.QueryAll ();
    while (res.Step ())
      AddBuilding (*tbl.GetFromResult (res));
  }
}

void
DynObstacles::AddBuilding (const std::string& type,
                           const proto::ShapeTransformation& trafo,
                           const HexCoord& pos)
{
  for (const auto& c : GetBuildingShape (type, trafo, pos, chain))
    {
      auto ref = buildings.Access (c);
      CHECK (!ref);
      ref = true;
    }
}

void
DynObstacles::AddBuilding (const Building& b)
{
  AddBuilding (b.GetType (), b.GetProto ().shape_trafo (), b.GetCentre ());
}

} // namespace pxd

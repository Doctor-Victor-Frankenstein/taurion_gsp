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

#include "coord.hpp"

#include <gtest/gtest.h>

#include <set>
#include <sstream>
#include <unordered_set>

namespace pxd
{
namespace
{

using CoordTests = testing::Test;

TEST_F (CoordTests, Equality)
{
  const HexCoord a(2, -5);
  const HexCoord aa(2, -5);
  const HexCoord b(-1, -2);

  EXPECT_EQ (a, a);
  EXPECT_EQ (a, aa);
  EXPECT_NE (a, b);
}

TEST_F (CoordTests, LessThan)
{
  const HexCoord a(0, 0);
  const HexCoord b(0, 1);
  const HexCoord c(1, 0);

  EXPECT_LT (a, b);
  EXPECT_LT (a, c);
  EXPECT_LT (b, c);

  EXPECT_FALSE (a < a);
  EXPECT_FALSE (c < b);
}

TEST_F (CoordTests, Arithmetic)
{
  HexCoord test(-2, 5);
  EXPECT_EQ (2 * test, HexCoord (-4, 10));
  EXPECT_EQ (0 * test, HexCoord (0, 0));
  EXPECT_EQ (-1 * test, HexCoord (2, -5));

  test += HexCoord (5, -5);
  EXPECT_EQ (test, HexCoord (3, 0));

  EXPECT_EQ (HexCoord (1, 2) + HexCoord (-5, 3), HexCoord (-4, 5));
}

TEST_F (CoordTests, Rotation)
{
  EXPECT_EQ (HexCoord (1, 2).RotateCW (0), HexCoord (1, 2));
  EXPECT_EQ (HexCoord (1, 2).RotateCW (1), HexCoord (3, -1));
  EXPECT_EQ (HexCoord (1, 2).RotateCW (2), HexCoord (2, -3));
  EXPECT_EQ (HexCoord (1, 2).RotateCW (3), HexCoord (-1, -2));
  EXPECT_EQ (HexCoord (1, 2).RotateCW (4), HexCoord (-3, 1));
  EXPECT_EQ (HexCoord (1, 2).RotateCW (5), HexCoord (-2, 3));

  /* This is a chained rotation that will come out to zero, but
     verifies various cases other than the basic rotations.  */
  EXPECT_EQ (HexCoord (1, 2)
                .RotateCW (20)
                .RotateCW (-30)
                .RotateCW (1)
                .RotateCW (2)
                .RotateCW (3)
                .RotateCW (4),
             HexCoord (1, 2));
}

TEST_F (CoordTests, DistanceL1)
{
  const HexCoord a(-2, 1);
  const HexCoord b(3, -2);

  EXPECT_EQ (HexCoord::DistanceL1 (a, b), 5);
  EXPECT_EQ (HexCoord::DistanceL1 (b, a), 5);

  EXPECT_EQ (HexCoord::DistanceL1 (a, a), 0);
  EXPECT_EQ (HexCoord::DistanceL1 (b, b), 0);
}

TEST_F (CoordTests, Hashing)
{
  const HexCoord a(-5, 2);
  const HexCoord aa(-5, 2);
  const HexCoord b(5, -2);
  const HexCoord c(5, 2);

  std::hash<HexCoord> hasher;

  EXPECT_NE (hasher (a), hasher (b));
  EXPECT_NE (hasher (a), hasher (c));
  EXPECT_NE (hasher (b), hasher (c));

  EXPECT_EQ (hasher (a), hasher (aa));
}

TEST_F (CoordTests, UnorderedSet)
{
  const HexCoord a(-5, 2);
  const HexCoord aa(-5, 2);
  const HexCoord b(5, -2);
  const HexCoord c(5, 2);

  std::unordered_set<HexCoord> coords;

  coords.insert (a);
  coords.insert (b);
  EXPECT_EQ (coords.size (), 2);
  EXPECT_EQ (coords.count (aa), 1);
  EXPECT_EQ (coords.count (b), 1);
  EXPECT_EQ (coords.count (c), 0);

  coords.insert (aa);
  EXPECT_EQ (coords.size (), 2);
  EXPECT_EQ (coords.count (a), 1);
  EXPECT_EQ (coords.count (b), 1);
  EXPECT_EQ (coords.count (c), 0);

  coords.insert (c);
  EXPECT_EQ (coords.size (), 3);
  EXPECT_EQ (coords.count (a), 1);
  EXPECT_EQ (coords.count (b), 1);
  EXPECT_EQ (coords.count (c), 1);
}

TEST_F (CoordTests, Neighbours)
{
  const HexCoord centre(-2, 1);

  std::set<HexCoord> neighbours;
  for (const auto& n : centre.Neighbours ())
    {
      EXPECT_EQ (neighbours.count (n), 0);
      neighbours.insert (n);
    }

  EXPECT_EQ (neighbours.size (), 6);
  for (const auto& n : {HexCoord (-3, 1), HexCoord (-2, 0), HexCoord (-1, 0),
                        HexCoord (-1, 1), HexCoord (-2, 2), HexCoord (-3, 2)})
    EXPECT_EQ (neighbours.count (n), 1);

  for (const auto& n : neighbours)
    EXPECT_EQ (HexCoord::DistanceL1 (centre, n), 1);
}

TEST_F (CoordTests, IsPrincipalDirectionTo)
{
  constexpr HexCoord base(42, -10);

  constexpr HexCoord nonPrincipal[] =
    {
      HexCoord (1, 1),
      HexCoord (-1, -1),
      HexCoord (2, 3),
      HexCoord (-5, -5),
      HexCoord (3, 10),
      HexCoord (0, 0),
      base + HexCoord (1, 0),
    };
  for (const auto& dir : nonPrincipal)
    {
      HexCoord d;
      HexCoord::IntT steps;
      ASSERT_FALSE (base.IsPrincipalDirectionTo (base + dir, d, steps));
    }

  constexpr HexCoord isPrincipal[] =
    {
      HexCoord (-1, 0),
      HexCoord (1, 0),
      HexCoord (0, -1),
      HexCoord (0, 1),
      HexCoord (-1, 1),
      HexCoord (1, -1),
      HexCoord (10, -10),
      HexCoord (0, 42),
      HexCoord (100, 0),
    };
  for (const auto& dir : isPrincipal)
    {
      HexCoord d;
      HexCoord::IntT steps;
      ASSERT_TRUE (base.IsPrincipalDirectionTo (base + dir, d, steps));
      ASSERT_EQ (steps * d, dir);
      ASSERT_EQ (HexCoord::DistanceL1 (HexCoord (), d), 1);
    }
}

TEST_F (CoordTests, StreamOutput)
{
  std::ostringstream out;
  out << HexCoord (-5, 42);
  EXPECT_EQ (out.str (), "(-5, 42)");
}

} // anonymous namespace
} // namespace pxd

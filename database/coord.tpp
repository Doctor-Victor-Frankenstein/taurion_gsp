/*
    GSP for the Taurion blockchain game
    Copyright (C) 2019  Autonomous Worlds Ltd

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

/* Template implementation code for coord.hpp.  */

#include <type_traits>

namespace pxd
{

template <typename T>
  HexCoord
  GetCoordFromColumn (const Database::Result<T>& res)
{
  static_assert (std::is_base_of<ResultWithCoord, T>::value,
                 "GetCoordFromColumn needs a ResultWithCoord");

  const auto x = res.template Get<typename T::x> ();
  const auto y = res.template Get<typename T::y> ();

  return HexCoord (x, y);
}

} // namespace pxd

#ifndef PXD_JSONUTILS_HPP
#define PXD_JSONUTILS_HPP

#include "hexagonal/coord.hpp"

#include <json/json.h>

namespace pxd
{

/**
 * Encodes a HexCoord object into a JSON object, so that it can be returned
 * from the JSON-RPC interface.
 *
 * The format is: {"x": x-coord, "y": y-coord}
 */
Json::Value CoordToJson (const HexCoord& c);

/**
 * Parses a JSON object (e.g. passed by RPC) into a HexCoord.  Returns false
 * if the format isn't right, e.g. the values are out of range for
 * HexCoord::IntT or the object is missing keys.
 */
bool CoordFromJson (const Json::Value& val, HexCoord& c);

} // namespace pxd

#endif // PXD_JSONUTILS_HPP
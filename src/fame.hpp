#ifndef PXD_FAME_HPP
#define PXD_FAME_HPP

#include "database/account.hpp"
#include "database/character.hpp"
#include "database/damagelists.hpp"
#include "database/database.hpp"
#include "proto/combat.pb.h"

#include <map>
#include <string>

namespace pxd
{

/**
 * The main class handling computation and updates of fame.  This is notified
 * about kills by the combat logic, and then updates the fame accordingly.
 *
 * The actual fame update takes place in a virtual subroutine, so that this
 * can be mocked out for testing (or tested separately).
 */
class FameUpdater
{

private:

  /** A DamageLists instance we use for the updates during computation.  */
  DamageLists dl;

  /** Character table used for looking up owners.  */
  CharacterTable characters;

  /** Accounts table for updating fame and kills.  */
  AccountsTable accounts;

  /**
   * For accounts that had their fame updated, we keep the "original" fame
   * value in this cache.  This allows us do the whole update computation
   * on a single, consistent, frozen set of fame values, rather than have
   * it depend on the exact order of changes.
   */
  std::map<std::string, unsigned> originalFame;

  /**
   * Retrieves the original fame value of the given account.  This tries to look
   * it up in originalFame.  If not found there, we use the current value
   * from the Account instance and also store it in the cache.
   */
  unsigned GetOriginalFame (const Account& a);

  /**
   * Computes the "fame level" of a player.  This is used to determine who
   * gets fame (namely those max one level above/below).  This is an int so
   * that we can safely compute differences.
   */
  static int GetLevel (unsigned fame);

  friend class FameLevelTests;
  friend class FameTests;

protected:

  /**
   * Updates fame accordingly for the given kill.  This is the main internal
   * routine handling fame computation, which holds the actual logic.
   *
   * It is virtual so that it can be mocked for testing.
   */
  virtual void UpdateForKill (Database::IdT victim,
                              const DamageLists::Attackers& attackers);

public:

  explicit FameUpdater (Database& db, const unsigned height)
    : dl(db, height), characters(db), accounts(db)
  {}

  virtual ~FameUpdater () = default;

  FameUpdater () = delete;
  FameUpdater (const FameUpdater&) = delete;
  void operator= (const FameUpdater&) = delete;

  /**
   * Returns a DamageLists instance for the current block.
   */
  DamageLists&
  GetDamageLists ()
  {
    return dl;
  }

  /**
   * Updates fame when the given fighter target has been killed.
   */
  void UpdateForKill (const proto::TargetId& target);

};

} // namespace pxd

#endif // PXD_FAME_HPP
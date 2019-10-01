#!/usr/bin/env python
# coding=utf8

#   GSP for the Taurion blockchain game
#   Copyright (C) 2019  Autonomous Worlds Ltd
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

"""
Tests creation / initialisation of accounts.
"""

from pxtest import PXTest, CHARACTER_COST


class AccountsTest (PXTest):

  def run (self):
    self.collectPremine ()

    self.mainLogger.info ("No accounts yet...")
    accounts = self.getAccounts ()
    self.assertEqual (accounts, {})

    self.mainLogger.info ("Initialising basic accounts...")
    self.initAccount ("domob", "r")
    self.initAccount ("domob", "g")
    self.initAccount ("andy", "b")
    self.generate (1)

    accounts = self.getAccounts ()
    self.assertEqual (accounts["domob"].getFaction (), "r")
    self.assertEqual (accounts["andy"].getFaction (), "b")


if __name__ == "__main__":
  AccountsTest ().main ()
/*
    This file is part of ION Blockchain Library.

    ION Blockchain Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    ION Blockchain Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with ION Blockchain Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2017-2020 Telegram Systems LLP
*/
#pragma once

#include "full-node.h"
#include "validator/interfaces/block-handle.h"

namespace ion {

namespace validator {

namespace fullnode {

class FullNodeMaster : public td::actor::Actor {
 public:
  virtual ~FullNodeMaster() = default;

  static td::actor::ActorOwn<FullNodeMaster> create(adnl::AdnlNodeIdShort adnl_id, td::uint16 port,
                                                    FileHash zero_state_file_hash,
                                                    td::actor::ActorId<keyring::Keyring> keyring,
                                                    td::actor::ActorId<adnl::Adnl> adnl,
                                                    td::actor::ActorId<ValidatorManagerInterface> validator_manager);
};

}  // namespace fullnode

}  // namespace validator

}  // namespace ion

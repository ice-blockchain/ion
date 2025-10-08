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

#include "ion/ion-types.h"
#include "crypto/common/refcnt.hpp"
#include "validator-set.h"
#include "crypto/block/mc-config.h"

namespace ion {

namespace validator {

using McShardHash = block::McShardHashI;

class ConfigHolder : public td::CntObject {
 public:
  virtual ~ConfigHolder() = default;

  virtual td::Ref<ValidatorSet> get_total_validator_set(int next) const = 0;  // next = -1 -> prev, next = 0 -> cur
  virtual td::Ref<ValidatorSet> get_validator_set(ShardIdFull shard, UnixTime utime, CatchainSeqno seqno) const = 0;
  virtual std::pair<UnixTime, UnixTime> get_validator_set_start_stop(int next) const = 0;
};

}  // namespace validator

}  // namespace ion

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
*/
#pragma once

#include "keys/encryptor.h"
#include "ion/ion-types.h"

namespace block {

class Config;
struct TotalValidatorSet;

class ValidatorSet : public td::CntObject {
 public:
  const ion::ValidatorDescr* get_validator(const ion::NodeIdShort& id) const;
  bool is_validator(ion::NodeIdShort id) const;
  ion::CatchainSeqno get_catchain_seqno() const {
    return cc_seqno_;
  }
  td::uint32 get_validator_set_hash() const {
    return hash_;
  }
  ion::ShardIdFull get_shard() const {
    return for_;
  }
  ion::ValidatorWeight get_total_weight() const {
    return total_weight_;
  }
  std::vector<ion::ValidatorDescr> export_vector() const;
  ValidatorSet* make_copy() const override;
  ValidatorSet(ion::CatchainSeqno cc_seqno, ion::ShardIdFull from, std::vector<ion::ValidatorDescr> nodes);

 private:
  ion::CatchainSeqno cc_seqno_;
  ion::ShardIdFull for_;
  td::uint32 hash_;
  ion::ValidatorWeight total_weight_;
  std::vector<ion::ValidatorDescr> ids_;
  std::vector<std::pair<ion::NodeIdShort, size_t>> ids_map_;
};

class ValidatorSetCompute {
 public:
  td::Ref<ValidatorSet> get_validator_set(ion::ShardIdFull shard, ion::UnixTime utime, ion::CatchainSeqno cc) const;
  td::Ref<ValidatorSet> get_next_validator_set(ion::ShardIdFull shard, ion::UnixTime utime,
                                               ion::CatchainSeqno cc) const;
  td::Status init(const Config* config);
  ValidatorSetCompute() = default;

 private:
  const Config* config_{nullptr};
  std::shared_ptr<TotalValidatorSet> cur_validators_, next_validators_;
  td::Ref<ValidatorSet> compute_validator_set(ion::ShardIdFull shard, const TotalValidatorSet& vset, ion::UnixTime time,
                                              ion::CatchainSeqno cc_seqno) const;
};

}  // namespace block

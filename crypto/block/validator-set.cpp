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
#include <keys/keys.hpp>

#include "auto/tl/ton_api.h"
#include "block/block.h"

#include "mc-config.h"
#include "validator-set.h"

namespace block {
using td::Ref;

const ion::ValidatorDescr *ValidatorSet::get_validator(const ion::NodeIdShort &id) const {
  auto it =
      std::lower_bound(ids_map_.begin(), ids_map_.end(), id, [](const auto &p, const auto &x) { return p.first < x; });
  return it < ids_map_.end() && it->first == id ? &ids_[it->second] : nullptr;
}

bool ValidatorSet::is_validator(ion::NodeIdShort id) const {
  return get_validator(id);
}

ValidatorSet::ValidatorSet(ion::CatchainSeqno cc_seqno, ion::ShardIdFull from, std::vector<ion::ValidatorDescr> nodes)
    : cc_seqno_(cc_seqno), for_(from), ids_(std::move(nodes)) {
  total_weight_ = 0;

  ids_map_.reserve(ids_.size());

  for (std::size_t i = 0; i < ids_.size(); i++) {
    total_weight_ += ids_[i].weight;
    ids_map_.emplace_back(ion::PublicKey{ion::pubkeys::Ed25519{ids_[i].key}}.compute_short_id().bits256_value(), i);
  }

  std::sort(ids_map_.begin(), ids_map_.end());
  for (std::size_t i = 1; i < ids_map_.size(); i++) {
    CHECK(ids_map_[i - 1].first != ids_map_[i].first);
  }

  hash_ = compute_validator_set_hash(cc_seqno, from, ids_);
}

ValidatorSet *ValidatorSet::make_copy() const {
  return new ValidatorSet{*this};
}

std::vector<ion::ValidatorDescr> ValidatorSet::export_vector() const {
  return ids_;
}

td::Status ValidatorSetCompute::init(const Config *config) {
  config_ = nullptr;
  cur_validators_.reset();
  next_validators_.reset();
  if (!config) {
    return td::Status::Error("null configuration pointer passed to ValidatorSetCompute");
  }
  config_ = config;
  auto cv_root = config_->get_config_param(34);
  if (cv_root.not_null()) {
    TRY_RESULT(validators, block::Config::unpack_validator_set(std::move(cv_root)));
    cur_validators_ = std::move(validators);
  }
  auto nv_root = config_->get_config_param(36);
  if (nv_root.not_null()) {
    TRY_RESULT(validators, block::Config::unpack_validator_set(std::move(nv_root)));
    next_validators_ = std::move(validators);
  }
  return td::Status::OK();
}

Ref<ValidatorSet> ValidatorSetCompute::compute_validator_set(ion::ShardIdFull shard, const TotalValidatorSet &vset,
                                                             ion::UnixTime time, ion::CatchainSeqno cc_seqno) const {
  if (!config_) {
    return {};
  }
  LOG(DEBUG) << "in compute_validator_set() for " << shard.to_str();
  auto nodes = config_->compute_validator_set(shard, vset, time, cc_seqno);
  if (nodes.empty()) {
    LOG(ERROR) << "compute_validator_set() for " << shard.to_str() << "," << time << "," << cc_seqno
               << " returned empty list";
    return {};
  }
  return Ref<ValidatorSet>{true, cc_seqno, shard, std::move(nodes)};
}

Ref<ValidatorSet> ValidatorSetCompute::get_validator_set(ion::ShardIdFull shard, ion::UnixTime utime,
                                                         ion::CatchainSeqno cc) const {
  if (!config_ || !cur_validators_) {
    LOG(ERROR) << "ValidatorSetCompute::get_validator_set() : no config or no cur_validators";
    return {};
  }
  return compute_validator_set(shard, *cur_validators_, utime, cc);
}

Ref<ValidatorSet> ValidatorSetCompute::get_next_validator_set(ion::ShardIdFull shard, ion::UnixTime utime,
                                                              ion::CatchainSeqno cc) const {
  if (!config_ || !cur_validators_) {
    LOG(ERROR) << "ValidatorSetCompute::get_next_validator_set() : no config or no cur_validators";
    return {};
  }
  if (!next_validators_) {
    return compute_validator_set(shard, *cur_validators_, utime, cc + 1);
  }
  bool is_mc = shard.is_masterchain();
  auto ccv_cfg = config_->get_catchain_validators_config();
  unsigned cc_lifetime = is_mc ? ccv_cfg.mc_cc_lifetime : ccv_cfg.shard_cc_lifetime;
  if (next_validators_->utime_since > (utime / cc_lifetime + 1) * cc_lifetime) {
    return compute_validator_set(shard, *cur_validators_, utime, cc + 1);
  } else {
    return compute_validator_set(shard, *next_validators_, utime, cc + 1);
  }
}

}  // namespace block

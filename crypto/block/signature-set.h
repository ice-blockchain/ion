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

#include "auto/tl/lite_api.hpp"
#include "common/errorcode.h"
#include "crypto/common/refcnt.hpp"
#include "ion/ion-types.h"
#include "vm/cells.h"

#include "validator-set.h"

namespace block {

class BlockSignatureSet : public td::CntObject {
 public:
  virtual size_t get_size() const = 0;
  virtual td::Result<ion::ValidatorWeight> get_weight(td::Ref<ValidatorSet> vset) const = 0;
  virtual bool is_ordinary() const {
    return false;
  }
  virtual bool is_final() const = 0;

  td::Result<ion::ValidatorWeight> check_signatures(td::Ref<ValidatorSet> vset, ion::BlockIdExt block_id) const {
    if (!is_final()) {
      return td::Status::Error(ion::ErrorCode::protoviolation, "not final signatures");
    }
    return check_signatures_impl(std::move(vset), block_id);
  }
  td::Result<ion::ValidatorWeight> check_approve_signatures(td::Ref<ValidatorSet> vset,
                                                            ion::BlockIdExt block_id) const {
    if (is_final()) {
      return td::Status::Error(ion::ErrorCode::protoviolation, "not approve signatures");
    }
    return check_signatures_impl(std::move(vset), block_id);
  }

  virtual td::Result<td::Ref<vm::Cell>> serialize(td::Ref<ValidatorSet> vset) const = 0;
  virtual ion::tl_object_ptr<ion::ton_api::tonNode_SignatureSet> tl() const = 0;
  virtual ion::tl_object_ptr<ion::lite_api::liteServer_SignatureSet> tl_lite() const = 0;

  // ordinary signature set only (is_ordinary())
  virtual std::vector<ion::tl_object_ptr<ion::ton_api::tonNode_blockSignature>> tl_legacy() const {
    UNREACHABLE();
  }

  BlockSignatureSet(ion::CatchainSeqno cc_seqno, td::uint32 validator_set_hash)
      : cc_seqno_(cc_seqno), validator_set_hash_(validator_set_hash) {
  }
  ion::CatchainSeqno get_catchain_seqno() const {
    return cc_seqno_;
  }
  td::uint32 get_validator_set_hash() const {
    return validator_set_hash_;
  }

 protected:
  ion::CatchainSeqno cc_seqno_;
  td::uint32 validator_set_hash_;

  virtual td::Result<ion::ValidatorWeight> check_signatures_impl(td::Ref<ValidatorSet> vset,
                                                                 ion::BlockIdExt block_id) const = 0;

 public:
  static td::Ref<BlockSignatureSet> create_ordinary(std::vector<ion::BlockSignature> signatures,
                                                    ion::CatchainSeqno cc_seqno, td::uint32 validator_set_hash);
  static td::Ref<BlockSignatureSet> create_simplex(
      std::vector<ion::BlockSignature> signatures, ion::CatchainSeqno cc_seqno, td::uint32 validator_set_hash,
      td::Bits256 session_id, td::uint32 slot, ion::tl_object_ptr<ion::ton_api::consensus_CandidateHashData> candidate);
  static td::Ref<BlockSignatureSet> create_simplex_approve(
      std::vector<ion::BlockSignature> signatures, ion::CatchainSeqno cc_seqno, td::uint32 validator_set_hash,
      td::Bits256 session_id, td::uint32 slot, ion::tl_object_ptr<ion::ton_api::consensus_CandidateHashData> candidate);

  static td::Result<td::Ref<BlockSignatureSet>> fetch(td::Ref<vm::Cell> cell, ion::ValidatorWeight& total_weight);
  static td::Result<td::Ref<BlockSignatureSet>> fetch(td::Ref<vm::Cell> cell, td::Ref<ValidatorSet> vset);
  static td::Ref<BlockSignatureSet> fetch(
      const std::vector<ion::tl_object_ptr<ion::ton_api::tonNode_blockSignature>>& f, ion::CatchainSeqno cc_seqno,
      td::uint32 validator_set_hash);
  static td::Ref<BlockSignatureSet> fetch(const ion::tl_object_ptr<ion::ton_api::tonNode_SignatureSet>& f);
  static td::Result<td::Ref<BlockSignatureSet>> fetch(
      const ion::tl_object_ptr<ion::lite_api::liteServer_SignatureSet>& f);

  static constexpr size_t MAX_SIGNATURES = 1024;
};

}  // namespace block

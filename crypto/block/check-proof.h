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

#include "vm/cells.h"
#include "block/block.h"

namespace block {
using td::Ref;

td::Status check_block_header_proof(td::Ref<vm::Cell> root, ion::BlockIdExt blkid,
                                    ion::Bits256* store_state_hash_to = nullptr, bool check_state_hash = false,
                                    td::uint32* save_utime = nullptr, ion::LogicalTime* save_lt = nullptr);
td::Status check_shard_proof(ion::BlockIdExt blk, ion::BlockIdExt shard_blk, td::Slice shard_proof);
td::Status check_account_proof(td::Slice proof, ion::BlockIdExt shard_blk, const block::StdAddress& addr,
                               td::Ref<vm::Cell> root, ion::LogicalTime* last_trans_lt = nullptr,
                               ion::Bits256* last_trans_hash = nullptr, td::uint32* save_utime = nullptr,
                               ion::LogicalTime* save_lt = nullptr);
td::Result<td::Bits256> check_state_proof(ion::BlockIdExt blkid, td::Slice proof);
td::Result<Ref<vm::Cell>> check_extract_state_proof(ion::BlockIdExt blkid, td::Slice proof, td::Slice data);

td::Status check_block_signatures(const std::vector<ion::ValidatorDescr>& nodes,
                                  const std::vector<ion::BlockSignature>& signatures, const ion::BlockIdExt& blkid);

struct AccountState {
  ion::BlockIdExt blk;
  ion::BlockIdExt shard_blk;
  td::BufferSlice shard_proof;
  td::BufferSlice proof;
  td::BufferSlice state;
  bool is_virtualized{false};

  struct Info {
    td::Ref<vm::Cell> root, true_root;
    ion::LogicalTime last_trans_lt{0};
    ion::Bits256 last_trans_hash;
    ion::LogicalTime gen_lt{0};
    td::uint32 gen_utime{0};
  };

  td::Result<Info> validate(ion::BlockIdExt ref_blk, block::StdAddress addr) const;
};

struct Transaction {
  ion::BlockIdExt blkid;
  ion::LogicalTime lt;
  ion::Bits256 hash;
  td::Ref<vm::Cell> root;

  struct Info {
    ion::BlockIdExt blkid;
    td::uint32 now;
    ion::LogicalTime prev_trans_lt;
    ion::Bits256 prev_trans_hash;
    td::Ref<vm::Cell> transaction;
  };
  td::Result<Info> validate();
};

struct TransactionList {
  ion::LogicalTime lt;
  ion::Bits256 hash;
  std::vector<ion::BlockIdExt> blkids;
  td::BufferSlice transactions_boc;

  struct Info {
    ion::LogicalTime lt;
    ion::Bits256 hash;
    std::vector<Transaction::Info> transactions;
  };

  td::Result<Info> validate() const;
};

struct BlockTransaction {
  ion::BlockIdExt blkid;
  td::Ref<vm::Cell> root;
  td::Ref<vm::Cell> proof;

  struct Info {
    ion::BlockIdExt blkid;
    td::uint32 now;
    ion::LogicalTime lt;
    ion::Bits256 hash;
    td::Ref<vm::Cell> transaction;
  };
  td::Result<Info> validate(bool check_proof) const;
};

struct BlockTransactionList {
  ion::BlockIdExt blkid;
  td::BufferSlice transactions_boc;
  td::BufferSlice proof_boc;
  ion::LogicalTime start_lt;
  td::Bits256 start_addr;
  bool reverse_mode;
  int req_count;

  struct Info {
    ion::BlockIdExt blkid;
    std::vector<BlockTransaction::Info> transactions;
  };

  td::Result<Info> validate(bool check_proof) const;
};

}  // namespace block

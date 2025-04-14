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
#include "vm/cells.h"
#include "ion/ion-types.h"
#include "auto/tl/ion_api.h"
#include "interfaces/out-msg-queue-proof.h"
#include "td/actor/actor.h"
#include "interfaces/shard.h"
#include "validator.h"

namespace ion {

namespace validator {
using td::Ref;

class ValidatorManager;
class ValidatorManagerInterface;

class BuildOutMsgQueueProof : public td::actor::Actor {
 public:
  BuildOutMsgQueueProof(ShardIdFull dst_shard, std::vector<BlockIdExt> blocks, block::ImportedMsgQueueLimits limits,
                        td::actor::ActorId<ValidatorManagerInterface> manager,
                        td::Promise<tl_object_ptr<ion_api::tonNode_outMsgQueueProof>> promise)
      : dst_shard_(dst_shard), limits_(limits), manager_(manager), promise_(std::move(promise)) {
    blocks_.resize(blocks.size());
    for (size_t i = 0; i < blocks_.size(); ++i) {
      blocks_[i].id = blocks[i];
    }
  }

  void abort_query(td::Status reason);
  void start_up() override;
  void got_state_root(size_t i, Ref<vm::Cell> root);
  void got_block_root(size_t i, Ref<vm::Cell> root);
  void build_proof();

 private:
  ShardIdFull dst_shard_;
  std::vector<OutMsgQueueProof::OneBlock> blocks_;
  block::ImportedMsgQueueLimits limits_;

  td::actor::ActorId<ValidatorManagerInterface> manager_;
  td::Promise<tl_object_ptr<ion_api::tonNode_outMsgQueueProof>> promise_;

  size_t pending = 0;
};

}  // namespace validator
}  // namespace ion

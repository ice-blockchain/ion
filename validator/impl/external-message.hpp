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

#include "interfaces/validator-manager.h"
#include "validator/interfaces/external-message.h"
#include "auto/tl/ion_api.h"
#include "adnl/utils.hpp"
#include "block/transaction.h"

namespace ion {

namespace validator {

class ExtMessageQ : public ExtMessage {
  td::Ref<vm::Cell> root_;
  AccountIdPrefixFull addr_prefix_;
  td::BufferSlice data_;
  Hash hash_;
  ion::WorkchainId wc_;
  ion::StdSmcAddress addr_;

 public:
  AccountIdPrefixFull shard() const override {
    return addr_prefix_;
  }
  td::BufferSlice serialize() const override {
    return data_.clone();
  }
  td::Ref<vm::Cell> root_cell() const override {
    return root_;
  }
  Hash hash() const override {
    return hash_;
  }
  ion::WorkchainId wc() const override {
    return wc_;
  }

  ion::StdSmcAddress addr() const override {
    return addr_;
  }

  ExtMessageQ(td::BufferSlice data, td::Ref<vm::Cell> root, AccountIdPrefixFull shard, ion::WorkchainId wc,
              ion::StdSmcAddress addr);
  static td::Result<td::Ref<ExtMessageQ>> create_ext_message(td::BufferSlice data,
                                                             block::SizeLimitsConfig::ExtMsgLimits limits);
  static void run_message(td::Ref<ExtMessage> message, td::actor::ActorId<ion::validator::ValidatorManager> manager,
                          td::Promise<td::Ref<ExtMessage>> promise);
  static td::Status run_message_on_account(ion::WorkchainId wc,
                                           block::Account* acc,
                                           UnixTime utime, LogicalTime lt,
                                           td::Ref<vm::Cell> msg_root,
                                           std::unique_ptr<block::ConfigInfo> config);
};

}  // namespace validator

}  // namespace ion

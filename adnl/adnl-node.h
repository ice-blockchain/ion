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

#include "auto/tl/ion_api.h"
#include "adnl-node-id.hpp"
#include "adnl-address-list.h"

namespace ion {

namespace adnl {

class AdnlNode {
 private:
  AdnlNodeIdFull pub_;
  AdnlAddressList addr_list_;

 public:
  AdnlNode(AdnlNodeIdFull pub, AdnlAddressList addr_list) : pub_(std::move(pub)), addr_list_(std::move(addr_list)) {
  }
  AdnlNode(const AdnlNode& from) : pub_(from.pub_), addr_list_(from.addr_list_) {
  }
  static td::Result<AdnlNode> create(const tl_object_ptr<ion_api::adnl_node>& obj);

  tl_object_ptr<ion_api::adnl_node> tl() const {
    return create_tl_object<ion_api::adnl_node>(pub_.tl(), addr_list_.tl());
  }
  AdnlNodeIdFull pub_id() const {
    return pub_;
  }
  AdnlNodeIdShort compute_short_id() const {
    return pub_.compute_short_id();
  }
  const AdnlAddressList& addr_list() const {
    return addr_list_;
  }
};

class AdnlNodesList {
 private:
  std::vector<AdnlNode> nodes_;

 public:
  const auto& nodes() const {
    return nodes_;
  }
  AdnlNodesList() {
  }
  void push(AdnlNode node) {
    nodes_.push_back(std::move(node));
  }
  tl_object_ptr<ion_api::adnl_nodes> tl() const;
  static td::Result<AdnlNodesList> create(const tl_object_ptr<ion_api::adnl_nodes>& nodes);
};

}  // namespace adnl

}  // namespace ion

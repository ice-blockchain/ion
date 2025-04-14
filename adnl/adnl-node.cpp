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
#include "adnl-node.h"

namespace ion {

namespace adnl {

td::Result<AdnlNode> AdnlNode::create(const tl_object_ptr<ion_api::adnl_node> &obj) {
  TRY_RESULT(id, AdnlNodeIdFull::create(obj->id_));
  TRY_RESULT(addr_list, AdnlAddressList::create(std::move(obj->addr_list_)));
  return AdnlNode{std::move(id), std::move(addr_list)};
}

tl_object_ptr<ion_api::adnl_nodes> AdnlNodesList::tl() const {
  std::vector<tl_object_ptr<ion_api::adnl_node>> vec;
  for (auto &node : nodes_) {
    vec.emplace_back(node.tl());
  }
  return create_tl_object<ion_api::adnl_nodes>(std::move(vec));
}

td::Result<AdnlNodesList> AdnlNodesList::create(const tl_object_ptr<ion_api::adnl_nodes> &nodes) {
  AdnlNodesList res{};
  for (auto &node : nodes->nodes_) {
    TRY_RESULT(N, AdnlNode::create(node));
    res.push(std::move(N));
  }
  return res;
}

}  // namespace adnl

}  // namespace ion

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
#include "adnl/adnl-node-id.hpp"
#include "auto/tl/lite_api.h"
#include "td/utils/port/IPAddress.h"
#include "ion/ion-types.h"

namespace liteclient {

struct QueryInfo {
  enum Type { t_simple, t_seqno, t_utime, t_lt, t_mc_seqno };
  int query_id = 0;
  ion::ShardIdFull shard_id{ion::masterchainId};
  Type type = t_simple;
  td::uint64 value = 0;
  /* Query types and examples:
   * t_simple - query to the recent blocks in a shard, or general info. value = 0.
   *   getTime, getMasterchainInfo (shard_id = masterchain)
   *   sendMessage
   *   getAccountState, runSmcMethod - when no block is given
   * t_seqno - query to block with seqno in a shard. value = seqno.
   *   lookupBlock by seqno
   *   getBlock, getBlockHeader
   *   getAccountState, runSmcMethod  - when shard block is given
   * t_utime - query to a block with given unixtime in a shard. value = utime.
   *   lookupBlock by utime
   * t_lt - query to a block with given lt in a shard. value = lt.
   *   lookupBlock by lt
   *   getTransactions
   * t_mc_seqno - query to a block in a shard, masterchain seqno is given. value = mc_seqno.
   *   getAccountState, runSmcMethod - when mc block is given
   */

  std::string to_str() const;
};

QueryInfo get_query_info(td::Slice data);
QueryInfo get_query_info(const ion::lite_api::Function& f);

struct LiteServerConfig {
 private:
  struct ShardInfo {
    ion::ShardIdFull shard_id;
    ion::BlockSeqno seqno;
    ion::UnixTime utime;
    ion::LogicalTime lt;
  };

  struct Slice {
    std::vector<ShardInfo> shards_from, shards_to;
    bool unlimited = false;

    bool accepts_query(const QueryInfo& query_info) const;
  };

  bool is_full = false;
  std::vector<Slice> slices;

 public:
  ion::adnl::AdnlNodeIdFull adnl_id;
  std::string hostname;

  LiteServerConfig() = default;
  LiteServerConfig(ion::adnl::AdnlNodeIdFull adnl_id, std::string hostname)
      : is_full(true), adnl_id(adnl_id), hostname(std::move(hostname)) {
  }
  LiteServerConfig(ion::adnl::AdnlNodeIdFull adnl_id, td::IPAddress ip)
      : is_full(true), adnl_id(adnl_id), hostname(PSTRING() << ip.get_ip_str() << ":" << ip.get_port()) {
  }

  bool accepts_query(const QueryInfo& query_info) const;

  static td::Result<std::vector<LiteServerConfig>> parse_global_config(
      const ion::ton_api::liteclient_config_global& config);
};

}  // namespace liteclient

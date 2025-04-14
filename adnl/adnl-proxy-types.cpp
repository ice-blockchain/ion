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
#include "adnl-proxy-types.hpp"
#include "tl-utils/tl-utils.hpp"
#include "auto/tl/ion_api.hpp"
#include "td/utils/overloaded.h"
#include "td/utils/Time.h"
#include "common/errorcode.h"

namespace ion {

namespace adnl {

td::Result<AdnlProxy::Packet> AdnlProxyNone::decrypt(td::BufferSlice packet) const {
  if (packet.size() < 32) {
    return td::Status::Error(ErrorCode::protoviolation, "bad signature");
  }
  if (packet.as_slice().truncate(32) != id_.as_slice()) {
    return td::Status::Error(ErrorCode::protoviolation, "bad proxy id");
  }
  Packet p{};
  p.flags = 0;
  p.ip = 0;
  p.port = 0;
  p.adnl_start_time = 0;
  p.seqno = 0;
  p.date = 0;
  p.data = std::move(packet);
  p.data.confirm_read(32);
  return std::move(p);
}

td::BufferSlice AdnlProxyFast::encrypt(Packet packet) const {
  if (!packet.date) {
    packet.date = static_cast<td::int32>(td::Clocks::system());
    packet.flags |= 8;
  }
  auto obj = create_tl_object<ion_api::adnl_proxyPacketHeader>(id_, packet.flags, packet.ip, packet.port,
                                                               packet.adnl_start_time, packet.seqno, packet.date,
                                                               td::sha256_bits256(packet.data.as_slice()));
  char data[64];
  td::MutableSlice S{data, 64};
  S.copy_from(get_tl_object_sha256(obj).as_slice());
  S.remove_prefix(32);
  S.copy_from(shared_secret_.as_slice());

  obj->signature_ = td::sha256_bits256(td::Slice(data, 64));

  return serialize_tl_object(obj, false, std::move(packet.data));
}

td::Result<AdnlProxy::Packet> AdnlProxyFast::decrypt(td::BufferSlice packet) const {
  TRY_RESULT(obj, fetch_tl_prefix<ion_api::adnl_proxyPacketHeader>(packet, false));
  if (obj->proxy_id_ != id_) {
    return td::Status::Error(ErrorCode::protoviolation, "bad proxy id");
  }

  auto signature = std::move(obj->signature_);
  obj->signature_ = td::sha256_bits256(packet.as_slice());

  char data[64];
  td::MutableSlice S{data, 64};
  S.copy_from(get_tl_object_sha256(obj).as_slice());
  S.remove_prefix(32);
  S.copy_from(shared_secret_.as_slice());

  if (td::sha256_bits256(td::Slice(data, 64)) != signature) {
    return td::Status::Error(ErrorCode::protoviolation, "bad signature");
  }

  Packet p;
  p.flags = obj->flags_;
  p.ip = (p.flags & 1) ? obj->ip_ : 0;
  p.port = (p.flags & 1) ? static_cast<td::uint16>(obj->port_) : 0;
  p.adnl_start_time = (p.flags & 2) ? obj->adnl_start_time_ : 0;
  p.seqno = (p.flags & 4) ? obj->seqno_ : 0;
  p.date = (p.flags & 8) ? obj->date_ : 0;
  p.data = std::move(packet);

  return std::move(p);
}

td::Result<std::shared_ptr<AdnlProxy>> AdnlProxy::create(const ion_api::adnl_Proxy &proxy_type) {
  std::shared_ptr<AdnlProxy> R;
  ion_api::downcast_call(
      const_cast<ion_api::adnl_Proxy &>(proxy_type),
      td::overloaded([&](const ion_api::adnl_proxy_none &x) { R = std::make_shared<AdnlProxyNone>(x.id_); },
                     [&](const ion_api::adnl_proxy_fast &x) {
                       R = std::make_shared<AdnlProxyFast>(x.id_, x.shared_secret_.as_slice());
                     }));
  return std::move(R);
}

}  // namespace adnl

}  // namespace ion

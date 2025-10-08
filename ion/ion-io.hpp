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

#include "ion-types.h"
#include "td/utils/base64.h"

namespace td {

inline td::StringBuilder &operator<<(td::StringBuilder &stream, const ion::Bits256 &x) {
  return stream << td::base64_encode(td::Slice(x.data(), x.size() / 8));
}

inline td::StringBuilder &operator<<(td::StringBuilder &stream, const ion::ShardIdFull &x) {
  return stream << "[ w=" << x.workchain << " s=" << x.shard << " ]";
}

inline td::StringBuilder &operator<<(td::StringBuilder &stream, const ion::AccountIdPrefixFull &x) {
  return stream << "[ w=" << x.workchain << " s=" << x.account_id_prefix << " ]";
}

inline td::StringBuilder &operator<<(td::StringBuilder &stream, const ion::BlockId &x) {
  return stream << "[ w=" << x.workchain << " s=" << x.shard << " seq=" << x.seqno << " ]";
}
inline td::StringBuilder &operator<<(td::StringBuilder &stream, const ion::BlockIdExt &x) {
  return stream << "[ w=" << x.id.workchain << " s=" << x.id.shard << " seq=" << x.id.seqno << " " << x.root_hash << " "
                << x.file_hash << " ]";
}

}  // namespace td

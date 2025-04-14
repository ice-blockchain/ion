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

#include "block/block.h"
#include "td/utils/CancellationToken.h"
#include "td/utils/optional.h"
#include "td/utils/Time.h"
#include <atomic>
#include <array>

namespace ion {
class Miner {
 public:
  struct Options {
    block::StdAddress my_address;
    std::array<td::uint8, 16> seed;
    std::array<td::uint8, 32> complexity;
    td::optional<td::Timestamp> expire_at;
    td::int64 max_iterations = std::numeric_limits<td::int64>::max();
    std::atomic<td::uint64>* hashes_computed{nullptr};
    td::CancellationToken token_;
  };

  static td::optional<std::string> run(const Options& options);
};
}  // namespace ion

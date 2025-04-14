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
#include "ion/ion-types.h"
#include "auto/tl/ion_api.h"

namespace ion::validatorsession {

td::Result<td::BufferSlice> serialize_candidate(const tl_object_ptr<ion_api::validatorSession_candidate>& block,
                                                bool compression_enabled);
td::Result<tl_object_ptr<ion_api::validatorSession_candidate>> deserialize_candidate(td::Slice data,
                                                                                     bool compression_enabled,
                                                                                     int max_decompressed_data_size);

td::Result<td::BufferSlice> compress_candidate_data(td::Slice block, td::Slice collated_data,
                                                    size_t& decompressed_size);
td::Result<std::pair<td::BufferSlice, td::BufferSlice>> decompress_candidate_data(td::Slice compressed,
                                                                                  int decompressed_size);

}  // namespace ion::validatorsession

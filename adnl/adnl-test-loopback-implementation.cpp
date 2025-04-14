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
#include "adnl-test-loopback-implementation.h"

namespace ion {

namespace adnl {

AdnlAddressList TestLoopbackNetworkManager::generate_dummy_addr_list(bool empty) {
  auto obj = ion::create_tl_object<ion::ion_api::adnl_address_udp>(1, 1);
  auto objv = std::vector<ion::tl_object_ptr<ion::ion_api::adnl_Address>>();
  objv.push_back(std::move(obj));
  td::uint32 now = Adnl::adnl_start_time();
  auto addrR = ion::adnl::AdnlAddressList::create(
      ion::create_tl_object<ion::ion_api::adnl_addressList>(std::move(objv), empty ? 0 : now, empty ? 0 : now, 0, 0));
  addrR.ensure();
  return addrR.move_as_ok();
}

}  // namespace adnl

}  // namespace ion

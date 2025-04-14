/* 
    This file is part of ION Blockchain source code.

    ION Blockchain is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    ION Blockchain is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ION Blockchain.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give permission 
    to link the code of portions of this program with the OpenSSL library. 
    You must obey the GNU General Public License in all respects for all 
    of the code used other than OpenSSL. If you modify file(s) with this 
    exception, you may extend this exception to your version of the file(s), 
    but you are not obligated to do so. If you do not wish to do so, delete this 
    exception statement from your version. If you delete this exception statement 
    from all source files in the program, then also delete it here.

    Copyright 2017-2020 Telegram Systems LLP
*/
#include "adnl/adnl-network-manager.h"
#include "adnl/adnl-test-loopback-implementation.h"
#include "adnl/adnl.h"
#include "dht/dht.h"
#include "dht/dht.hpp"

#include "td/utils/port/signals.h"
#include "td/utils/port/path.h"
#include "td/utils/Random.h"

#include <memory>
#include <set>

int main() {
  SET_VERBOSITY_LEVEL(verbosity_INFO);

  std::string db_root_ = "tmp-dir-test-dht";
  td::rmrf(db_root_).ignore();
  td::mkdir(db_root_).ensure();

  td::set_default_failure_signal_handler().ensure();

  td::actor::ActorOwn<ion::keyring::Keyring> keyring;
  td::actor::ActorOwn<ion::adnl::TestLoopbackNetworkManager> network_manager;
  td::actor::ActorOwn<ion::adnl::Adnl> adnl;
  std::vector<td::actor::ActorOwn<ion::dht::Dht>> dht;
  std::shared_ptr<ion::dht::DhtGlobalConfig> dht_config;

  td::actor::Scheduler scheduler({7});

  std::vector<ion::adnl::AdnlNodeIdFull> dht_ids;
  td::uint32 total_nodes = 11;
  std::atomic<td::uint32> remaining{0};

  scheduler.run_in_context([&] {
    keyring = ion::keyring::Keyring::create(db_root_);
    network_manager = td::actor::create_actor<ion::adnl::TestLoopbackNetworkManager>("test net");
    adnl = ion::adnl::Adnl::create(db_root_, keyring.get());
    td::actor::send_closure(adnl, &ion::adnl::Adnl::register_network_manager, network_manager.get());

    auto addr0 = ion::adnl::TestLoopbackNetworkManager::generate_dummy_addr_list(true);
    auto addr = ion::adnl::TestLoopbackNetworkManager::generate_dummy_addr_list();

    for (td::uint32 i = 0; i < total_nodes; i++) {
      auto pk1 = ion::PrivateKey{ion::privkeys::Ed25519::random()};
      auto pub1 = pk1.compute_public_key();
      auto src = ion::adnl::AdnlNodeIdShort{pub1.compute_short_id()};

      if (i == 0) {
        auto obj = ion::create_tl_object<ion::ion_api::dht_node>(pub1.tl(), addr0.tl(), -1, td::BufferSlice());
        auto d = pk1.create_decryptor().move_as_ok();
        obj->signature_ = d->sign(serialize_tl_object(obj, true)).move_as_ok();

        std::vector<ion::tl_object_ptr<ion::ion_api::dht_node>> vec;
        vec.push_back(std::move(obj));
        auto nodes = ion::create_tl_object<ion::ion_api::dht_nodes>(std::move(vec));
        auto conf = ion::create_tl_object<ion::ion_api::dht_config_global>(std::move(nodes), 6, 3);
        auto dht_configR = ion::dht::Dht::create_global_config(std::move(conf));
        dht_configR.ensure();
        dht_config = dht_configR.move_as_ok();
      }
      td::actor::send_closure(keyring, &ion::keyring::Keyring::add_key, std::move(pk1), true, [](td::Unit) {});
      td::actor::send_closure(adnl, &ion::adnl::Adnl::add_id, ion::adnl::AdnlNodeIdFull{pub1}, addr,
                              static_cast<td::uint8>(0));
      td::actor::send_closure(network_manager, &ion::adnl::TestLoopbackNetworkManager::add_node_id, src, true, true);

      dht.push_back(ion::dht::Dht::create(src, db_root_, dht_config, keyring.get(), adnl.get()).move_as_ok());
      dht_ids.push_back(ion::adnl::AdnlNodeIdFull{pub1});
    }
    for (auto &n1 : dht_ids) {
      td::actor::send_closure(adnl, &ion::adnl::Adnl::add_peer, n1.compute_short_id(), dht_ids[0], addr);
    }
  });

  LOG(ERROR) << "testing different values";
  auto key_pk = ion::PrivateKey{ion::privkeys::Ed25519::random()};
  auto key_pub = key_pk.compute_public_key();
  auto key_short_id = key_pub.compute_short_id();
  auto key_dec = key_pk.create_decryptor().move_as_ok();
  {
    for (td::uint32 idx = 0; idx <= ion::dht::DhtKey::max_index() + 1; idx++) {
      ion::dht::DhtKey dht_key{key_short_id, "test", idx};
      if (idx <= ion::dht::DhtKey::max_index()) {
        dht_key.check().ensure();
      } else {
        dht_key.check().ensure_error();
      }
    }
    {
      ion::dht::DhtKey dht_key{key_short_id, "test", 0};
      dht_key.check().ensure();
      dht_key = ion::dht::DhtKey{key_short_id, "", 0};
      dht_key.check().ensure_error();
      dht_key =
          ion::dht::DhtKey{key_short_id, td::BufferSlice{ion::dht::DhtKey::max_name_length()}.as_slice().str(), 0};
      dht_key.check().ensure();
      dht_key =
          ion::dht::DhtKey{key_short_id, td::BufferSlice{ion::dht::DhtKey::max_name_length() + 1}.as_slice().str(), 0};
      dht_key.check().ensure_error();
    }
    {
      ion::dht::DhtKey dht_key{key_short_id, "test", 0};
      auto dht_update_rule = ion::dht::DhtUpdateRuleSignature::create().move_as_ok();
      ion::dht::DhtKeyDescription dht_key_description{dht_key.clone(), key_pub, dht_update_rule, td::BufferSlice()};
      dht_key_description.update_signature(key_dec->sign(dht_key_description.to_sign()).move_as_ok());
      dht_key_description.check().ensure();
      dht_key_description = ion::dht::DhtKeyDescription{dht_key.clone(), key_pub, dht_update_rule, td::BufferSlice(64)};
      dht_key_description.check().ensure_error();
      dht_key_description.update_signature(key_dec->sign(dht_key_description.to_sign()).move_as_ok());
      dht_key_description.check().ensure();

      auto pk = ion::PrivateKey{ion::privkeys::Ed25519::random()};
      auto pub = pk.compute_public_key();
      dht_key_description = ion::dht::DhtKeyDescription{dht_key.clone(), pub, dht_update_rule, td::BufferSlice(64)};
      dht_key_description.update_signature(
          pk.create_decryptor().move_as_ok()->sign(dht_key_description.to_sign()).move_as_ok());
      dht_key_description.check().ensure_error();
    }
  }
  {
    ion::dht::DhtKey dht_key{key_short_id, "test", 0};
    auto dht_update_rule = ion::dht::DhtUpdateRuleSignature::create().move_as_ok();
    ion::dht::DhtKeyDescription dht_key_description{std::move(dht_key), key_pub, std::move(dht_update_rule),
                                                    td::BufferSlice()};
    dht_key_description.update_signature(key_dec->sign(dht_key_description.to_sign()).move_as_ok());

    auto ttl = static_cast<td::uint32>(td::Clocks::system() + 3600);
    ion::dht::DhtValue dht_value{dht_key_description.clone(), td::BufferSlice("value"), ttl, td::BufferSlice("")};
    dht_value.check().ensure_error();
    dht_value.update_signature(key_dec->sign(dht_value.to_sign()).move_as_ok());
    dht_value.check().ensure();
    CHECK(!dht_value.expired());

    dht_value = ion::dht::DhtValue{dht_key_description.clone(), td::BufferSlice(""), ttl, td::BufferSlice("")};
    dht_value.update_signature(key_dec->sign(dht_value.to_sign()).move_as_ok());
    dht_value.check().ensure();

    dht_value = ion::dht::DhtValue{dht_key_description.clone(), td::BufferSlice(""),
                                   static_cast<td::uint32>(td::Clocks::system() - 1), td::BufferSlice("")};
    dht_value.update_signature(key_dec->sign(dht_value.to_sign()).move_as_ok());
    dht_value.check().ensure();
    CHECK(dht_value.expired());

    dht_value = ion::dht::DhtValue{dht_key_description.clone(), td::BufferSlice("value"), ttl, td::BufferSlice("")};
    dht_value.update_signature(td::BufferSlice{64});
    dht_value.check().ensure_error();

    dht_value = ion::dht::DhtValue{dht_key_description.clone(), td::BufferSlice(ion::dht::DhtValue::max_value_size()),
                                   ttl, td::BufferSlice("")};
    dht_value.update_signature(key_dec->sign(dht_value.to_sign()).move_as_ok());
    dht_value.check().ensure();

    dht_value = ion::dht::DhtValue{dht_key_description.clone(),
                                   td::BufferSlice(ion::dht::DhtValue::max_value_size() + 1), ttl, td::BufferSlice("")};
    dht_value.update_signature(key_dec->sign(dht_value.to_sign()).move_as_ok());
    dht_value.check().ensure_error();
  }

  {
    ion::dht::DhtKey dht_key{key_short_id, "test", 0};
    auto dht_update_rule = ion::dht::DhtUpdateRuleAnybody::create().move_as_ok();
    ion::dht::DhtKeyDescription dht_key_description{std::move(dht_key), key_pub, std::move(dht_update_rule),
                                                    td::BufferSlice()};
    dht_key_description.update_signature(key_dec->sign(dht_key_description.to_sign()).move_as_ok());

    auto ttl = static_cast<td::uint32>(td::Clocks::system() + 3600);
    ion::dht::DhtValue dht_value{dht_key_description.clone(), td::BufferSlice("value"), ttl, td::BufferSlice()};
    dht_value.check().ensure();
    CHECK(!dht_value.expired());
    dht_value.update_signature(key_dec->sign(dht_value.to_sign()).move_as_ok());
    dht_value.check().ensure_error();

    dht_value = ion::dht::DhtValue{dht_key_description.clone(), td::BufferSlice(), ttl, td::BufferSlice()};
    dht_value.check().ensure();

    dht_value = ion::dht::DhtValue{dht_key_description.clone(), td::BufferSlice(ion::dht::DhtValue::max_value_size()),
                                   ttl, td::BufferSlice()};
    dht_value.check().ensure();

    dht_value = ion::dht::DhtValue{dht_key_description.clone(),
                                   td::BufferSlice(ion::dht::DhtValue::max_value_size() + 1), ttl, td::BufferSlice()};
    dht_value.check().ensure_error();
  }

  {
    ion::dht::DhtKey dht_key{key_short_id, "test", 0};
    auto dht_update_rule = ion::dht::DhtUpdateRuleOverlayNodes::create().move_as_ok();
    ion::dht::DhtKeyDescription dht_key_description{std::move(dht_key), key_pub, std::move(dht_update_rule),
                                                    td::BufferSlice()};
    dht_key_description.update_signature(key_dec->sign(dht_key_description.to_sign()).move_as_ok());

    auto ttl = static_cast<td::uint32>(td::Clocks::system() + 3600);
    ion::dht::DhtValue dht_value{dht_key_description.clone(), td::BufferSlice(""), ttl, td::BufferSlice()};
    dht_value.check().ensure_error();

    auto obj = ion::create_tl_object<ion::ion_api::overlay_nodes>();
    dht_value =
        ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
    dht_value.check().ensure();

    for (td::uint32 i = 0; i < 100; i++) {
      auto pk = ion::PrivateKey{ion::privkeys::Ed25519::random()};
      auto pub = pk.compute_public_key();

      auto date = static_cast<td::int32>(td::Clocks::system() - 10);
      //overlay.node.toSign id:adnl.id.short overlay:int256 version:int = overlay.node.ToSign;
      //overlay.node id:PublicKey overlay:int256 version:int signature:bytes = overlay.Node;
      auto to_sign = ion::create_serialize_tl_object<ion::ion_api::overlay_node_toSign>(
          ion::adnl::AdnlNodeIdShort{pub.compute_short_id()}.tl(), key_short_id.tl(), date);
      auto n = ion::create_tl_object<ion::ion_api::overlay_node>(
          pub.tl(), key_short_id.tl(), date, pk.create_decryptor().move_as_ok()->sign(to_sign.as_slice()).move_as_ok());
      obj->nodes_.push_back(std::move(n));
      dht_value =
          ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
      auto size = ion::serialize_tl_object(obj, true).size();
      if (size <= ion::dht::DhtValue::max_value_size()) {
        dht_value.check().ensure();
      } else {
        dht_value.check().ensure_error();
      }
    }

    obj->nodes_.clear();
    auto pk = ion::PrivateKey{ion::privkeys::Ed25519::random()};
    auto pub = pk.compute_public_key();

    auto date = static_cast<td::int32>(td::Clocks::system() - 10);
    //overlay.node.toSign id:adnl.id.short overlay:int256 version:int = overlay.node.ToSign;
    //overlay.node id:PublicKey overlay:int256 version:int signature:bytes = overlay.Node;
    auto to_sign = ion::create_serialize_tl_object<ion::ion_api::overlay_node_toSign>(
        ion::adnl::AdnlNodeIdShort{pub.compute_short_id()}.tl(), key_short_id.tl() ^ td::Bits256::ones(), date);
    auto n = ion::create_tl_object<ion::ion_api::overlay_node>(
        pub.tl(), key_short_id.tl() ^ td::Bits256::ones(), date,
        pk.create_decryptor().move_as_ok()->sign(to_sign.as_slice()).move_as_ok());
    obj->nodes_.push_back(std::move(n));
    dht_value =
        ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
    dht_value.check().ensure_error();

    obj->nodes_.clear();
    to_sign = ion::create_serialize_tl_object<ion::ion_api::overlay_node_toSign>(
        ion::adnl::AdnlNodeIdShort{pub.compute_short_id()}.tl(), key_short_id.tl(), date);
    n = ion::create_tl_object<ion::ion_api::overlay_node>(
        pub.tl(), key_short_id.tl(), date, pk.create_decryptor().move_as_ok()->sign(to_sign.as_slice()).move_as_ok());
    obj->nodes_.push_back(std::move(n));
    dht_value =
        ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
    dht_value.check().ensure();

    obj->nodes_.clear();
    //to_sign = ion::create_serialize_tl_object<ion::ion_api::overlay_node_toSign>(
    //    ion::adnl::AdnlNodeIdShort{pub.compute_short_id()}.tl(), key_short_id.tl(), date);
    n = ion::create_tl_object<ion::ion_api::overlay_node>(pub.tl(), key_short_id.tl(), date, td::BufferSlice{64});
    obj->nodes_.push_back(std::move(n));
    dht_value =
        ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
    dht_value.check().ensure_error();

    obj->nodes_.clear();
    to_sign = ion::create_serialize_tl_object<ion::ion_api::overlay_node_toSign>(
        ion::adnl::AdnlNodeIdShort{pub.compute_short_id()}.tl(), key_short_id.tl(), date);
    n = ion::create_tl_object<ion::ion_api::overlay_node>(
        pub.tl(), key_short_id.tl(), date, pk.create_decryptor().move_as_ok()->sign(to_sign.as_slice()).move_as_ok());
    obj->nodes_.push_back(std::move(n));
    dht_value =
        ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
    dht_value.check().ensure();

    auto dht_value2 =
        ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
    dht_value2.check().ensure();
    dht_value.update(std::move(dht_value2)).ensure();
    CHECK(ion::fetch_tl_object<ion::ion_api::overlay_nodes>(dht_value.value().as_slice(), true)
              .move_as_ok()
              ->nodes_.size() == 1);

    obj->nodes_.clear();
    {
      td::BufferSlice x{64};
      td::Random::secure_bytes(x.as_slice());
      auto pk2 = ion::PrivateKey{ion::privkeys::Unenc{x.clone()}};
      n = ion::create_tl_object<ion::ion_api::overlay_node>(
          pk2.compute_public_key().tl(), key_short_id.tl(), date,
          pk2.create_decryptor().move_as_ok()->sign(to_sign.as_slice()).move_as_ok());
      obj->nodes_.push_back(std::move(n));
    }
    dht_value2 =
        ion::dht::DhtValue{dht_key_description.clone(), ion::serialize_tl_object(obj, true), ttl, td::BufferSlice()};
    dht_value2.check().ensure();
    dht_value.update(std::move(dht_value2)).ensure();
    CHECK(ion::fetch_tl_object<ion::ion_api::overlay_nodes>(dht_value.value().as_slice(), true)
              .move_as_ok()
              ->nodes_.size() == 2);
  }
  LOG(ERROR) << "success";

  LOG(ERROR) << "empty run";
  auto t = td::Timestamp::in(10.0);
  while (scheduler.run(1)) {
    if (t.is_in_past()) {
      break;
    }
  }

  LOG(ERROR) << "success";

  for (td::uint32 x = 0; x < 100; x++) {
    ion::dht::DhtKey dht_key{key_short_id, PSTRING() << "test-" << x, x % 8};
    auto dht_update_rule = ion::dht::DhtUpdateRuleSignature::create().move_as_ok();
    ion::dht::DhtKeyDescription dht_key_description{std::move(dht_key), key_pub, std::move(dht_update_rule),
                                                    td::BufferSlice()};
    dht_key_description.update_signature(key_dec->sign(dht_key_description.to_sign()).move_as_ok());

    auto ttl = static_cast<td::uint32>(td::Clocks::system() + 3600);
    td::uint8 v[1];
    v[0] = static_cast<td::uint8>(x);
    ion::dht::DhtValue dht_value{std::move(dht_key_description), td::BufferSlice(td::Slice(v, 1)), ttl,
                                 td::BufferSlice("")};
    dht_value.update_signature(key_dec->sign(dht_value.to_sign()).move_as_ok());

    remaining++;
    auto P = td::PromiseCreator::lambda([&](td::Result<td::Unit> R) {
      R.ensure();
      remaining--;
    });

    scheduler.run_in_context([&] {
      td::actor::send_closure(dht[td::Random::fast(0, total_nodes - 1)], &ion::dht::Dht::set_value,
                              std::move(dht_value), std::move(P));
    });
  }

  LOG(ERROR) << "stores";
  t = td::Timestamp::in(60.0);
  while (scheduler.run(1)) {
    if (!remaining) {
      break;
    }
    if (t.is_in_past()) {
      LOG(FATAL) << "failed: remaining = " << remaining;
    }
  }
  LOG(ERROR) << "success";

  for (td::uint32 x = 0; x < 100; x++) {
    ion::dht::DhtKey dht_key{key_short_id, PSTRING() << "test-" << x, x % 8};

    remaining++;
    auto P = td::PromiseCreator::lambda([&, idx = x](td::Result<ion::dht::DhtValue> R) {
      R.ensure();
      auto v = R.move_as_ok();
      CHECK(v.key().key().public_key_hash() == key_short_id);
      CHECK(v.key().key().name() == (PSTRING() << "test-" << idx));
      CHECK(v.key().key().idx() == idx % 8);
      td::uint8 buf[1];
      buf[0] = static_cast<td::uint8>(idx);
      CHECK(v.value().as_slice() == td::Slice(buf, 1));
      remaining--;
    });

    scheduler.run_in_context([&] {
      td::actor::send_closure(dht[td::Random::fast(0, total_nodes - 1)], &ion::dht::Dht::get_value, dht_key,
                              std::move(P));
    });
  }

  LOG(ERROR) << "gets";
  t = td::Timestamp::in(60.0);
  while (scheduler.run(1)) {
    if (!remaining) {
      break;
    }
    if (t.is_in_past()) {
      LOG(FATAL) << "failed: remaining = " << remaining;
    }
  }
  LOG(ERROR) << "success";

  td::rmrf(db_root_).ensure();
  std::_Exit(0);
  return 0;
}

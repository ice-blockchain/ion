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
#include "adnl/adnl-node-id.hpp"
#include "adnl/adnl.h"
#include "adnl/utils.hpp"
#include "adnl/adnl-test-loopback-implementation.h"
#include "auto/tl/ion_api.h"
#include "checksum.h"
#include "common/bitstring.h"
#include "dht/dht.h"
#include "keys/keys.hpp"
#include "overlay-manager.h"
#include "overlay.h"
#include "overlay-id.hpp"
#include "overlay/overlays.h"
#include "td/actor/actor.h"
#include "td/utils/OptionParser.h"
#include "td/utils/Status.h"
#include "td/utils/Time.h"
#include "td/utils/UInt.h"
#include "td/utils/buffer.h"
#include "td/utils/crypto.h"
#include "td/utils/filesystem.h"
#include "td/utils/format.h"
#include "td/utils/port/path.h"
#include "td/utils/Random.h"
#include "td/utils/port/signals.h"
#include "td/utils/port/FileFd.h"
#include "td/utils/overloaded.h"
#include "common/errorlog.h"
#include "tl-utils/common-utils.hpp"
#include "tl/TlObject.h"
#include <memory>
#include <vector>

#if TD_DARWIN || TD_LINUX
#include <unistd.h>
#endif
#include <iostream>
#include <sstream>

#include <set>

struct Node {
  ion::PrivateKey pk;
  ion::PublicKeyHash id;
  ion::PublicKey id_full;
  ion::adnl::AdnlNodeIdShort adnl_id;
  ion::adnl::AdnlNodeIdFull adnl_id_full;
  bool can_receive;
};

static std::vector<Node> root_nodes;
static std::vector<Node> slave_nodes;
static std::vector<Node *> all_nodes;
static td::uint32 total_nodes = 4;
static td::int32 node_slaves_cnt = 3;
static size_t remaining = 0;
static td::Bits256 bcast_hash;

class Callback : public ion::overlay::Overlays::Callback {
 public:
  Callback(bool can_receive) : can_receive_(can_receive) {
  }
  void receive_message(ion::adnl::AdnlNodeIdShort src, ion::overlay::OverlayIdShort overlay_id,
                       td::BufferSlice data) override {
    UNREACHABLE();
  }
  void receive_query(ion::adnl::AdnlNodeIdShort src, ion::overlay::OverlayIdShort overlay_id, td::BufferSlice data,
                     td::Promise<td::BufferSlice> promise) override {
    UNREACHABLE();
  }
  void receive_broadcast(ion::PublicKeyHash src, ion::overlay::OverlayIdShort overlay_id,
                         td::BufferSlice data) override {
    CHECK(can_receive_);
    CHECK(td::sha256_bits256(data.as_slice()) == bcast_hash);
    CHECK(remaining > 0);
    remaining--;
  }

 private:
  bool can_receive_;
};

int main(int argc, char *argv[]) {
  SET_VERBOSITY_LEVEL(verbosity_INFO);
  td::set_default_failure_signal_handler().ensure();

  std::string db_root_ = "tmp-dir-test-catchain";
  td::rmrf(db_root_).ignore();
  td::mkdir(db_root_).ensure();

  td::set_default_failure_signal_handler().ensure();

  td::actor::ActorOwn<ion::keyring::Keyring> keyring;
  td::actor::ActorOwn<ion::adnl::TestLoopbackNetworkManager> network_manager;
  td::actor::ActorOwn<ion::adnl::Adnl> adnl;
  td::actor::ActorOwn<ion::overlay::Overlays> overlay_manager;

  td::actor::Scheduler scheduler({7});
  scheduler.run_in_context([&] {
    ion::errorlog::ErrorLog::create(db_root_);
    keyring = ion::keyring::Keyring::create(db_root_);
    network_manager = td::actor::create_actor<ion::adnl::TestLoopbackNetworkManager>("test net");
    adnl = ion::adnl::Adnl::create(db_root_, keyring.get());
    overlay_manager =
        ion::overlay::Overlays::create(db_root_, keyring.get(), adnl.get(), td::actor::ActorId<ion::dht::Dht>{});
    td::actor::send_closure(adnl, &ion::adnl::Adnl::register_network_manager, network_manager.get());
  });

  td::uint32 att = 0;
  for (td::uint32 start = att; att < start + 5; att++) {
    LOG(WARNING) << "Test #" << att;
    root_nodes.resize(total_nodes);
    slave_nodes.resize(total_nodes * node_slaves_cnt);

    auto overlay_id_full =
        ion::create_serialize_tl_object<ion::ion_api::pub_overlay>(td::BufferSlice(PSTRING() << "TEST" << att));
    ion::overlay::OverlayIdFull overlay_id(overlay_id_full.clone());
    auto overlay_id_short = overlay_id.compute_short_id();

    ion::overlay::OverlayOptions opts;
    opts.max_slaves_in_semiprivate_overlay_ = node_slaves_cnt;
    opts.default_permanent_members_flags_ = ion::overlay::OverlayMemberFlags::DoNotReceiveBroadcasts;

    ion::overlay::OverlayPrivacyRules rules(
        20 << 20, ion::overlay::CertificateFlags::AllowFec | ion::overlay::CertificateFlags::Trusted, {});

    std::vector<ion::PublicKeyHash> root_keys;
    std::vector<ion::adnl::AdnlNodeIdShort> root_adnl;

    size_t real_members = 0;

    scheduler.run_in_context([&] {
      auto addr = ion::adnl::TestLoopbackNetworkManager::generate_dummy_addr_list();

      for (auto &n : root_nodes) {
        bool receive_bcasts = (real_members == 0) ? true : (td::Random::fast_uint32() & 1);
        if (receive_bcasts) {
          real_members++;
        }
        n.can_receive = receive_bcasts;

        auto pk1 = ion::PrivateKey{ion::privkeys::Ed25519::random()};
        auto pub1 = pk1.compute_public_key();
        n.adnl_id_full = ion::adnl::AdnlNodeIdFull{pub1};
        n.adnl_id = ion::adnl::AdnlNodeIdShort{pub1.compute_short_id()};
        td::actor::send_closure(keyring, &ion::keyring::Keyring::add_key, std::move(pk1), true, [](td::Unit) {});
        td::actor::send_closure(adnl, &ion::adnl::Adnl::add_id, ion::adnl::AdnlNodeIdFull{pub1}, addr,
                                static_cast<td::uint8>(0));
        td::actor::send_closure(network_manager, &ion::adnl::TestLoopbackNetworkManager::add_node_id, n.adnl_id, true,
                                true);

        auto pk2 = ion::PrivateKey{ion::privkeys::Ed25519::random()};
        auto pub2 = pk2.compute_public_key();
        n.id_full = pub2;
        n.id = pub2.compute_short_id();
        n.pk = pk2;
        td::actor::send_closure(keyring, &ion::keyring::Keyring::add_key, std::move(pk2), true, [](td::Unit) {});

        LOG(DEBUG) << "created node " << n.adnl_id << " " << n.id;

        all_nodes.push_back(&n);
        root_keys.push_back(n.id);
        root_adnl.push_back(n.adnl_id);
      }

      for (auto &n : slave_nodes) {
        bool receive_bcasts = (real_members == 0) ? true : (td::Random::fast_uint32() & 1);
        if (receive_bcasts) {
          real_members++;
        }
        n.can_receive = receive_bcasts;

        auto pk1 = ion::PrivateKey{ion::privkeys::Ed25519::random()};
        auto pub1 = pk1.compute_public_key();
        n.adnl_id_full = ion::adnl::AdnlNodeIdFull{pub1};
        n.adnl_id = ion::adnl::AdnlNodeIdShort{pub1.compute_short_id()};
        td::actor::send_closure(keyring, &ion::keyring::Keyring::add_key, std::move(pk1), true, [](td::Unit) {});
        td::actor::send_closure(adnl, &ion::adnl::Adnl::add_id, ion::adnl::AdnlNodeIdFull{pub1}, addr,
                                static_cast<td::uint8>(0));
        td::actor::send_closure(network_manager, &ion::adnl::TestLoopbackNetworkManager::add_node_id, n.adnl_id, true,
                                true);

        auto pk2 = ion::PrivateKey{ion::privkeys::Ed25519::random()};
        auto pub2 = pk2.compute_public_key();
        n.id_full = pub2;
        n.id = pub2.compute_short_id();
        n.pk = pk2;
        td::actor::send_closure(keyring, &ion::keyring::Keyring::add_key, std::move(pk2), true, [](td::Unit) {});

        LOG(DEBUG) << "created node " << n.adnl_id << " " << n.id;
        all_nodes.push_back(&n);
      }

      for (auto &n1 : all_nodes) {
        for (auto &n2 : all_nodes) {
          td::actor::send_closure(adnl, &ion::adnl::Adnl::add_peer, n1->adnl_id, n2->adnl_id_full, addr);
        }
      }

      for (auto &n1 : root_nodes) {
        opts.local_overlay_member_flags_ =
            (n1.can_receive ? 0 : ion::overlay::OverlayMemberFlags::DoNotReceiveBroadcasts);
        td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::create_semiprivate_overlay, n1.adnl_id,
                                ion::overlay::OverlayIdFull(overlay_id_full.clone()), root_adnl, root_keys,
                                ion::overlay::OverlayMemberCertificate{}, std::make_unique<Callback>(n1.can_receive),
                                rules, "", opts);
      }
      for (size_t i = 0; i < slave_nodes.size(); i++) {
        auto &n1 = slave_nodes[i];
        opts.local_overlay_member_flags_ =
            (n1.can_receive ? 0 : ion::overlay::OverlayMemberFlags::DoNotReceiveBroadcasts);

        ion::overlay::OverlayMemberCertificate cert(root_nodes[i / node_slaves_cnt].id_full, 0, i % node_slaves_cnt,
                                                  2000000000, td::BufferSlice());
        auto buf = cert.to_sign_data(n1.adnl_id);
        auto dec = root_nodes[i / node_slaves_cnt].pk.create_decryptor().move_as_ok();
        auto signature = dec->sign(buf.as_slice()).move_as_ok();
        cert.set_signature(signature.as_slice());
        auto enc = root_nodes[i / node_slaves_cnt].id_full.create_encryptor().move_as_ok();
        enc->check_signature(cert.to_sign_data(n1.adnl_id), cert.signature()).ensure();

        td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::create_semiprivate_overlay, n1.adnl_id,
                                ion::overlay::OverlayIdFull(overlay_id_full.clone()), root_adnl, root_keys, cert,
                                std::make_unique<Callback>(n1.can_receive), rules, "", opts);
      }
    });

    td::BufferSlice broadcast(1 << 20);
    td::Random::secure_bytes(broadcast.as_slice());
    remaining = real_members;
    bcast_hash = td::sha256_bits256(broadcast.as_slice());

    auto t = td::Timestamp::in(20.0);
    while (scheduler.run(1)) {
      if (t.is_in_past()) {
        break;
      }
    }

    scheduler.run_in_context([&] {
      /*td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::get_stats,
                              [&](td::Result<ion::tl_object_ptr<ion::ion_api::engine_validator_overlaysStats>> R) {
                                if (R.is_ok()) {
                                  auto res = R.move_as_ok();
                                  for (auto &o : res->overlays_) {
                                    if (o->overlay_id_ == overlay_id_short.bits256_value()) {
                                      LOG(ERROR) << "NODE " << o->adnl_id_ << " nodes=" << o->nodes_.size();
                                      for (auto &x : o->stats_) {
                                        LOG(ERROR) << "\t" << x->key_ << " " << x->value_;
                                      }
                                      for (auto &x : o->nodes_) {
                                        LOG(ERROR) << "\t\t" << x->adnl_id_;
                                      }
                                    }
                                  }
                                }
                              });*/
      td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::send_broadcast_fec_ex, root_nodes[0].adnl_id,
                              overlay_id_short, root_nodes[0].id, 0, std::move(broadcast));
    });

    t = td::Timestamp::in(10.0);
    while (scheduler.run(1)) {
      if (t.is_in_past()) {
        break;
      }
      if (!remaining) {
        break;
      }
    }

    LOG_CHECK(!remaining) << "remaining=" << remaining << " all=" << real_members;

    broadcast = td::BufferSlice(700);
    td::Random::secure_bytes(broadcast.as_slice());
    remaining = real_members;
    bcast_hash = td::sha256_bits256(broadcast.as_slice());
    scheduler.run_in_context([&] {
      td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::send_broadcast_ex, root_nodes[0].adnl_id,
                              overlay_id_short, root_nodes[0].id, 0, std::move(broadcast));
    });

    t = td::Timestamp::in(10.0);
    while (scheduler.run(1)) {
      if (t.is_in_past()) {
        break;
      }
      if (!remaining) {
        break;
      }
    }

    LOG_CHECK(!remaining) << "remaining=" << remaining;

    scheduler.run_in_context([&] {
      root_nodes.clear();
      slave_nodes.clear();
      all_nodes.clear();
    });
  }

  for (td::uint32 start = att; att < start + 5; att++) {
    LOG(WARNING) << "Test #" << att;
    root_nodes.resize(total_nodes);

    auto overlay_id_full =
        ion::create_serialize_tl_object<ion::ion_api::pub_overlay>(td::BufferSlice(PSTRING() << "TEST" << att));
    ion::overlay::OverlayIdFull overlay_id(overlay_id_full.clone());
    auto overlay_id_short = overlay_id.compute_short_id();

    ion::overlay::OverlayOptions opts;

    ion::overlay::OverlayPrivacyRules rules(
        20 << 20, ion::overlay::CertificateFlags::AllowFec | ion::overlay::CertificateFlags::Trusted, {});

    std::vector<ion::PublicKeyHash> root_keys;
    std::vector<ion::adnl::AdnlNodeIdShort> root_adnl;

    size_t real_members = 0;

    scheduler.run_in_context([&] {
      auto addr = ion::adnl::TestLoopbackNetworkManager::generate_dummy_addr_list();

      for (auto &n : root_nodes) {
        real_members++;
        auto pk1 = ion::PrivateKey{ion::privkeys::Ed25519::random()};
        auto pub1 = pk1.compute_public_key();
        n.adnl_id_full = ion::adnl::AdnlNodeIdFull{pub1};
        n.adnl_id = ion::adnl::AdnlNodeIdShort{pub1.compute_short_id()};
        td::actor::send_closure(keyring, &ion::keyring::Keyring::add_key, std::move(pk1), true, [](td::Unit) {});
        td::actor::send_closure(adnl, &ion::adnl::Adnl::add_id, ion::adnl::AdnlNodeIdFull{pub1}, addr,
                                static_cast<td::uint8>(0));
        td::actor::send_closure(network_manager, &ion::adnl::TestLoopbackNetworkManager::add_node_id, n.adnl_id, true,
                                true);

        auto pk2 = ion::PrivateKey{ion::privkeys::Ed25519::random()};
        auto pub2 = pk2.compute_public_key();
        n.id_full = pub2;
        n.id = pub2.compute_short_id();
        n.pk = pk2;
        td::actor::send_closure(keyring, &ion::keyring::Keyring::add_key, std::move(pk2), true, [](td::Unit) {});

        LOG(DEBUG) << "created node " << n.adnl_id << " " << n.id;

        all_nodes.push_back(&n);
        root_keys.push_back(n.id);
        root_adnl.push_back(n.adnl_id);
      }

      for (auto &n1 : all_nodes) {
        for (auto &n2 : all_nodes) {
          td::actor::send_closure(adnl, &ion::adnl::Adnl::add_peer, n1->adnl_id, n2->adnl_id_full, addr);
        }
      }

      for (auto &n1 : root_nodes) {
        td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::create_private_overlay_ex, n1.adnl_id,
                                ion::overlay::OverlayIdFull(overlay_id_full.clone()), root_adnl,
                                std::make_unique<Callback>(true), rules, "", opts);
      }
    });

    auto t = td::Timestamp::in(10.0);
    while (scheduler.run(1)) {
      if (t.is_in_past()) {
        break;
      }
    }

    td::BufferSlice broadcast(1 << 20);
    td::Random::secure_bytes(broadcast.as_slice());
    remaining = real_members;
    bcast_hash = td::sha256_bits256(broadcast.as_slice());

    scheduler.run_in_context([&] {
      td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::send_broadcast_fec_ex, root_nodes[0].adnl_id,
                              overlay_id_short, root_nodes[0].id, 0, std::move(broadcast));
    });

    t = td::Timestamp::in(10.0);
    while (scheduler.run(1)) {
      if (t.is_in_past()) {
        break;
      }
      if (!remaining) {
        break;
      }
    }

    LOG_CHECK(!remaining) << "remaining=" << remaining;

    broadcast = td::BufferSlice(700);
    td::Random::secure_bytes(broadcast.as_slice());
    remaining = real_members;
    bcast_hash = td::sha256_bits256(broadcast.as_slice());
    scheduler.run_in_context([&] {
      td::actor::send_closure(overlay_manager, &ion::overlay::Overlays::send_broadcast_ex, root_nodes[0].adnl_id,
                              overlay_id_short, root_nodes[0].id, 0, std::move(broadcast));
    });

    t = td::Timestamp::in(10.0);
    while (scheduler.run(1)) {
      if (t.is_in_past()) {
        break;
      }
      if (!remaining) {
        break;
      }
    }

    LOG_CHECK(!remaining) << "remaining=" << remaining;

    scheduler.run_in_context([&] {
      root_nodes.clear();
      slave_nodes.clear();
      all_nodes.clear();
    });
  }

  td::rmrf(db_root_).ensure();
  std::_Exit(0);
  return 0;
}

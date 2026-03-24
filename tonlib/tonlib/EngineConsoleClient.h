#pragma once

#include "adnl/adnl-ext-client.h"
#include "keys/keys.hpp"
#include "td/actor/actor.h"
#include "td/actor/coro_task.h"
#include "td/utils/port/IPAddress.h"

namespace tonlib {

bool is_engine_console_query(const ion::tl_object_ptr<ion::ton_api::Function>& function);

class EngineConsoleClient : public td::actor::Actor {
 public:
  EngineConsoleClient(td::IPAddress address, ion::PublicKey server_public_key, ion::PrivateKey client_private_key);

  void on_ready();
  void on_stop_ready();

  td::actor::Task<ion::tl_object_ptr<ion::ton_api::Object>> query(ion::tl_object_ptr<ion::ton_api::Function> function);

 private:
  td::IPAddress address_;
  ion::PublicKey server_public_key_;
  ion::PrivateKey client_private_key_;
  td::actor::ActorOwn<ion::adnl::AdnlExtClient> client_;
  bool ready_ = false;
  std::vector<td::Promise<td::Unit>> pending_ready_promises_;
};

}  // namespace tonlib

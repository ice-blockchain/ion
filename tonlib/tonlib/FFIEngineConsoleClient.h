#pragma once

#include "auto/tl/ton_api.h"
#include "keys/keys.hpp"
#include "td/utils/port/IPAddress.h"

#include "EngineConsoleClient.h"
#include "FFIEventLoop.h"

namespace tonlib {

class FFIEngineConsoleClient {
 public:
  FFIEngineConsoleClient(FFIEventLoop& loop, td::IPAddress address, ion::PublicKey server_public_key,
                         ion::PrivateKey client_private_key);

  FFIEngineConsoleClient(FFIEngineConsoleClient&&) = default;

  ~FFIEngineConsoleClient() {
    if (!client_.empty()) {
      loop_.run_in_context([client = std::move(client_)]() mutable { client.reset(); });
    }
  }

  void request(ion::tl_object_ptr<ion::ton_api::Function> query,
               td::Promise<ion::tl_object_ptr<ion::ton_api::Object>> promise);

  FFIEventLoop& loop() {
    return loop_;
  }

 private:
  FFIEventLoop& loop_;
  td::unique_ptr<td::Guard> counter_;
  td::actor::ActorOwn<EngineConsoleClient> client_;
};

}  // namespace tonlib

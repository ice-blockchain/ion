#include "FFIEngineConsoleClient.h"

namespace tonlib {

FFIEngineConsoleClient::FFIEngineConsoleClient(FFIEventLoop& loop, td::IPAddress address,
                                               ion::PublicKey server_public_key, ion::PrivateKey client_private_key)
    : loop_(loop), counter_(loop.new_actor()) {
  loop_.run_in_context([&] {
    client_ = td::actor::create_actor<EngineConsoleClient>("EngineConsoleClient", address, server_public_key,
                                                           client_private_key);
  });
}

void FFIEngineConsoleClient::request(ion::tl_object_ptr<ion::ton_api::Function> query,
                                     td::Promise<ion::tl_object_ptr<ion::ton_api::Object>> promise) {
  loop_.run_in_context(
      [client = this->client_.get(), query = std::move(query), promise = std::move(promise)]() mutable {
        td::actor::send_closure(client, &EngineConsoleClient::query, std::move(query), std::move(promise));
      });
}

}  // namespace tonlib

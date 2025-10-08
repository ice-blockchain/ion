if (NOT OPENSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

set(ION_VALIDATOR_SOURCE
  accept-block.cpp
  block.cpp
  candidates-buffer.cpp
  check-proof.cpp
  collator.cpp
  config.cpp
  external-message.cpp
  fabric.cpp
  ihr-message.cpp
  liteserver.cpp
  message-queue.cpp
  out-msg-queue-proof.cpp
  proof.cpp
  shard.cpp
  signature-set.cpp
  top-shard-descr.cpp
  validate-query.cpp
  validator-set.cpp

  accept-block.hpp
  block.hpp
  candidates-buffer.hpp
  check-proof.hpp
  collator-impl.h
  collator.h
  config.hpp
  external-message.hpp
  ihr-message.hpp
  liteserver.hpp
  liteserver-cache.hpp
  message-queue.hpp
  out-msg-queue-proof.hpp
  proof.hpp
  shard.hpp
  signature-set.hpp
  top-shard-descr.hpp
  validate-query.hpp
  validator-set.hpp)

add_library(ion_validator STATIC ${ION_VALIDATOR_SOURCE})

target_include_directories(ion_validator PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/.. 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../crypto 
  ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ion_validator PRIVATE tdutils tdactor adnl tl_api tl_lite_api tl-lite-utils dht tdfec
  overlay catchain validatorsession ion_crypto ion_block)

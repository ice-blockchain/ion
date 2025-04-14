cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(HTTP_SOURCE
  http.h
  http.cpp
  http-connection.h
  http-connection.cpp
  http-inbound-connection.h
  http-inbound-connection.cpp
  http-outbound-connection.h
  http-outbound-connection.cpp
  http-server.h
  http-server.cpp
  http-client.h
  http-client.hpp
  http-client.cpp
)

add_library(ionhttp STATIC ${HTTP_SOURCE})
target_include_directories(ionhttp PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
target_link_libraries(ionhttp PUBLIC tdactor ion_crypto tl_api tdnet )

add_executable(http-proxy http-proxy.cpp)
target_include_directories(http-proxy PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
target_link_libraries(http-proxy PRIVATE ionhttp git)

install(TARGETS http-proxy RUNTIME DESTINATION bin)

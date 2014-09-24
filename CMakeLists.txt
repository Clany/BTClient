cmake_minimum_required(VERSION 2.8.11)
project(BitTorrent)

list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
  $ENV{DEV_LIBS}/clany/cmake
)
include(Utilities)

find_compiler(
  GNU   4.7  "-std=c++11 -Wall"
  Clang 3.2  "-std=c++11 -Wall"
  MSVC  2013 "/W4"
)

option(USE_MY_PATH "Use my own default library path" OFF)
if(USE_MY_PATH)
  add_default_lib_path($ENV{DEV_LIBS} $ENV{DEV_PLAT})
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

if(WIN32)
  add_definitions("-D_WIN32_WINNT=0x602")
  set(WINSOCK2_LIB "ws2_32")
endif()

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

set(SRC_LIST
  src/main.cpp
  src/metainfo.cpp
  src/bt_client.cpp
  src/bt_server.cpp
)

set(HEADER_LIST
  include/socket.hpp
  include/tcp_server.hpp
  include/bt_client.h
  include/bt_server.h
  include/metainfo.h
)

add_executable(bt-client ${SRC_LIST} ${HEADER_LIST})
target_link_libraries(bt-client ${OPENSSL_LIBRARIES} ${WINSOCK2_LIB})

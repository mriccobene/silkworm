#[[
   Copyright 2020-2021 The Silkworm Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
]]

if(MSVC)
  add_link_options(/STACK:0x1000000)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_link_options(-Wl,-stack_size -Wl,0x1000000)
else()
  add_link_options(-Wl,-z,stack-size=10000000)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")
endif()

if(NOT SILKWORM_CORE_ONLY)

  find_package(Boost CONFIG REQUIRED COMPONENTS filesystem)

  find_package(CLI11 CONFIG REQUIRED)

  find_package(gRPC CONFIG REQUIRED)

  add_executable(stage1 stage1.cpp
                        SentryClient.cpp
                        messages/Message.cpp
                        messages/OutboundMessage.cpp
                        messages/InboundMessage.cpp
                        messages/InboundGetBlockHeaders.cpp
                        messages/OutboundSendMessageByIdRequest.cpp
                        rpc/SetStatus.cpp
                        rpc/SendMessageById.cpp
                        rpc/ReceiveMessages.cpp
                        "${CMAKE_SOURCE_DIR}/interfaces/sentry.grpc.pb.cc"
                        "${CMAKE_SOURCE_DIR}/interfaces/sentry.pb.cc")

  target_link_libraries(stage1 PRIVATE silkworm_db
                                       silkworm_tg_api
                                       CLI11::CLI11
                                       Boost::filesystem
                                       gRPC::grpc++
                                       protobuf::libprotobuf)

  target_include_directories(stage1 PRIVATE ${CMAKE_SOURCE_DIR}
                                            "${CMAKE_SOURCE_DIR}/interfaces"
                                            messages
                                            rpc)

endif(NOT SILKWORM_CORE_ONLY)

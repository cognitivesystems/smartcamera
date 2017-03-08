cmake_minimum_required(VERSION 2.8)

project(human-tracking-server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -gdwarf-2")

find_library(BCM_HOST_LIBRARY bcm_host)
find_library(EVENT_LIBRARY event)
find_library(THRIFTNB_LIBRARY thriftnb)

add_subdirectory(../rpc ${CMAKE_CURRENT_BINARY_DIR}/thrift-gen-cpp)
add_subdirectory(../../glipf ${CMAKE_CURRENT_BINARY_DIR}/glipf)


add_executable(human-tracking-server
  src/application.cpp
  src/glipf-server-handler.cpp
  src/glipf-server-handler.h
)

target_include_directories(
  human-tracking-server PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR} ../../glipf/include
)

target_link_libraries(
  human-tracking-server
  ${BCM_HOST_LIBRARY}
  glipf-thrift-lib
  glipf
  ${THRIFTNB_LIBRARY}
  ${EVENT_LIBRARY}
)

install(TARGETS human-tracking-server
        RUNTIME DESTINATION .)

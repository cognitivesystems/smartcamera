cmake_minimum_required(VERSION 2.8)

project(glipf-thrift-lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -gdwarf-2")

find_library(THRIFT_LIBRARY thrift)


# Service for the human detection & tracking test application
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/glipf-server/glipf_types.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/glipf-server/glipf_types.h
         ${CMAKE_CURRENT_BINARY_DIR}/glipf-server/GlipfServer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/glipf-server/GlipfServer.h
  COMMAND ${CMAKE_COMMAND} -E make_directory
                           ${CMAKE_CURRENT_BINARY_DIR}/glipf-server
  COMMAND thrift -r --gen cpp -out ${CMAKE_CURRENT_BINARY_DIR}/glipf-server
                 ${CMAKE_CURRENT_SOURCE_DIR}/glipf.thrift
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/glipf.thrift
)

add_library(
  glipf-thrift-lib STATIC EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_BINARY_DIR}/glipf-server/glipf_types.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/glipf-server/GlipfServer.cpp
)

target_link_libraries(
  glipf-thrift-lib
  ${THRIFT_LIBRARY}
)

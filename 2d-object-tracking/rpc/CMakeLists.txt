cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -gdwarf-2")

find_library(THRIFT_LIBRARY thrift)


# Thrift service for the test application that uses a single camera to
# detect and track 2D objects
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/threshold_contours_types.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/threshold_contours_types.h
         ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/threshold_contours_constants.h
         ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/threshold_contours_constants.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/ThresholdContours.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/ThresholdContours.h
  COMMAND ${CMAKE_COMMAND} -E make_directory
                           ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours
  COMMAND thrift -r --gen cpp -out ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours
                 ${CMAKE_CURRENT_SOURCE_DIR}/threshold_contours.thrift
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/threshold_contours.thrift
)

add_library(
  threshold-contours-thrift-lib STATIC EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/threshold_contours_types.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/threshold_contours_constants.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/threshold-contours/ThresholdContours.cpp
)

target_link_libraries(
  threshold-contours-thrift-lib
  ${THRIFT_LIBRARY}
)

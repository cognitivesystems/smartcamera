cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03 -Wall -Wextra")


# Ensure that all dependencies are installed
FIND_PACKAGE(OpenCV REQUIRED)
find_package(PkgConfig)
pkg_check_modules(QT_CORE REQUIRED QtCore)
pkg_check_modules(EIGEN REQUIRED eigen3)
pkg_check_modules(GSL REQUIRED gsl)

add_subdirectory(../rpc ${CMAKE_CURRENT_BINARY_DIR}/thrift-gen-cpp)


# Set up compiler options
include_directories(SYSTEM
        ${QT_CORE_INCLUDE_DIRS}
        ${EIGEN_INCLUDE_DIRS}
        ${GSL_INCLUDE_DIRS}
        ${OPENCV_INCLUDES}
)

LINK_DIRECTORIES(
  ${QT_CORE_LIBRARY_DIRS}
  ${EIGEN_LIBRARY_DIRS}
  ${GSL_LIBRARY_DIRS}
)

add_definitions(
  ${QT_CORE_CFLAGS_OTHER}
  ${EIGEN_CFLAGS_OTHER}
  ${GSL_CFLAGS_OTHER}
)


# Define targets
SET(HDRS
        src/detector/Detector.h
        src/tracker/Tracker.h
        src/tracker/ParticleFilter.h
)

SET(SRCS
        src/detector/Detector.cpp
        src/tracker/Tracker.cpp
        src/tracker/ParticleFilter.cpp
        src/main.cpp
)

add_executable(objdetect ${HDRS} ${SRCS})
target_include_directories(objdetect PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(objdetect
                      ${QT_CORE_LIBRARIES}
                      ${EIGEN_LIBRARIES}
                      ${GSL_LIBRARIES}
                      ${OpenCV_LIBS}
                      threshold-contours-thrift-lib
)


# Specify files to install
install(TARGETS objdetect
        RUNTIME DESTINATION .)

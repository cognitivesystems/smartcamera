cmake_minimum_required(VERSION 2.8)

project(glipf)
find_package(OpenCV REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -gdwarf-2")

find_library(GLESV2_LIBRARY GLESv2)
find_library(EGL_LIBRARY EGL)

find_package(PkgConfig)
pkg_check_modules(V4L2 REQUIRED libv4l2)


include_directories(SYSTEM
  ${V4L2_INCLUDE_DIRS}
)

link_directories(
  ${V4L2_LIBRARY_DIRS}
)

add_definitions(
  ${V4L2_CFLAGS_OTHER}
)

set(
  GLIPF_HEADERS
  include/glipf/sources/frame-properties.h
  include/glipf/sources/frame-source.h
  include/glipf/sources/opencv-camera.h
  include/glipf/sources/v4l2-camera.h
  include/glipf/sources/opencv-video-source.h
  include/glipf/processors/processing-result.h
  include/glipf/processors/gles-processor.h
  include/glipf/processors/copy-processor.h
  include/glipf/processors/color-space-conversion-processor.h
  include/glipf/processors/background-subtraction-processor.h
  include/glipf/processors/foreground-coverage-processor.h
  include/glipf/processors/foreground-histogram-processor.h
  include/glipf/processors/model-debug-processor.h
  include/glipf/processors/model-occlusion-processor.h
  include/glipf/processors/norm-dist-bg-sub-processor.h
  include/glipf/processors/threshold-processor.h
  include/glipf/sinks/sink.h
  include/glipf/sinks/display-sink.h
  include/glipf/utils/timer.h
  include/glipf/gles-utils/gles-context.h
  include/glipf/gles-utils/shader-builder.h
  include/glipf/gles-utils/glsl-program-builder.h
  include/glipf/gles-utils/texture-container.h
  include/glipf/gles-utils/dump-to-image.h
)

set(
  GLIPF_SOURCES
  src/sources/frame-properties.cpp
  src/sources/opencv-camera.cpp
  src/sources/v4l2-camera.cpp
  src/sources/opencv-video-source.cpp
  src/processors/gles-processor.cpp
  src/processors/copy-processor.cpp
  src/processors/color-space-conversion-processor.cpp
  src/processors/background-subtraction-processor.cpp
  src/processors/foreground-coverage-processor.cpp
  src/processors/foreground-histogram-processor.cpp
  src/processors/model-debug-processor.cpp
  src/processors/model-occlusion-processor.cpp
  src/processors/norm-dist-bg-sub-processor.cpp
  src/processors/threshold-processor.cpp
  src/sinks/display-sink.cpp
  src/utils/timer.cpp
  src/gles-utils/gles-context.cpp
  src/gles-utils/shader-builder.cpp
  src/gles-utils/glsl-program-builder.cpp
  src/gles-utils/texture-container.cpp
  src/gles-utils/dump-to-image.cpp
)

add_library(
  glipf STATIC EXCLUDE_FROM_ALL
  ${GLIPF_HEADERS}
  ${GLIPF_SOURCES}
)

target_include_directories(
  glipf PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  glipf
  rt # Needed for clock_gettime
  ${GLESV2_LIBRARY}
  ${EGL_LIBRARY}
  ${OpenCV_LIBS}
  ${V4L2_LIBRARIES}
)

install(DIRECTORY src/glsl/ DESTINATION glsl)

PROJECT(SMARTCAMERA_APPLICATION)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03 -Wall -Wextra")

add_subdirectory(../../rpc ${CMAKE_CURRENT_BINARY_DIR}/thrift-gen-cpp)

INCLUDE(${CMAKE_ROOT}/Modules/FindQt4.cmake)
INCLUDE(${SMARTCAMERA_SOURCE_DIR}/Modules/Findlibgsl.cmake)
INCLUDE(${SMARTCAMERA_SOURCE_DIR}/Modules/FindEigen3.cmake)
FIND_PACKAGE(OpenCV REQUIRED )

INCLUDE_DIRECTORIES(SYSTEM
	${QT_INCLUDES}
	${OPENCV_INCLUDES}
	${EIGEN3_INCLUDE_DIR}
)

SET(HDRS
	detection/Detector.h
	target/Target.h
	tracking/ParticleFilter.h
	tracking/Tracker.h
	application/Application.h
        middleware/IpuInterface.h
)

SET(SRCS
	detection/Detector.cpp
	target/Target.cpp
	tracking/ParticleFilter.cpp
	tracking/Tracker.cpp
	application/Application.cpp
        middleware/IpuInterface.cpp
	main.cpp
)	

ADD_EXECUTABLE(
        smartcamera
        ${HDRS}
        ${SRCS}
)
TARGET_INCLUDE_DIRECTORIES(smartcamera PRIVATE
  ${SMARTCAMERA_APPLICATION_BINARY_DIR}
  ${SMARTCAMERA_APPLICATION_SOURCE_DIR}
)
TARGET_LINK_LIBRARIES(
	smartcamera
	${QT_QTCORE_LIBRARY}
	${OpenCV_LIBS}
        ${LIBGSL_LIBRARIES}
        thrift
        glipf-thrift-lib
)

INSTALL(TARGETS smartcamera DESTINATION bin)

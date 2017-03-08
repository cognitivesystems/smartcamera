# smartcamera
Code for the Raspberry pi based Image Processing Unit and 2 example applications

## Build Instructions ##

The project consists of a library implementing low-level image
processing algorithms (GLIPF) and 2 high-level computer vision
applications utilising it: a human tracker, and a 2D object tracker.
Each of the applications has 2 components: a server that runs on
a Raspberry Pi and provides image processing as a service, and a client
that communicates with the server and runs high-level algorithms on data
obtained from it. As all packages that make up the project have slightly
different dependencies, this page documents what is required to build
each one of them.

Several placeholder variables are used throughout this document:
- `$RASPBERRY_PI` refers to the directory where the Raspberry Pi's
  filesystem is mounted
- `$PROJECT_ROOT` refers to the directory containing the project's
  source code

### GL Image Processing Framework ###

GLIPF is designed to run on the Raspberry Pi. It requires a compiler
supporting C++11 (e.g. GCC 4.8 or newer) and the following libraries to
build:
- OpenGL ES 2.0
- EGL >=1.4
- OpenCV >=2.3
- Boost (Optional and Variant) >=1.50
- Video4Linux2 >=1.0

The Boost libraries are header-only and aren't needed at run-time.

While GLIPF can be built on its own as a shared or static library, this
usually isn't necessary: the server applications that include it compile
and link against it as part of their build process.

### 2D Object Tracker ###

#### Server ####

The 2D Object Tracker's server is designed to run on the Raspberry Pi.
It requires a compiler supporting C++11 (e.g. GCC 4.8 or newer) and the
following libraries to build:
- GLIPF
- Apache Thrift >=0.9.1 (with the non-blocking server enabled)
- Broadcom hardware interface library (`libbcm_host`)
- Boost (PropertyTree) >=1.50

The Boost library is header-only and isn't needed at run-time.

While the server could in theory be built directly on the Raspberry Pi,
Raspbian doesn't ship with a compiler supporting C++11. As a result,
[cross-compilation](cross-compilation.md) is the recommented way of
building the application:

    mkdir 2d-object-tracking-server-build
    cd 2d-object-tracking-server-build
    cmake -D CMAKE_TOOLCHAIN_FILE=Toolchain-RaspberryPi-Raspbian.cmake -D CMAKE_INSTALL_PREFIX=$RASPBERRY_PI/2d-object-tracking $PROJECT_ROOT/2d-object-tracking/server/
    make install

Once the server application has been built and installed to the
Raspberry Pi, it can be configured and run:

    cp $PROJECT_ROOT/2d-object-tracking/server/cfg/server.json $RASPBERRY_PI/2d-object-tracking/
    gedit $RASPBERRY_PI/2d-object-tracking/server.json # edit the server's configuration file
    ./2d-object-tracking-server # Execute on the Raspberry Pi

#### Client ####

The 2D Object Tracker's client application can run on any Linux
computer, be it the Raspberry Pi, or a standard PC. To build it,
a compiler supporting C++03 and the following libraries are needed:
- Apache Thrift >=0.9.1
- Qt (only the Core module is required) 4.8
- Eigen >= 3.1.0
- GNU Scientific Library >= 1.15
- OpenCV >=2.3

The client can either be built directly on the computer it is to run on,
or cross-compiled if the Raspberry Pi is chosen to be the deployment
target. In the former case the build steps would be:

    mkdir 2d-object-tracking-client-build
    cd 2d-object-tracking-client-build
    cmake $PROJECT_ROOT/2d-object-tracking/client/
    make

Once the client application has been built, it can be configured and
run:

    cp $PROJECT_ROOT/2d-object-tracking/client/configData/config.ini ./
    gedit config.ini # edit the client's configuration file
    ./objdetect

### Human Tracker ###

#### Server ####

The Human Tracker's server is designed to run on the Raspberry Pi.
It requires a compiler supporting C++11 (e.g. GCC 4.8 or newer) and the
following libraries to build:
- GLIPF
- Apache Thrift >=0.9.1 (with the non-blocking server enabled)
- Broadcom hardware interface library (`libbcm_host`)
- Boost (PropertyTree) >=1.50

The Boost library is header-only and isn't needed at run-time.

While the server could in theory be built directly on the Raspberry Pi,
Raspbian doesn't ship with a compiler supporting C++11. As a result,
[cross-compilation](cross-compilation.md) is the recommented way of
building the application:

    mkdir human-tracking-server-build
    cd human-tracking-server-build
    cmake -D CMAKE_TOOLCHAIN_FILE=Toolchain-RaspberryPi-Raspbian.cmake -D CMAKE_INSTALL_PREFIX=$RASPBERRY_PI/human-tracking $PROJECT_ROOT/human-tracking/server/
    make install

Once the server application has been built, it can be configured and
run:

    cp $PROJECT_ROOT/human-tracking/server/cfg/server.json $RASPBERRY_PI/human-tracking/
    gedit $RASPBERRY_PI/human-tracking/server.json # edit the server's configuration file
    ./human-tracking-server # Execute on the Raspberry Pi

#### Client ####

The Human Tracker's client application can run on any Linux computer
with an X server, be it the Raspberry Pi, or a standard PC. To build it,
a compiler supporting C++03 and the following libraries are needed:
- Apache Thrift >=0.9.1
- Qt (only the Core module is required) 4.8
- Eigen >= 3.1.0
- GNU Scientific Library >= 1.15
- OpenCV >=2.3

The client must be built directly on the computer it is to run on;
cross-compilation is currently not supported. The build steps would
usually be:

    mkdir human-tracking-client-build
    cd human-tracking-client-build
    cmake $PROJECT_ROOT/human-tracking/client/
    make

Once the server application has been built and installed to the
Raspberry Pi, it can be configured and run:

    cp $PROJECT_ROOT/human-tracking/client/configData/config.ini ./
    gedit config.ini # edit the client's configuration file
    src/smartcamera


## Cross-Compiling for Raspberry Pi using CMake on Linux ##

Raspberry Pi is a great device with a (relatively) powerful GPU, but due
to its low-end CPU compiling C/C++ applications on the machine takes too
long to be feasible for anything but the simplest programs. Luckily,
setting up a cross-compiling toolchain that produces binaries compatible
with Raspberry Pi is a rather straightforward affair on Linux.

### Toolchain Setup ###

First of all, create a new directory where the toolchain and other files
required for cross-compilation will be stored:

    mkdir ~/rpi
    cd ~/rpi

The Raspberry Pi foundation maintains a repository with tools for
cross-compilation. Let's clone it:

    git clone https://github.com/raspberrypi/tools.git

The rest of this document assumes a 64-bit host system and uses the
toolchain located in
`tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/`. For
a 32-bit host system the toolchain located in
`tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/` should be
used instead.

At this stage it would be possible to compile some simple programs, but
any useful application will need access to headers and libraries from
the target system. They can be copied from an SD card containing
Raspbian (Arch Linux ARM or Pidora should work too). Assuming the SD
card is mounted at `$RASPBIAN_FS`, the following commands will copy the
required files:

    mkdir -p rootfs/opt
    cp $RASPBIAN_FS/lib/ rootfs/ -rv
    cp $RASPBIAN_FS/usr/ rootfs/ -rv
    cp $RASPBIAN_FS/opt/vc rootfs/opt/ -rv

### CMake Setup ###

CMake is a build system that's very popular among C/C++ developers and
has good support for cross-compilation. The recommended way to configure
it to cross-compile for a specific platform is to provide it with
a toolchain file. Let's create such a file for the toolchain we
downloaded:

~~~
# Save this file as Toolchain-RaspberryPi-Raspbian.cmake
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# Specify the cross compiler
SET(CMAKE_C_COMPILER $ENV{HOME}/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER $ENV{HOME}/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-g++)

# Where is the target environment
SET(CMAKE_FIND_ROOT_PATH $ENV{HOME}/rpi/rootfs)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")

# Search for programs only in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers only in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Raspberry Pi-specific include directories
INCLUDE_DIRECTORIES(SYSTEM $ENV{HOME}/rpi/rootfs/usr/include)
INCLUDE_DIRECTORIES(SYSTEM $ENV{HOME}/rpi/rootfs/usr/local/include)
INCLUDE_DIRECTORIES(SYSTEM $ENV{HOME}/rpi/rootfs/opt/vc/include)
INCLUDE_DIRECTORIES(SYSTEM $ENV{HOME}/rpi/rootfs/opt/vc/include/interface/vcos/pthreads)
INCLUDE_DIRECTORIES(SYSTEM $ENV{HOME}/rpi/rootfs/opt/vc/include/interface/vmcs_host/linux)
LIST(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/rpi/rootfs/opt/vc")

# Adjust linker flags for cross-compilation
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --sysroot=${CMAKE_FIND_ROOT_PATH}")

# Setup pkg-config
SET(PKG_CONFIG_EXECUTABLE $ENV{HOME}/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-pkg-config-real)
SET(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_FIND_ROOT_PATH}")
SET(ENV{PKG_CONFIG_PATH} "${CMAKE_FIND_ROOT_PATH}/usr/lib/pkgconfig/:${CMAKE_FIND_ROOT_PATH}/usr/share/pkgconfig/:${CMAKE_FIND_ROOT_PATH}/usr/lib/arm-linux-gnueabihf/pkgconfig/")
~~~

Now we're all set to start cross-compiling CMake-based projects for
Raspberry Pi. If you have such a project handy, create a new build
directory and start CMake, pointing it to the toolchain file:

    mkdir build
    cd build
    cmake -D CMAKE_BUILD_TYPE=Debug -D CMAKE_TOOLCHAIN_FILE=$HOME/rpi/Toolchain-RaspberryPi-Raspbian.cmake path/to/your/project

Unless CMake can't find some libraries in the target environment, you
should end up with a Makefile and be able to compile the project using
`make`.

### Summary ###

When developing for such low-power devices as Raspberry Pi,
cross-compilation is usually a must. Otherwise builds — even incremental
— take so much time that continuous testing of code changes becomes
difficult. The good thing is that nowadays it's quite easy to set up
a toolchain to cross-compile for Raspberry Pi, especially if one uses
Linux with CMake.


@namespace glipf::sources

Frame sources provide image data to be processed. The framework supports
2 types of frame sources: video files (OpenCvVideoSource) and cameras
(V4L2Camera and OpenCvCamera). The former are useful for testing, the
latter are needed in real-world scenarios.

The *GL Image Processing Framework* is a collection of low-level image
processing algorithms implemented using OpenGL ES 2.0-based GPGPU
techniques, along with utilities for acquiring, displaying, and saving
images. This page briefly introduces GLIPF's major components, and links
to their detailed documentation.

[Build instructions](build-instructions.md) and
[cross-compilation information](cross-compilation.md) can be found on
separate pages.

### Frame Sources ###

Frame sources provide image data to be processed. The framework supports
2 types of frame sources: video files and cameras. The former are useful
for testing, the latter are needed in real-world scenarios.

### Image Processors ###

Image processors are at the core of the framework: they implement
low-level image processing algorithms. They accept data returned by
frame sources as input, and output high-level information extracted from
it.

### Data Sinks ###

Data sinks are responsible for sharing data produced by the framework
with the outside world. To achieve that goal, they may publish the data
on a network, display or save it.


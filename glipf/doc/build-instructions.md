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

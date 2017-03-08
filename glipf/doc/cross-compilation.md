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

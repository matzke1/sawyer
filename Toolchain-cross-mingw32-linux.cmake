# CMake toolchain file for compiling Windows executables on Linux
# When running "cmake" add -DCMAKE_TOOLCHAIN_FILE=...<this file> -DCMAKE_INSTALL_PREFIX=... etc.
# This file is based on http://www.cmake.org/Wiki/CmakeMingw retrieved 2014-06-12 (last modified 2012-07-04)

# The name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# Which compilers to use for C and C++
SET(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
SET(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)
SET(CMAKE_RC_COMPILER i586-mingw32msvc-windres)

# Target environment location. Multiple directories possible.
SET(CMAKE_FIND_ROOT_PATH  /usr/i586-mingw32msvc /home/matzke/lib-mingw32)

# Adjust the default behaviour of the FIND_XXX() commands:
# Search headers and libraries in the target environment, search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

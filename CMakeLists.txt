cmake_minimum_required(VERSION 2.8)
project(Sawyer)

# Set either the build type or the G++ flags (i.e., cmake -DCMAKE_BUILD_TYPE=Debug ...) or else choose a default here
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Release)
endif()

# Extra optimization switches for GCC compilers
if(CMAKE_COMPILER_IS_GNUCXX AND (${CMAKE_BUILD_TYPE} STREQUAL "Release"))
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fomit-frame-pointer -DNDEBUG")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")  ## Strip binary
endif()

# Extra warning switches for GCC compilers
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-local-typedefs")
endif()

# What is the system thread library?
option(MULTI_THREADED, "Should library be multi-thread aware?" OFF)
if(NOT "${MULTI_THREADED}")
  message(STATUS "Multi-thread support is disabled")
  find_package(Boost 1.47.0 REQUIRED COMPONENTS iostreams system filesystem regex chrono)
else()
  message(STATUS "Multi-thread support is enabled")
  find_package(Threads)
  if( ${CMAKE_CXX_COMPILER_ID} MATCHES GNU )
    message(STATUS "  adding -pthread to compile commands")
    add_definitions(-pthread)
  elseif( ${CMAKE_CXX_COMPILER_ID} MATCHES Clang )
    message(STATUS "  adding -pthread to compile commands")
    add_definitions(-pthread)
  else()
    message(STATUS "not adding -pthread")
  endif()
  find_package(Boost 1.47.0 REQUIRED COMPONENTS iostreams system filesystem regex chrono thread)
endif()


# Microsoft Visual Studio needs to be told where the boost libraries are located or else you'll get
# errors like "fatal error LNK1104: cannot open file 'libboost_system-vc90-mt-gd-1_44.lib'" even though cmake was
# able to find the libraries.
link_directories(${LINK_DIRECTORIES} ${Boost_LIBRARY_DIRS})

# We need additional libraries when compiling with MinGW.
if(${MINGW})
  find_library(libpsapi psapi)
  find_library(libshlwapi shlwapi)
  set(Windows_LIBRARIES ${libpsapi} ${libshlwapi})
else()
  set(Windows_LIBRARIES "")
endif()

# libsawyer
set(lib_src
  Sawyer/Assert.C
  Sawyer/Attribute.C
  Sawyer/CommandLine.C
  Sawyer/GraphTraversal.C
  Sawyer/LineVector.C
  Sawyer/Markup.C
  Sawyer/MarkupPod.C
  Sawyer/Message.C
  Sawyer/ProgressBar.C
  Sawyer/Sawyer.C
  Sawyer/SmallObject.C
  Sawyer/Stopwatch.C
  Sawyer/Synchronization.C)
add_library(libsawyer SHARED ${lib_src})
target_link_libraries(libsawyer ${Boost_LIBRARIES} ${Windows_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(libsawyer PROPERTIES DEFINE_SYMBOL "SAWYER_DO_EXPORTS")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_subdirectory(docs/examples)
add_subdirectory(tests/Basic)
add_subdirectory(tests/Allocation)
add_subdirectory(tests/Pointers)
add_subdirectory(tests/Container)
add_subdirectory(tests/Message)
add_subdirectory(tests/Assert)
add_subdirectory(tests/ProgressBar)
add_subdirectory(tests/Markup)
add_subdirectory(tests/CommandLine)
add_subdirectory(tests/Threads)
add_subdirectory(tools)

#-------------------------------------------------------------------------------
#                     _              _           _        _ _
#                    | |            (_)         | |      | | |
#     _ __ ___   __ _| | _____       _ _ __  ___| |_ __ _| | |
#    | '_ ` _ \ / _` | |/ / _ \     | | '_ \/ __| __/ _` | | |
#    | | | | | | (_| |   <  __/     | | | | \__ \ || (_| | | |
#    |_| |_| |_|\__,_|_|\_\___|     |_|_| |_|___/\__\__,_|_|_|
#
#-------------------------------------------------------------------------------
install(TARGETS libsawyer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# Install all header files
file(GLOB header_files "${CMAKE_CURRENT_SOURCE_DIR}/Sawyer/*.h")
install(FILES ${header_files} DESTINATION include/Sawyer)

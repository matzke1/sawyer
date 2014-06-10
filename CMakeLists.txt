cmake_minimum_required(VERSION 2.8)
project(Sawyer)

# Set either the build type or the G++ flags
set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "-O3 -fomit-frame-pointer -DNDEBUG")
#    set(CMAKE_EXE_LINKER_FLAGS "-s")  ## Strip binary
#endif()

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

find_package(PkgConfig)
find_package(Boost 1.47.0 REQUIRED COMPONENTS iostreams system filesystem regex chrono)

set(lib_src
  sawyer/Assert.C
  sawyer/CommandLine.C
  sawyer/Markup.C
  sawyer/MarkupRoff.C
  sawyer/Message.C
  sawyer/ProgressBar.C
  sawyer/Sawyer.C
  sawyer/SmallObject.C
  sawyer/Stopwatch.C)
add_library(sawyer SHARED ${lib_src})

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_subdirectory(tests/Container)
add_subdirectory(tests/Message)
add_subdirectory(tests/Assert)
add_subdirectory(tests/ProgressBar)
add_subdirectory(tests/CommandLine)
add_subdirectory(tests/Allocation)
add_subdirectory(tests/Pointers)

#-------------------------------------------------------------------------------
#                     _              _           _        _ _
#                    | |            (_)         | |      | | |
#     _ __ ___   __ _| | _____       _ _ __  ___| |_ __ _| | |
#    | '_ ` _ \ / _` | |/ / _ \     | | '_ \/ __| __/ _` | | |
#    | | | | | | (_| |   <  __/     | | | | \__ \ || (_| | | |
#    |_| |_| |_|\__,_|_|\_\___|     |_|_| |_|___/\__\__,_|_|_|
#
#-------------------------------------------------------------------------------
install(TARGETS sawyer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# Install all header files
file(GLOB header_files "${CMAKE_CURRENT_SOURCE_DIR}/sawyer/*.h")
install(FILES ${header_files} DESTINATION include/sawyer)

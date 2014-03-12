cmake_minimum_required(VERSION 2.8)
project(Sawyer)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

find_package(PkgConfig)
find_package(Boost 1.47.0 REQUIRED COMPONENTS system filesystem regex signals)

set(lib_src
  src/Assert.C
  src/CommandLine.C
  src/Markup.C
  src/MarkupRoff.C
  src/Message.C
  src/ProgressBar.C
  src/Sawyer.C)
add_library(sawyer SHARED ${lib_src})

include_directories(
  ${Boost_INCLUDE_DIRS}
  src
)

add_subdirectory(tests/Message)
add_subdirectory(tests/Assert)
add_subdirectory(tests/ProgressBar)
add_subdirectory(tests/CommandLine)

#-------------------------------------------------------------------------------
#                     _              _           _        _ _
#                    | |            (_)         | |      | | |
#     _ __ ___   __ _| | _____       _ _ __  ___| |_ __ _| | |
#    | '_ ` _ \ / _` | |/ / _ \     | | '_ \/ __| __/ _` | | |
#    | | | | | | (_| |   <  __/     | | | | \__ \ || (_| | | |
#    |_| |_| |_|\__,_|_|\_\___|     |_|_| |_|___/\__\__,_|_|_|
#
#-------------------------------------------------------------------------------
INSTALL(TARGETS sawyer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# Install all header files
FILE(GLOB header_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${header_files} DESTINATION include)

cmake_minimum_required(VERSION 2.8)
project(Sawyer)
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig)
find_package(Boost 1.47.0 REQUIRED COMPONENTS system filesystem regex signals)

set(lib_src Assert.C Message.C ProgressBar.C Sawyer.C)
add_library(sawyer SHARED ${lib_src})

include_directories(
  ${Boost_INCLUDE_DIRS}
)

add_executable(test1 test1.C)
target_link_libraries(test1 sawyer)

add_executable(test2 test2.C)
target_link_libraries(test2 sawyer)

add_executable(test3 test3.C)
target_link_libraries(test3 sawyer)

add_executable(test4 test4.C)
target_link_libraries(test4 sawyer)

add_executable(test5 test5.C)
target_link_libraries(test5 sawyer)

add_executable(test6 test6.C)
target_link_libraries(test6 sawyer)

add_executable(test7 test7.C)
target_link_libraries(test7 sawyer)

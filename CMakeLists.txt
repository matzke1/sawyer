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

add_executable(tests/CommandLine/test1 tests/CommandLine/test1.C)
target_link_libraries(tests/CommandLine/test1 sawyer ${Boost_LIBRARIES})

add_executable(test1 test1.C)
target_link_libraries(test1 sawyer ${Boost_LIBRARIES})

add_executable(test2 test2.C)
target_link_libraries(test2 sawyer ${Boost_LIBRARIES})

add_executable(test3 test3.C)
target_link_libraries(test3 sawyer ${Boost_LIBRARIES})

add_executable(test4 test4.C)
target_link_libraries(test4 sawyer ${Boost_LIBRARIES})

add_executable(test5 test5.C)
target_link_libraries(test5 sawyer ${Boost_LIBRARIES})

add_executable(test6 test6.C)
target_link_libraries(test6 sawyer ${Boost_LIBRARIES})

add_executable(test7 test7.C)
target_link_libraries(test7 sawyer ${Boost_LIBRARIES})

add_executable(test8 test8.C)
target_link_libraries(test8 sawyer ${Boost_LIBRARIES})

add_executable(test9 test9.C)
target_link_libraries(test9 sawyer ${Boost_LIBRARIES})

add_executable(test10 test10.C)
target_link_libraries(test10 sawyer ${Boost_LIBRARIES})

add_executable(test11 test11.C)
target_link_libraries(test11 sawyer ${Boost_LIBRARIES})

add_executable(test12 test12.C)
target_link_libraries(test12 sawyer ${Boost_LIBRARIES})

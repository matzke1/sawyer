cmake_minimum_required(VERSION 2.8)
project(Sawyer)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

find_package(PkgConfig)
find_package(Boost 1.47.0 REQUIRED COMPONENTS system filesystem regex signals)

set(lib_src Assert.C CommandLine.C Markup.C MarkupRoff.C Message.C ProgressBar.C Sawyer.C)
add_library(sawyer SHARED ${lib_src})

include_directories(
  ${Boost_INCLUDE_DIRS}
)

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


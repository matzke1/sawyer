add_executable(cmdline_demo1 demo1.C)
target_link_libraries(cmdline_demo1 sawyer ${Boost_LIBRARIES})

add_executable(cmdline_docTest docTest.C)
target_link_libraries(cmdline_docTest sawyer ${Boost_LIBRARIES})

add_executable(cmdline_unitTests unitTests.C)
target_link_libraries(cmdline_unitTests sawyer ${Boost_LIBRARIES})

add_executable(cmdline_demo1 demo1.C)
target_link_libraries(cmdline_demo1 sawyer)

add_executable(cmdline_docTest docTest.C)
target_link_libraries(cmdline_docTest sawyer)

add_executable(cmdUnitTests cmdUnitTests.C)
target_link_libraries(cmdUnitTests sawyer)

add_executable(cmdline_grepExample grepExample.C)
target_link_libraries(cmdline_grepExample sawyer)

#add_executable(cmdline_codethorn codethorn.C codethorn_CommandLineOptions.C)
#target_link_libraries(cmdline_codethorn sawyer)

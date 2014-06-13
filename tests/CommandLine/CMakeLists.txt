add_executable(cmdline_demo1 demo1.C)
target_link_libraries(cmdline_demo1 libsawyer)

add_executable(cmdline_docTest docTest.C)
target_link_libraries(cmdline_docTest libsawyer)

add_executable(cmdline_unitTests unitTests.C)
target_link_libraries(cmdline_unitTests libsawyer)

add_executable(cmdline_grepExample grepExample.C)
target_link_libraries(cmdline_grepExample libsawyer)

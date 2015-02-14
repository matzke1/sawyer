add_executable(ptrUnitTests ptrUnitTests.C)
target_link_libraries(ptrUnitTests libsawyer)

add_executable(ptrMultiThread ptrMultiThread.C)
target_link_libraries(ptrMultiThread libsawyer)

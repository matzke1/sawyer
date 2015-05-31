add_executable(basicTest1 basicTest1.C)
target_link_libraries(basicTest1 libsawyer)

add_executable(basicTest2 basicTest2.C)
target_link_libraries(basicTest2 libsawyer)

add_executable(geometryUnitTests geometryUnitTests.C)
target_link_libraries(geometryUnitTests libsawyer)

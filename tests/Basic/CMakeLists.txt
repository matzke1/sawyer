add_executable(basicTest1 basicTest1.C)
target_link_libraries(basicTest1 sawyer)

add_executable(basicTest2 basicTest2.C)
target_link_libraries(basicTest2 sawyer)

add_executable(geometryUnitTests geometryUnitTests.C)
target_link_libraries(geometryUnitTests sawyer)

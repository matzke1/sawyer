add_executable(threadWorkerTests threadWorkerTests.C)
target_link_libraries(threadWorkerTests sawyer)

add_executable(cachableDemo cachableDemo.C)
target_link_libraries(cachableDemo sawyer)

add_executable(workListTests workListTests.C)
target_link_libraries(workListTests sawyer)

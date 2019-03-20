add_executable(mesg_test1 test1.C)
target_link_libraries(mesg_test1 sawyer)

add_executable(mesg_test4 test4.C)
target_link_libraries(mesg_test4 sawyer)

add_executable(mesg_test5 test5.C)
target_link_libraries(mesg_test5 sawyer)

add_executable(mesg_test6 test6.C)
target_link_libraries(mesg_test6 sawyer)

add_executable(mesg_test7 test7.C)
target_link_libraries(mesg_test7 sawyer)

add_executable(mesg_test8 test8.C)
target_link_libraries(mesg_test8 sawyer)

add_executable(mesgUnitTests mesgUnitTests.C)
target_link_libraries(mesgUnitTests sawyer)

add_executable(mesgMultithreadTests mesgMultithreadTests.C)
target_link_libraries(mesgMultithreadTests sawyer)

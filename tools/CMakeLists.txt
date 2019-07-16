add_executable(symbolFreq symbolFreq.C)
target_link_libraries(symbolFreq sawyer)

add_executable(searchCode searchCode.C)
target_link_libraries(searchCode sawyer)

add_executable(findIncludes findIncludes.C)
target_link_libraries(findIncludes sawyer)

add_executable(stringifyEnums stringifyEnums.C)
target_link_libraries(stringifyEnums sawyer)

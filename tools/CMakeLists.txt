add_executable(symbolFreq symbolFreq.C)
target_link_libraries(symbolFreq libsawyer)

add_executable(searchCode searchCode.C)
target_link_libraries(searchCode libsawyer)

add_executable(findIncludes findIncludes.C)
target_link_libraries(findIncludes libsawyer)

add_executable(stringifyEnums stringifyEnums.C)
target_link_libraries(stringifyEnums libsawyer)

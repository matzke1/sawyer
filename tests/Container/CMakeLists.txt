add_executable(optionalUnitTests optionalUnitTests.C)
target_link_libraries(optionalUnitTests libsawyer)

add_executable(mapUnitTests mapUnitTests.C)
target_link_libraries(mapUnitTests libsawyer)

add_executable(listUnitTests listUnitTests.C)
target_link_libraries(listUnitTests libsawyer)

add_executable(graphUnitTests graphUnitTests.C)
target_link_libraries(graphUnitTests libsawyer)

add_executable(graphBoost graphBoost.C)
target_link_libraries(graphBoost libsawyer)

add_executable(intervalUnitTests intervalUnitTests.C)
target_link_libraries(intervalUnitTests libsawyer)

add_executable(addressMapUnitTests addressMapUnitTests.C)
target_link_libraries(addressMapUnitTests libsawyer)

add_executable(bitvecTests bitvecTests.C)
target_link_libraries(bitvecTests libsawyer)

add_executable(distinctListUnitTests distinctListUnitTests.C)
target_link_libraries(distinctListUnitTests libsawyer)

add_executable(setUnitTests setUnitTests.C)
target_link_libraries(setUnitTests libsawyer)

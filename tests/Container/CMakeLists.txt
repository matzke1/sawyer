add_executable(optionalUnitTests optionalUnitTests.C)
target_link_libraries(optionalUnitTests sawyer ${Boost_LIBRARIES})

add_executable(mapUnitTests mapUnitTests.C)
target_link_libraries(mapUnitTests sawyer ${Boost_LIBRARIES})

add_executable(listUnitTests listUnitTests.C)
target_link_libraries(listUnitTests sawyer ${Boost_LIBRARIES})

add_executable(graphUnitTests graphUnitTests.C)
target_link_libraries(graphUnitTests sawyer ${Boost_LIBRARIES})

add_executable(graphBoost graphBoost.C)
target_link_libraries(graphBoost sawyer ${Boost_LIBRARIES})

add_executable(intervalUnitTests intervalUnitTests.C)
target_link_libraries(intervalUnitTests sawyer ${Boost_LIBRARIES})

add_executable(addressMapUnitTests addressMapUnitTests.C)
target_link_libraries(addressMapUnitTests sawyer ${Boost_LIBRARIES})

add_executable(bitvecTests bitvecTests.C)
target_link_libraries(bitvecTests sawyer ${Boost_LIBRARIES})

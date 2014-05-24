add_executable(poolPerf poolPerf.C)
target_link_libraries(poolPerf sawyer ${Boost_LIBRARIES})

add_executable(smallObjectPerf smallObjectPerf.C)
target_link_libraries(smallObjectPerf sawyer ${Boost_LIBRARIES})

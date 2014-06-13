add_executable(poolPerf poolPerf.C)
target_link_libraries(poolPerf libsawyer)

add_executable(smallObjectPerf smallObjectPerf.C)
target_link_libraries(smallObjectPerf libsawyer)

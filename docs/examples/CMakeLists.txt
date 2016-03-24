add_executable(commandLineEx1 commandLineEx1.C)
target_link_libraries(commandLineEx1 libsawyer)

add_executable(attribute attribute.C)
target_link_libraries(attribute libsawyer)

add_executable(graphIso graphIso.C)
target_link_libraries(graphIso libsawyer)

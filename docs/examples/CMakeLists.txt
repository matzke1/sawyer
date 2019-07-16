add_executable(commandLineEx1 commandLineEx1.C)
target_link_libraries(commandLineEx1 sawyer)

add_executable(attribute attribute.C)
target_link_libraries(attribute sawyer)

add_executable(graphIso graphIso.C)
target_link_libraries(graphIso sawyer)

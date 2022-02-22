aux_source_directory(. cfiles)
add_library(net ${cfiles})
target_link_libraries(net ssl crypto c m)

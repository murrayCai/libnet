add_executable(test_client client.c)
target_link_libraries(test_client mc net)

add_executable(test_server server.c)
target_link_libraries(test_server mc net)

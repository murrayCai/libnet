add_executable(echo_client echo_tcp_client.c)
target_link_libraries(echo_client mc net)

add_executable(echo_server echo_tcp_server.c)
target_link_libraries(echo_server mc net)

add_executable(pm_server pm_server.c)
target_link_libraries(pm_server mc net)

add_executable(pm_client pm_client.c)
target_link_libraries(pm_client mc net)


add_executable(timer_client echo_timer_client.c)
target_link_libraries(timer_client mc net)

add_executable(timer_server echo_timer_server.c)
target_link_libraries(timer_server mc net)


add_executable(pm_timer_client pm_timer_client.c)
target_link_libraries(pm_timer_client mc net)

add_executable(pm_timer_server pm_timer_server.c)
target_link_libraries(pm_timer_server mc net)

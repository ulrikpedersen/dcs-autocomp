cmake_minimum_required(VERSION 3.15)
project(autocompletepv C)

set(CMAKE_C_STANDARD 99)
find_package(Threads REQUIRED)
#set(CMAKE_C_FLAGS -pthread)

add_executable(dcs_compgen main.c db_rec_names.c)
target_link_libraries(dcs_compgen PRIVATE Threads::Threads)

cmake_minimum_required(VERSION 3.9)
project(autocompletepv C)

set(CMAKE_C_STANDARD 99)
find_package(Threads REQUIRED)
#set(CMAKE_C_FLAGS -pthread)

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
  # Add profiling (gprof) flags
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -p")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -p")
ENDIF()

add_executable(dcs_compgen main.c db_rec_names.c)
target_link_libraries(dcs_compgen PRIVATE Threads::Threads)
install(TARGETS dcs_compgen RUNTIME DESTINATION bin)
install(FILES complete_console.sh DESTINATION bin RENAME .dcs_complete_console.sh)

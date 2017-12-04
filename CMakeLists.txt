cmake_minimum_required(VERSION 3.6.2)
project(smb-tools)

#Export compile commands for editor autocomplete
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Add uninstall target
configure_file("./cmake/uninstall.cmake" "./cmake/uninstall.cmake" COPYONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "cmake/uninstall.cmake")

add_subdirectory(./smb-build-replay)


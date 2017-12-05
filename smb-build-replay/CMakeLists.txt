cmake_minimum_required(VERSION 3.6.2)
project(smb-build-replay)

#Use C++ 14
set(CMAKE_CXX_STANDARD 14)

#Export compile commands for editor autocomplete
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Be really pedantic!
add_definitions(-Wall -Wextra -pedantic)

#Show as an executable, not a shared library in file managers
if(UNIX)
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
endif(UNIX)

#External dependencies
find_package(Boost REQUIRED COMPONENTS program_options)

include_directories(.)

set(SOURCE_FILES
    ./smb-build-replay.cpp
    )

set(HEADER_FILES
    ./json.hpp
    )

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} Boost::program_options)

if(WIN32)
    #Windows has no concept of rpath, so just group all the exes/dlls in one big mess of a directory
    install(TARGETS ${PROJECT_NAME} DESTINATION .)
else(WIN32)
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
endif(WIN32)


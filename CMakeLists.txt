cmake_minimum_required(VERSION 3.16)
project(SmartTrafficManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG QUIET)

if(TARGET raylib)
    set(TRAFFIC_RAYLIB_TARGET raylib)
elseif(TARGET raylib::raylib)
    set(TRAFFIC_RAYLIB_TARGET raylib::raylib)
elseif(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/raylib/lib/libraylib.a")
    add_library(traffic_raylib STATIC IMPORTED)
    set_target_properties(traffic_raylib PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/raylib/lib/libraylib.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/raylib/include"
    )
    set(TRAFFIC_RAYLIB_TARGET traffic_raylib)
else()
    message(FATAL_ERROR
        "Raylib was not found. Install raylib with vcpkg or provide the "
        "MinGW Raylib package in the project's raylib/ directory.")
endif()

add_executable(smart_traffic
    main.cpp
    Graph.cpp
    Dijkstra.cpp
    Vehicle.cpp
    TrafficSignal.cpp
    Simulation.cpp
)
target_link_libraries(smart_traffic PRIVATE ${TRAFFIC_RAYLIB_TARGET})

if(WIN32)
    target_link_libraries(smart_traffic PRIVATE opengl32 gdi32 winmm)
endif()

enable_testing()
add_executable(data_structure_tests
    tests/data_structure_tests.cpp
    Graph.cpp
    Dijkstra.cpp
)
target_include_directories(data_structure_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME data_structure_tests COMMAND data_structure_tests)

add_executable(simulation_tests
    tests/simulation_tests.cpp
    Graph.cpp
    Dijkstra.cpp
    Vehicle.cpp
    TrafficSignal.cpp
    Simulation.cpp
)
target_include_directories(simulation_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME simulation_tests COMMAND simulation_tests)

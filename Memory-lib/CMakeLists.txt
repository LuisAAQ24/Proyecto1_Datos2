cmake_minimum_required(VERSION 3.16)
project(ProfilerLib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Qt6 REQUIRED COMPONENTS Core Network)


add_library(ProfilerLib STATIC
    memory_instrumentaion.cpp
    memory_instrumentation.h
    ListaGuardado.cpp
    ListaGuardado.h


)


target_link_libraries(ProfilerLib PUBLIC
    Qt6::Core
    Qt6::Network
)

target_include_directories(ProfilerLib PUBLIC .)

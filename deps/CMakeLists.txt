cmake_minimum_required(VERSION 3.20.0)

set(DEPSNAME deps)

project(${DEPSNAME})

add_subdirectory(opm)
add_subdirectory(glfw)

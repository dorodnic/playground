cmake_minimum_required (VERSION 2.6)
project (Playground)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 3.2 REQUIRED)

add_executable(main src/main.cpp)
target_include_directories(main PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(main ${OPENGL_gl_LIBRARY})
target_link_libraries(main glfw)

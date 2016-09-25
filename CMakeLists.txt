cmake_minimum_required (VERSION 2.6)
project (Playground)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(main src/main.cpp 
               src/ui.cpp src/ui.h 
               src/parser.cpp src/parser.h
               src/adaptors.h src/containers.h src/containers.cpp
               src/controls.h src/controls.cpp
               src/types.h src/bind.h src/bind.cpp
               )
target_include_directories(main PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(main ${OPENGL_gl_LIBRARY})
target_link_libraries(main glfw)

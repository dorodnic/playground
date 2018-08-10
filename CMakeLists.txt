cmake_minimum_required (VERSION 2.6)
project (Playground)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(OpenGL REQUIRED)

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    set(RESOURCES resources/ui.xml 
                  resources/shaders/flat2d_fragment.c
                  resources/shaders/flat2d_vertex.c
                  resources/shaders/font_fragment.c
                  resources/shaders/font_vertex.c
                  
                  resources/fonts/v.fnt
                  resources/fonts/vb.fnt
                  resources/fonts/v.png
                  resources/fonts/vb.png
                  )

    foreach(item IN LISTS RESOURCES)
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
        )
    endforeach()
endif()
add_custom_target(resources-target ALL DEPENDS ${RESOURCES})

add_executable(main src/main.cpp 
               src/ui.cpp src/ui.h 
               src/parser.cpp src/parser.h
               src/adaptors.h src/containers.h src/containers.cpp
               src/controls.h src/controls.cpp
               src/types.h src/bind.h src/bind.cpp
               src/serializer.h src/serializer.cpp
               src/font.h src/font.cpp
               src/shader.h src/shader.cpp
               src/flat2d.h src/flat2d.cpp
               src/render.h
               )
add_dependencies(main resources-target)

if(WIN32)
    add_subdirectory(glfw)
    add_subdirectory(glew/build/cmake)

    include_directories(glfw/include glex/include glew/include)
    target_link_libraries(main glfw3 glew_s ${OPENGL_gl_LIBRARY})
else()
    # Find glfw header
	find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h
		PATHS /usr/X11R6/include
			  /usr/include/X11
			  /opt/graphics/OpenGL/include
			  /opt/graphics/OpenGL/contrib/libglfw
			  /usr/local/include
			  /usr/include/GL
			  /usr/include
	)
	# Find glfw library
	find_library(GLFW_LIBRARIES NAMES glfw glfw3
			PATHS /usr/lib64
				  /usr/lib
				  /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
				  /usr/local/lib64
				  /usr/local/lib
				  /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
				  /usr/X11R6/lib
	)

    add_subdirectory(glew/build/cmake)
    include_directories(${GLFW_INCLUDE_DIR} glex/include glew/include)
    target_link_libraries(main glew_s ${GLFW_LIBRARIES} ${OPENGL_gl_LIBRARY})
endif()

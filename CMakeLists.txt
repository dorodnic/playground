cmake_minimum_required (VERSION 2.6)
project (Playground)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(OpenGL REQUIRED)

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

if(WIN32)
    add_subdirectory(glfw)
	target_link_libraries(main glfw3 ${OPENGL_gl_LIBRARY} glew32 glu32)
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
	include_directories(${GLFW_INCLUDE_DIR})
	target_link_libraries(main ${GLFW_LIBRARIES} ${OPENGL_gl_LIBRARY})
endif()

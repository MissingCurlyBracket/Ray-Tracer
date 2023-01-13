cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ComputerGraphics CXX)

# Slightly modified versions of the files from the cpp-starter-project by Jason Turner:
# https://github.com/lefticus/cpp_starter_project/
include("cmake/CompilerWarnings.cmake") # Enable almost all compiler warnings and CMake option to enable -Werror.
include("cmake/Sanitizers.cmake") # CMake options to enable address, memory, UB and thread sanitizers.
include("cmake/StaticAnalyzers.cmake") # CMake options to enable clang-tidy or cpp-check.

if (FRAMEWORK_BASIC_LIBRARY)
	add_library(CGFramework INTERFACE)
	target_include_directories(CGFramework INTERFACE "include/")
	target_compile_features(CGFramework INTERFACE cxx_std_20)
else()
	set(OpenGL_GL_PREFERENCE GLVND) # Prevent CMake warning about legacy fallback on Linux.
	find_package(OpenGL REQUIRED)

	find_package(fmt CONFIG REQUIRED)
	find_package(glm CONFIG REQUIRED)
	find_package(glew CONFIG REQUIRED)
	find_package(glfw3 CONFIG REQUIRED)
	find_package(imgui CONFIG REQUIRED)
	find_package(assimp CONFIG REQUIRED)
	# STB **still** doesn't provide a find_package script :-(
	add_library(stb INTERFACE)
	find_path(STB_INCLUDE_DIRS "stb.h")
	target_link_directories(stb INTERFACE ${STB_INCLUDE_DIRS})

	add_library(CGFramework STATIC
		"src/trackball.cpp"
		"src/mesh.cpp"
		"src/image.cpp"
		"src/shader.cpp"
		"src/window.cpp"
		"src/imguizmo.cpp"
		"src/ImGuizmo/ImGuizmo.cpp"
	)
	target_include_directories(CGFramework PRIVATE "include/framework/" PUBLIC "include/")
	target_link_libraries(CGFramework PUBLIC OpenGL::GL GLEW::GLEW fmt::fmt glm::glm glfw imgui::imgui assimp::assimp stb)
	target_compile_features(CGFramework PUBLIC cxx_std_20)
endif()

# Prevent accidentaly picking up a system-wide or vcpkg install of another loader (e.g. GLEW).
#target_compile_definitions(CGFramework PUBLIC "-DIMGUI_IMPL_OPENGL_LOADER_GLAD=1")

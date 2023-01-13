cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ComputerGraphics CXX)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("framework")
else()
	# During development the framework lives in parent folder.
	add_subdirectory("../../../framework/" "${CMAKE_BINARY_DIR}/framework/")
endif()

find_package(unofficial-nativefiledialog CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(TBB CONFIG REQUIRED)

add_executable(FinalProject
	"src/main.cpp"
	"src/ray_tracing.cpp"
	"src/scene.cpp"
	"src/draw.cpp"
	"src/screen.cpp"
	"src/bounding_volume_hierarchy.cpp")
target_link_libraries(FinalProject PRIVATE CGFramework unofficial::nativefiledialog::nfd OpenGL::GLU TBB::tbb)
target_compile_features(FinalProject PRIVATE cxx_std_20)
enable_sanitizers(FinalProject)
set_project_warnings(FinalProject)

target_compile_definitions(FinalProject PRIVATE
	"-DDATA_DIR=\"${CMAKE_CURRENT_LIST_DIR}/data/\"")

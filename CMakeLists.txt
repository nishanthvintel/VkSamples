cmake_minimum_required( VERSION 3.20 )

option( VULKAN_BUILD_SAMPLES "Build Vulkan Sample applications" ON )

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

project( VkSamples LANGUAGES CXX )

if( MSVC )
	add_compile_options( /MP )
endif()

if ( VULKAN_BUILD_SAMPLES )
	add_subdirectory( src/Clear )

	set_target_properties( Clear PROPERTIES FOLDER Samples )

	set_directory_properties( PROPERTIES VS_STARTUP_PROJECT Clear )
endif( VULKAN_BUILD_SAMPLES )

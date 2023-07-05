cmake_minimum_required( VERSION 3.20 )

set( TARGET_NAME Clear )

option( AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON )

if( AUTO_LOCATE_VULKAN )
	message( STATUS "Attempting to autolocate Vulkan" )
	
	find_package(Vulkan)
	
	if( NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "" )
		set( VULKAN_PATH ${Vulkan_INCLUDE_DIRS} )
		STRING( REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH} )
	endif()
	
	if( NOT VULKAN_FOUND )
		message( STATUS "Failed to locate Vulkan SDK. Retrying again.." )
		if( EXISTS "${VULKAN_PATH}" )
			message( STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}" )
		else()
			message( "ERROR: Unable to locate Vulkan SDK" )
			return()
		endif()
	endif()
	
else()
	message( "ERROR: Could not autolocate Vulkan SDK" )
	return()
endif()

project( ${TARGET_NAME} )

add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

set( VULKAN_LIB_LIST "vulkan-1" )

set( GLSLC "${VULKAN_PATH}/Bin/glslc.exe" )

message( "CMAKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}" )

set( SHADERS_IN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders" )
set( SHADERS_OUT_DIR "${CMAKE_BINARY_DIR}/${TARGET_NAME}/shaders" )

file( GLOB SHADERS "${SHADERS_IN_DIR}/*.vert" "${SHADERS_IN_DIR}/*.frag" )

file( MAKE_DIRECTORY ${SHADERS_OUT_DIR} )

message( "GLSLC ${GLSLC}" )

foreach( SHADER ${SHADERS} )
	get_filename_component( SHADER_NAME ${SHADER} NAME )
	set( SHADER_OUT_NAME "${SHADERS_OUT_DIR}/${SHADER_NAME}.spv" )
	message("${GLSLC} ${SHADER} -o ${SHADER_OUT_NAME}" )
	list( APPEND SHADER_OUT_NAMES ${SHADER_OUT_NAME} )
#	add_custom_command(
#		MAIN_DEPENDENCY ${SHADER}
#		OUTPUT ${SHADER_OUT_NAME}
#		COMMAND ${GLSLC} ${SHADER} " -o " ${SHADER_OUT_NAME}
#		VERBATIM
#	)
	add_custom_command(
		OUTPUT ${SHADER_OUT_NAME}
		COMMAND ${GLSLC} ${SHADER} " -o " ${SHADER_OUT_NAME}
		DEPENDS ${SHADER}
		COMMENT "Compiling SPIRV for ${SHADER}"
		VERBATIM
	)
endforeach()

if( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	include_directories( AFTER ${VULKAN_PATH}/Include )
	link_directories( AFTER ${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib )
endif()

add_definitions(-DSPIRV_DIR="${SHADERS_OUT_DIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)

add_executable(${TARGET_NAME} WIN32 ${CPP_FILES} ${HPP_FILES})

target_link_libraries( ${TARGET_NAME} ${VULKAN_LIB_LIST} ${GLFW3_LIB_LIST} )

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

#add_custom_target( build_shaders DEPENDS ${SHADER_OUT_NAMES} )
#add_dependencies( ${TARGET_NAME} ${SHADER_OUT_NAMES} )
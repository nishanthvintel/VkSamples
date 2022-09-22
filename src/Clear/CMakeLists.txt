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

set( GLFW3_PATH ${VULKAN_PATH}/../glfw )
if( EXISTS "${GLFW3_PATH}" )
	message( STATUS "Successfully located GLFW3: ${GLFW3_PATH}" )
else()
	message( "ERROR; Unable to locate GLFW3" )
	return()
endif()

set( VS_GENERATOR ${CMAKE_GENERATOR} )
STRING( REGEX REPLACE " " ";" VS_GENERATOR ${VS_GENERATOR} )
list( LENGTH VS_GENERATOR len )
math( EXPR LIST_LAST "${len} - 1" )
list( GET VS_GENERATOR ${LIST_LAST} VS_VER )
set( GLFW3_LIB_PATH ${GLFW3_PATH}/lib-vc${VS_VER} )

project( ${TARGET_NAME} )

add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

set( VULKAN_LIB_LIST "vulkan-1" )
set( GLFW3_LIB_LIST "glfw3" )

if( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	include_directories( AFTER ${VULKAN_PATH}/Include;${GLFW3_PATH}/include )
	link_directories( AFTER ${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib;${GLFW3_LIB_PATH} )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)

add_executable(${TARGET_NAME} ${CPP_FILES} ${HPP_FILES})

target_link_libraries( ${TARGET_NAME} ${VULKAN_LIB_LIST} ${GLFW3_LIB_LIST} )

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
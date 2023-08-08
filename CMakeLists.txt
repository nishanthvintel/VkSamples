cmake_minimum_required( VERSION 3.20 )

option( VULKAN_BUILD_SAMPLES "Build Vulkan Sample applications" ON )

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

project( VkSamples LANGUAGES CXX )

if( MSVC )
	add_compile_options( /MP )
endif()

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

if ( VULKAN_BUILD_SAMPLES )
	add_subdirectory( src/Clear )
	add_subdirectory( src/Triangle )

	set_target_properties( Clear PROPERTIES FOLDER Samples )
	set_target_properties( Triangle PROPERTIES FOLDER Samples )

	set_directory_properties( PROPERTIES VS_STARTUP_PROJECT Clear )
#	file( GLOB srcsubdirs RELATIVE src src/* )
#	foreach( srcsubdir ${srcsubdirs} )
#		message( ${srcsubdir})
#		if(IS_DIRECTORY src/${srcsubdir})
#			add_subdirectory( src/${srcsubdir} )
#			set_target_properties( ${srcsubdir} PROPERTIES FOLDER Samples )
#			if( ${srcsubdir} MATCHES "Clear" )
#				set_directory_properties( PROPERTIES VS_STARTUP_PROJECT ${srcsubdir} )
#			endif()
#		endif()
#	endforeach()
endif( VULKAN_BUILD_SAMPLES )

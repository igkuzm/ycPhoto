cmake_minimum_required(VERSION 3.15)

set(TARGET ycPhoto)

project(
	${TARGET} VERSION 1.0 
	DESCRIPTION "Library to save photos in yclients"
	HOMEPAGE_URL ""
	LANGUAGES C 
)

add_library(${TARGET} STATIC ycPhoto.c)

if(${BUILD_TEST})
	add_executable(ycPhoto_test test.c)
	target_link_libraries(ycPhoto_test ${TARGET})
endif()

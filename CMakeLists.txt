cmake_minimum_required(VERSION 3.3)


set (UNICODE TRUE)

set(XenoLib_MASTER_DIR 3rd_party/xenolib)
include(3rd_party/xenolib/build.cmake)
include(casmExtract/build.cmake)
include(mdoTextureExtract/build.cmake)

if (WIN32 AND UNICODE)
	message(STATUS "Compiling with wchar_t")
	add_definitions(-D_UNICODE -DUNICODE)
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof -pthread")
endif()
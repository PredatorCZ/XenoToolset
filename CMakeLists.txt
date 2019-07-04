cmake_minimum_required(VERSION 3.3)

if (NOT DEFINED UNICODE)
	set (UNICODE TRUE)
endif()

include(ExternalProject)
ExternalProject_Add(xenolib
DOWNLOAD_COMMAND ""
INSTALL_COMMAND ""
SOURCE_DIR "../3rd_party/xenolib"
BINARY_DIR "xenolib"
TMP_DIR "xenolib/tmp"
STAMP_DIR "xenolib/stamp"
)

include("casmExtract/build.cmake")
include("mdoTextureExtract/build.cmake")

if (WIN32 AND UNICODE)
	message(STATUS "Compiling with wchar_t")
	add_definitions(-D_UNICODE -DUNICODE)
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof -pthread")
endif()
project(casmExtract
VERSION 1.0.0)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}/${PROJECT_NAME}.cpp)

find_library(fndXenoLib XenoLib PATHS "${CMAKE_BINARY_DIR}/xenolib")

target_link_libraries(${PROJECT_NAME} ${fndXenoLib})

include_directories("./3rd_party/xenolib/include/")
include_directories("./3rd_party/xenolib/3rd_party/precore/")
include_directories("./3rd_party/pugixml/src/")

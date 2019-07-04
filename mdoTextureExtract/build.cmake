project(mdoTextureExtract
VERSION 1.0.0)

add_executable(${PROJECT_NAME}
${PROJECT_NAME}/${PROJECT_NAME}.cpp
3rd_party/pugixml/src/pugixml.cpp
3rd_party/xenolib/3rd_party/precore/datas/reflector.cpp
3rd_party/xenolib/3rd_party/precore/datas/reflectorXML.cpp
)

target_link_libraries(${PROJECT_NAME} XenoLib)

include_directories("./3rd_party/xenolib/include/")
include_directories("./3rd_party/xenolib/3rd_party/precore/")

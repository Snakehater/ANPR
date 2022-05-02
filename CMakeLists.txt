cmake_minimum_required(VERSION 2.8)
project( ANPR )
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
add_executable( main src/main.cpp )
target_link_libraries( main ${OpenCV_LIBS} )
configure_file(   
    ${CMAKE_CURRENT_SOURCE_DIR}/samples/001.jpg
    ${CMAKE_CURRENT_BINARY_DIR}/samples/001.jpg
    COPYONLY
)

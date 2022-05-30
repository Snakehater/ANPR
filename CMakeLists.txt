cmake_minimum_required( VERSION 3.2.2 )
project( ANPR )

find_package( OpenCV REQUIRED )
find_package( PkgConfig REQUIRED )
pkg_search_module( TESSERACT REQUIRED tesseract )
pkg_search_module( LEPTONICA REQUIRED lept )

include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${TESSERACT_INCLUDE_DIRS} )
include_directories( ${LEPTONICA_INCLUDE_DIRS} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )

add_executable( main src/main.cpp )

target_link_libraries( main ${OpenCV_LIBS} )
target_link_libraries( main ${TESSERACT_LIBRARIES} )
target_link_libraries( main ${LEPTONICA_LIBRARIES} )

configure_file(   
    ${CMAKE_CURRENT_SOURCE_DIR}/samples/001.jpg
    ${CMAKE_CURRENT_BINARY_DIR}/samples/001.jpg
    COPYONLY
)

# This will create a directory for debug material at runtime, ie when running make
add_custom_target(build-time-make-directory ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory debug)

add_custom_target(run
    COMMAND rm -rf debug/* && ./main ../samples/002.mp4
    DEPENDS main
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

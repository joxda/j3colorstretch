project(j3colorstretch)

cmake_minimum_required(VERSION 2.8.12)

set (OpenCV_DIR "/usr/local/Cellar/opencv/4.3.0/lib/cmake/opencv4/")

FIND_PACKAGE( OpenCV REQUIRED )

add_compile_options(-std=c++11 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic)

add_executable( j3colorstretch j3colorstretch.cpp  )
set_property(TARGET j3colorstretch PROPERTY CXX_STANDARD 11)


TARGET_INCLUDE_DIRECTORIES( j3colorstretch PRIVATE ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )

set(MY_OpenCV_LIBS "opencv_core;opencv_highgui;")

target_link_libraries( j3colorstretch  ${MY_OpenCV_LIBS})

install(TARGETS j3colorstretch DESTINATION bin)

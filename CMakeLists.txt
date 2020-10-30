project(j3colorstretch)

cmake_minimum_required(VERSION 2.8.12)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set (OpenCV_DIR "/usr/local/Cellar/opencv/*/lib/cmake/opencv4/")

FIND_PACKAGE( OpenCV REQUIRED )

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-unused")

add_compile_options(-std=c++11)

add_executable( j3colorstretch j3colorstretch.cpp  )
set_property(TARGET j3colorstretch PROPERTY CXX_STANDARD 11)

add_library( j3clrstrtch j3clrstrtch.cpp  )
set_property(TARGET j3clrstrtch PROPERTY CXX_STANDARD 11)

TARGET_INCLUDE_DIRECTORIES( j3colorstretch PRIVATE ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )

#set(MY_OpenCV_LIBS "opencv_core;opencv_highgui;")

target_link_libraries( j3clrstrtch  ${OpenCV_LIBS} )
target_link_libraries( j3colorstretch  ${OpenCV_LIBS} j3clrstrtch)

install(TARGETS j3colorstretch DESTINATION bin)
install(TARGETS j3clrstrtch DESTINATION lib)
install(PROGRAMS batch-stretch DESTINATION bin)

project(j3colorstretch)

cmake_minimum_required(VERSION 2.8.12)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set (OpenCV_DIR "/usr/local/Cellar/opencv/*/lib/cmake/opencv4/")

FIND_PACKAGE( OpenCV REQUIRED core improc highgui )

FIND_PACKAGE( Eigen3 3.3 NO_MODULE )

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-unused")

add_compile_options(-std=c++11)

if(DEFINED $ENV{CI})
  message("THIS IS A CI RUN")
  set(BUILD_SHARED_LIBS=OFF)
endif()

add_executable( j3colorstretch j3colorstretch.cpp j3clrstrtch.cpp )
set_property(TARGET j3colorstretch PROPERTY CXX_STANDARD 11)

#add_library( j3clrstrtch EXCLUDE_FROM_ALL j3clrstrtch.cpp )
#set_property(TARGET j3clrstrtch PROPERTY CXX_STANDARD 11)

TARGET_INCLUDE_DIRECTORIES( j3colorstretch PRIVATE ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )

#set(MY_OpenCV_LIBS "opencv_core;opencv_highgui;")

#target_link_libraries( j3clrstrtch  ${OpenCV_LIBS} )
if (TARGET Eigen3::Eigen AND NOT (DEFINED $ENV{CI}) )
  target_link_libraries( j3colorstretch  ${OpenCV_LIBS} Eigen3::Eigen)
else()
  target_link_libraries( j3colorstretch  ${OpenCV_LIBS})
endif()

install(TARGETS j3colorstretch DESTINATION bin PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )
#install(TARGETS j3clrstrtch DESTINATION lib)
install(PROGRAMS batch-stretch DESTINATION bin)

set(CPACK_GENERATOR "TGZ")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Joachim Janz") # required
set(CPACK_PACKAGE_VENDOR "joxda")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 3)
set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

include(CPack)

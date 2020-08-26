FIND_PACKAGE( OpenCV REQUIRED )

add_executable( j3colorstretch main.cpp  )
TARGET_INCLUDE_DIRECTORIES(j3colorstretch ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )
set(MY_OpenCV_LIBS "opencv_core;opencv_highgui;opencv_imgcodecs;opencv_calib3d;opencv_imgproc;opencv_features2d")
target_link_libraries( j3colorstretch  ${MY_OpenCV_LIBS} )

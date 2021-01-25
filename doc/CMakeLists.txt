project(j3colorstretch-doc)

cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../cmake" ${CMAKE_MODULE_PATH})

find_package(Doxygen REQUIRED)

set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_SOURCE_DIR}/html/index.html)

file(GLOB_RECURSE TO_DOCS ../j3*.?pp)

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${TO_DOCS}
                   COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   MAIN_DEPENDENCY Doxyfile
                   COMMENT "Generating docs")

add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})


find_package(Sphinx REQUIRED)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/docs/sphinx)

add_custom_target(Sphinx ALL
                  DEPENDS Doxygen
                  COMMAND
                  ${SPHINX_EXECUTABLE} -b html
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating documentation with Sphinx")

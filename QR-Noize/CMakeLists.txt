cmake_minimum_required(VERSION 2.8)

project( imgen )

add_subdirectory( Distortions )

add_executable( imgen ImageGen.cpp )

target_include_directories(imgen PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/Distortions"
                          "${PROJECT_SOURCE_DIR}"
                          )

target_link_libraries( imgen PUBLIC Distortions)


cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(Tutorial_Step6)

find_package(VTK COMPONENTS
        CommonColor
        CommonCore
        CommonTransforms
        FiltersSources
        FiltersTexture
        InteractionStyle
        InteractionWidgets
        RenderingContextOpenGL2
        RenderingCore
        RenderingFreeType
        RenderingGL2PSOpenGL2
        RenderingOpenGL2
        IOGeometry
        IOXML
        )

if (NOT VTK_FOUND)
    message(FATAL_ERROR "Tutorial_Step6: Unable to find the VTK build folder.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Prevent a "command line is too long" failure in Windows.
set(CMAKE_NINJA_FORCE_RESPONSE_FILE "ON" CACHE BOOL "Force Ninja to use response files.")
add_executable(Tutorial_Step6 MACOSX_BUNDLE main.cpp)
target_link_libraries(Tutorial_Step6 PRIVATE ${VTK_LIBRARIES})

# vtk_module_autoinit is needed
vtk_module_autoinit(
        TARGETS Tutorial_Step6
        MODULES ${VTK_LIBRARIES}
)

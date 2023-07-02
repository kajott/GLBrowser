cmake_minimum_required (VERSION 3.9)

project (GLMenu)

if (WIN32)
    link_directories (SDL2_win32/lib/x64)
    set (SDL2_DIR SDL2_win32/cmake)
endif ()

add_executable (glmenu
    src/main.cpp
    src/app.cpp
    src/renderer.cpp
    src/glad.c
    data/font_tex.c
)

if (WIN32)
    target_sources (glmenu PRIVATE
        src/utf8.manifest
    )
    target_link_libraries (glmenu PRIVATE SDL2main)
    set_target_properties (glmenu PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
else ()
    find_package (Threads REQUIRED)
    target_link_libraries (glmenu PUBLIC Threads::Threads m)
endif ()

find_package (SDL2 REQUIRED)
target_link_libraries (glmenu PUBLIC SDL2)

# no idea why this is needed; isn't CMake supposed to do that automatically
# in target_link_libraries()?!
get_target_property (SDL2INC SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories (glmenu PRIVATE ${SDL2INC})

# make the binary appear in the project's root directory
set_target_properties (glmenu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_NAME_DEBUG                       "glmenu_debug"
)

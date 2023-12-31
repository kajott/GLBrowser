cmake_minimum_required (VERSION 3.9)

project (GLBrowser)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

if (WIN32)
    link_directories (SDL2_win32/lib/x64)
    set (SDL2_DIR SDL2_win32/cmake)
endif ()

add_executable (glbrowser
    src/main.cpp
    src/app.cpp
    src/geometry.cpp
    src/renderer.cpp
    src/dirview.cpp
    src/menu.cpp
    src/file_assoc.cpp
    src/sysutil.cpp
    src/glad.c
    data/font_data.cpp
)

target_include_directories (glbrowser PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

if (WIN32)
    target_sources (glbrowser PRIVATE
        src/utf8.manifest
    )
    target_link_libraries (glbrowser PRIVATE SDL2main)
    set_target_properties (glbrowser PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
else ()
    find_package (Threads REQUIRED)
    target_link_libraries (glbrowser PUBLIC Threads::Threads m)
endif ()

find_package (SDL2 REQUIRED)
target_link_libraries (glbrowser PUBLIC SDL2)

# strict compiler options
if (MSVC)
    target_compile_options (glbrowser PRIVATE /W4 /WX)
else ()
    target_compile_options (glbrowser PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
    # GCC -pedantic prevents compiling GLAD
    set_property (SOURCE src/glad.c PROPERTY COMPILE_FLAGS -Wno-pedantic)
endif ()

# no idea why this is needed; isn't CMake supposed to do that automatically
# in target_link_libraries()?!
get_target_property (SDL2INC SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories (glbrowser PRIVATE ${SDL2INC})

# make the binary appear in the project's root directory
set_target_properties (glbrowser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_NAME_DEBUG                       "glbrowser_debug"
)

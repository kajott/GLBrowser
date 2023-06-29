# OpenGL Directory List and Launcher

(blah blah)

## Building (Linux)

- install SDL2 development packages
- build with CMake

## Building (Win32 + MSVC)

64-bit only!

- download `SDL2-devel-2.xx.y-VC.zip` from https://github.com/libsdl-org/SDL/releases/latest/
- unpack it into the project's root directory
- rename the `SDL2-2.xx.y` directory to `SDL2_win32`
- copy `SDL2_win32/lib/x64/SDL2.dll` into the root directory
- build with CMake (easiest way: use VS Code)

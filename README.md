# OpenGL Directory Browser

GLBrowser is a fullscreen directory browser that can be used to open files
with various applications. It runs on Windows and Linux and is a single-file
application on the latter. The UI is rendered with OpenGL 3.3. The font is
baked right into the executable file.

At this point, the list of viable applications is rather short;
in fact, this whole application is meant to be a frontend for my image viewer
[GLISS](https://svn.emphy.de/scripts/trunk/gliss.cpp) with little use outside
of that.


## Building (Linux)

- install SDL2 development packages
- build with CMake

Font data rebuilding is not directly supported on Linux at this moment,
unless you compile [the MSDF atlas generator](https://github.com/Chlumsky/msdf-atlas-gen)
yourself and run the commands listed in `data/build.cmd` manually.

## Building (Win32 + MSVC)

64-bit only!

- download `SDL2-devel-2.xx.y-VC.zip` from https://github.com/libsdl-org/SDL/releases/latest/
- unpack it into the project's root directory
- rename the `SDL2-2.xx.y` directory to `SDL2_win32`
- copy `SDL2_win32/lib/x64/SDL2.dll` into the root directory
- build with CMake (easiest way: use VS Code)

To rebuild the font data, download and unpack
[msdf-atlas-gen.exe](https://github.com/Chlumsky/msdf-atlas-gen/releases/latest)
into the `data` directory and run `build.cmd`.
A Python 3 installation is required for this to work.

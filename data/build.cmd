@cd /d "%~dp0"
@echo.

@set fontfile=bahnschrift.ttf

if not exist %fontfile% ( copy /b %SystemRoot%\Fonts\%fontfile% . )
@if errorlevel 1 goto end

@echo.

msdf-atlas-gen.exe ^
    -font %fontfile% ^
    -charset latin1_cs.txt ^
    -type msdf ^
    -format png ^
    -imageout font.png ^
    -json font.json ^
    -size 40 -pxrange 3
@if errorlevel 1 goto end

@echo.

convert_font.py
@if errorlevel 1 goto end

:end
@echo.
@pause

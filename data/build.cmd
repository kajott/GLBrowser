@cd /d "%~dp0"
@echo.

msdf-atlas-gen.exe ^
         -font %SystemRoot%\Fonts\bahnschrift.ttf -charset latin1_cs.txt ^
    -and -font %SystemRoot%\Fonts\arial.ttf       -charset graphics_cs.txt ^
    -type msdf ^
    -format png -imageout font.png ^
    -json font.json ^
    -size 40 -pxrange 3
@if errorlevel 1 goto end

@echo.

convert_font.py
@if errorlevel 1 goto end

:end
@echo.
@pause

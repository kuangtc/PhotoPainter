rem 1. path
set PICO_SDK=C:\Users\kuang\.pico-sdk\sdk\2.2.0
set CMAKE=C:\Users\kuang\.pico-sdk\cmake\v3.31.5\bin\cmake.exe
set NINJA=C:\Users\kuang\.pico-sdk\ninja\v1.12.1\ninja.exe

rem 2. clear
rmdir /s /q build

rem 3. CMake SetUp
"%CMAKE%" -S . -B build -DPICO_SDK_PATH="%PICO_SDK%" -DCMAKE_MAKE_PROGRAM="%NINJA%" -G Ninja

rem 4. Build
"%NINJA%" -C build

rem 5. copy
copy build\epd.uf2 PhotoPainter_v20260514.uf2

cmd
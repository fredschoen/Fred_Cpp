SET PATH=%PATH%;C:\Program Files\mingw64\bin
g++ -static-libgcc -static-libstdc++ LiensHTML.cpp -o LiensHTML.exe
del /Q old_LiensHTML.exe 2>nul
rename a.exe LiensHTML.exe
pause

rem : https://winlibs.com/
rem : -> https://github.com/brechtsanders/winlibs_mingw/releases/download/15.2.0posix-13.0.0-msvcrt-r1/winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64msvcrt-13.0.0-r1.zip
rem   -> à copier dans C:\Program Files\mingw64\bin

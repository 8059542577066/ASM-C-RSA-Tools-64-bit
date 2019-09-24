@echo off
rem gcc 5.1.0 (tdm64)
windres res.rc -o res.o
pause
gcc main.c res.o -O3 -ansi -o Enigma++.exe -l comdlg32 -D_FILE_OFFSET_BITS=64
pause
@echo off
rem gcc 5.1.0 (tdm64)
windres res.rc -o res.o
pause
gcc asm.s rsa.c -O3 -ansi -c
pause
gcc encoding.c -O3 -ansi -c
pause
gcc res.o asm.o rsa.o encoding.o main.c -O3 -ansi -o genrsa.exe
pause
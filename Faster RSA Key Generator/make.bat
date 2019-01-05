@echo off
rem gcc 5.1.0 (tdm64)
gcc asm.s rsa.c -O3 -ansi -o rsa.exe
pause
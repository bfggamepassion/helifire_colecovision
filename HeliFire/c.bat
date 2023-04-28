path c:\sdcc\bin
@echo off
sdcc -mz80 -c --std-c99 --opt-code-speed charset.c
del charset.lst
del charset.sym
del charset.asm
sdcc -mz80 -c --std-c99 --opt-code-speed main.c
del main.lst
del main.sym
del main.asm
sdasz80 -o music.rel music.s
sdcc -mz80 -c --std-c99 --opt-code-speed splash.c
del splash.lst
del splash.sym
del splash.asm
sdcc -mz80 -c --std-c99 --opt-code-speed title.c
del title.lst
del title.sym
del title.asm
pause

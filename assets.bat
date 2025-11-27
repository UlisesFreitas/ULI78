@echo off

echo Processing .....
uli78_pro\bin\prj2cart demos\luademo.lua build\luademo.uli
uli78_pro\bin\bin2txt build\luademo.uli build\assets\luademo.uli.dat -z

uli78_pro\bin\prj2cart demos\font.lua build\font.uli
uli78_pro\bin\prj2cart demos\jsdemo.js build\jsdemo.uli
uli78_pro\bin\prj2cart demos\music.lua build\music.uli
uli78_pro\bin\prj2cart demos\palette.lua build\palette.uli
uli78_pro\bin\prj2cart demos\sfx.lua build\sfx.uli


uli78_pro\bin\prj2cart demos\bunny\jsmark.js build\jsmark.uli
uli78_pro\bin\bin2txt build\font.uli build\assets\font.uli.dat -z
uli78_pro\bin\bin2txt build\jsdemo.uli build\assets\jsdemo.uli.dat -z
uli78_pro\bin\bin2txt build\music.uli build\assets\music.uli.dat -z
uli78_pro\bin\bin2txt build\palette.uli build\assets\palette.uli.dat -z
uli78_pro\bin\bin2txt build\sfx.uli build\assets\sfx.uli.dat -z
uli78_pro\bin\bin2txt build\jsmark.uli build\assets\jsmark.uli.dat -z


uli78_pro\bin\bin2txt build\cart.png build\assets\cart.png.dat

copy uli78_pro\bin\localplayer-sdl.exe uli78_pro\bin\localplayer.exe.dat > nul
copy uli78_pro\bin\SDL2.dll uli78_pro\bin\localplayer.exe.dll > nul

echo Done.

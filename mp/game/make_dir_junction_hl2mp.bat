@echo off

@REM replace this with your Source SDK Base 2013 Multiplayer/Half-life 2 Deathmatch directory
set "HL2MPDir=C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2 Deathmatch"

if not exist "%HL2MPDir%\hl2mp" (
	echo Unable to find the directory "%HL2MPDir%"
	echo If you've not done so already, please set the environment variable HL2MPDir to your SDK Base 2013 Multiplayer/Half-life 2 Deathmatch install directory.
	pause
	exit
)

mklink /J "%cd%\hl2mp" "%HL2MPDir%\hl2mp"
echo.

pause
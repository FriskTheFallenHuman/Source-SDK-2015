@echo off

@REM replace this with your Source SDK Base 2013 Singleplayer/Half-life 2 directory
set "HL2Dir=C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Singleplayer"

if not exist "%HL2Dir%\episodic" (
	echo Unable to find the directory "%HL2Dir%"
	echo If you've not done so already, please set the environment variable HL2Dir to your SDK Base 2013 Singleplayer/Half-life 2 install directory.
	pause
	exit
)

mklink /J "%cd%\episodic" "%HL2Dir%\episodic"
echo.

mklink /J "%cd%\ep2" "%HL2Dir%\ep2"
echo.

cls
call make_dir_junction_hl2mp.bat
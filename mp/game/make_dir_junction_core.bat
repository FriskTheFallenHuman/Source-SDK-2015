@echo off

@REM replace this with your Team Fortress 2 directory
set "TF2Dir=C:\Program Files (x86)\Steam\steamapps\common\Team Fortress 2"

if not exist "%TF2Dir%\hl2" (
	echo Unable to find the directory "%TF2Dir%"
	echo If you've not done so already, please set the environment variable TF2Dir to your Team Fortress 2 install directory.
	pause
	exit
)

mklink /J "%cd%\hl2" "%TF2Dir%\hl2"
echo.

cls
call make_dir_junction_hl2.bat
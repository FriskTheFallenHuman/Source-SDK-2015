@echo off
set params=

echo Games
echo --------
echo.

set /p HL2=Compile Half-Lif 2? (y): 
If not "%HL2%"=="n" (
    set params=%params% /hl2
)

set /p Episodic=Compile Half-Lif 2 Episodes? (y): 
If not "%Episodic%"=="n" (
    set params=%params% /episodic
)

set /p HL2DM=Compile Half-Lif 2 Deathmatch? (y): 
If not "%HL2DM%"=="n" (
    set params=%params% /hl2mp
)

cls
	devtools\bin\vpc.exe +game %params% /mksln games.sln /2013
pause
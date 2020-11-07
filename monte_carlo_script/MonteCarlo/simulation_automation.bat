@ECHO OFF
ECHO Running simulation automation script!


FOR /F "tokens=* delims=" %%x in (RunList.txt) DO mcxyz.exe %%x

PAUSE


set QTROOT=C:\Qt\4.8.4
set SPEC=win32-msvc2010

cd ..\src

for /D %%G in ("*") do (
cd %%G
%QTROOT%\bin\qmake -spec %SPEC% -tp vc %%G.pro
cd ..
)

%QTROOT%\bin\qmake -spec %SPEC% -tp vc metasystem.pro
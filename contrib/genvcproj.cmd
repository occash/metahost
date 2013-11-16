set QTROOT=C:\third-party\qt-4.8.5
set SPEC=win32-msvc2012

cd ..\src\metalib
%QTROOT%\bin\qmake -spec %SPEC% -tp vc metalib.pro

cd ..\..\samples
for /D %%G in ("*") do (
cd %%G
%QTROOT%\bin\qmake -spec %SPEC% -tp vc %%G.pro
cd ..
)
%QTROOT%\bin\qmake -spec %SPEC% -tp vc samples.pro
set QTROOT=C:\Qt\4.8.4
set SPEC=win32-msvc2010

for /D /r %%G in ("*") do (
cd %%G
%QTROOT%\bin\qmame -spec %SPEC% -tp vc %%G.pro
cd ..
)

%QTROOT%\bin\qmame -spec %SPEC% -tp vc metasystem.pro
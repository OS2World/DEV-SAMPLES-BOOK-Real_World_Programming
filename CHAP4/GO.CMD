@ECHO ������������������������������������������������Ŀ
@ECHO � Common Dialogs Application                     �
@ECHO � Chapter 4                                      �
@ECHO � Real World Programming for OS/2                �
@ECHO � Copyright (c) 1993 Blain, Delimon, and English �
@ECHO ��������������������������������������������������
@if "%COMPILER%"==""    goto nocompiler
@if %COMPILER%==CL386   set MAKECMD=nmake
@if %COMPILER%==cl386   set MAKECMD=nmake
@if %COMPILER%==BORLAND set MAKECMD=make
@if %COMPILER%==borland set MAKECMD=make
@if %COMPILER%==IBMC    set MAKECMD=nmake
@if %COMPILER%==ibmc    set MAKECMD=nmake
@if %COMPILER%==WATCOM  set MAKECMD=wmake
@if %COMPILER%==watcom  set MAKECMD=wmake
@if %COMPILER%==ZORTECH set MAKECMD=make
@if %COMPILER%==zortech set MAKECMD=make
%MAKECMD% -f ..\%COMPILER%.mak MAKEFILE=commdlg.dep
@goto done
:nocompiler
@ECHO 
@ECHO ��� COMPILER environment variable must be set to compiler name
@ECHO 
:done                                                               
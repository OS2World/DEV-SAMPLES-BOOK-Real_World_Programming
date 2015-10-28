#====================================================================
# WATCOM C/386
# Compiler Definition File
#
# Real World Programming for OS/2
# Copyright (c) 1993 Blain, Delimon, and English
#====================================================================

#=====================================================================
# Inference rules
#=====================================================================

.EXTENSIONS:
.EXTENSIONS:  .res .rc .lib .def .exe .dll .obj .c .h .asm

ABOUT = ..\common\about.obj

!include $(MAKEFILE)

.c.obj :
    @echo ¯¯¯
    @echo ®®® Compiling $*.c using WATCOM C/386 options ¯¯¯
    @echo ¯¯¯
    wcc386 -d1 -zq -W2 -D__WATCOM__ $(WCL386OPTS) $*.c

.asm.obj :
    @echo ¯¯¯
    @echo ®®® Assembling $*.asm using WATCOM C/386 options ¯¯¯
    @echo ¯¯¯
    masm -v -ML -Mx -d__MASM386__ -W2 $*.asm,$*.obj;

.rc.res :
    @echo ¯¯¯
    @echo ®®® Running resource compiler on $*.rc ¯¯¯
    @echo ¯¯¯
    rc -r $*.rc

.def.lib :
    @echo ¯¯¯
    @echo ®®® Creating import library from $*.def ¯¯¯
    @echo ¯¯¯
    wlib /n $*.lib +$*.dll

.obj.exe :
    @echo ¯¯¯
    @echo ®®® Building $*.exe using WATCOM C/386 options ¯¯¯
    @echo ¯¯¯
    @%create lnk.lnk
    @for %i in ($objs) do @%append lnk.lnk file %i
!ifdef LIBS
    %append lnk.lnk lib $LIBS
!endif
    wlink d a op symfile system os2v2_pm option map, stack=32k @lnk file $ABOUT
    rc $*.res $*.exe

.obj.dll :
    @echo ¯¯¯
    @echo ®®® Building $*.dll using WATCOM C/386 options ¯¯¯
    @echo ¯¯¯
    @%create lnk.lnk
    @for %i in ($objs) do @%append lnk.lnk file %i
!ifdef LIBS
    %append lnk.lnk lib $LIBS
!endif
!ifdef EXTRA
    %append lnk.lnk $EXTRA
!endif
    wlink system os2v2 dll initinstance option manyautodata @lnk op map
    rc $*.res $*.dll

#====================================================================
# CL386
# Compiler Definition File 
#
# Real World Programming for OS/2
# Copyright (c) 1993 Blain, Delimon, and English
#====================================================================

#=====================================================================
# Inference rules
#=====================================================================

CL386CCSWITCH = -G3sw
ABOUT         = ..\common\about.obj

.c.obj : 
    @echo ¯¯¯
    @echo ®®® Compiling $*.c using CL386 options ¯¯¯
    @echo ¯¯¯
    cl386 -c $(CL386CCSWITCH) -W2 -D__CL386__ $(CL386OPTS) $*.c > $*.err
    type $*.err

.asm.obj :
    @echo ¯¯¯
    @echo ®®® Assembling $*.asm using CL386 options ¯¯¯
    @echo ¯¯¯
    masm386 -v -ML -Mx -d__MASM386__ -W2 $*.asm,$*.obj;

.rc.res :
    @echo ¯¯¯
    @echo ®®® Running resource compiler on $*.rc ¯¯¯
    @echo ¯¯¯
    rc -r $*.rc

.def.lib :
    @echo ¯¯¯
    @echo ®®® Creating import library from $*.def ¯¯¯
    @echo ¯¯¯
    implib $*.lib $*.def

.obj.exe :                                          
    @echo ¯¯¯
    @echo ®®® Building $*.exe using CL386 options ¯¯¯
    @echo ¯¯¯
    link386 /nod /noe /map $(OBJS) $(ABOUT),$*.exe,$*.map,$(LIBS) os2386 libc,$*.def
    rc $*.res $*.exe

.obj.dll :
    @echo ¯¯¯
    @echo ®®® Building $*.dll using CL386 options ¯¯¯
    @echo ¯¯¯
    link386 /nod /noe /map $(OBJS) $(CL386INITOBJ),$*.dll,$*.map,$(LIBS) os2386 libcdll,$*.def
    rc $*.res $*.dll

!include $(MAKEFILE)

#====================================================================
# Zortech C++ for OS/2
# Compiler Definition File 
#
# Real World Programming for OS/2
# Copyright (c) 1993 Blain, Delimon, and English
#====================================================================

ZORTECHLINKER = blink
ABOUT         = ..\common\about.obj

include $(MAKEFILE)

#=====================================================================
# Inference rules
#=====================================================================

.c.obj : 
    @echo ¯¯¯
    @echo ®®® Compiling $*.c using Zortech options ¯¯¯
    @echo ¯¯¯
    ztc -c -mf -Jm -w2 -D__ZORTECH__ $(ZORTECHOPTS) $*.c 

.asm.obj :
    @echo ¯¯¯
    @echo ®®® Assembling $*.asm using MASM386 options ¯¯¯
    @echo ¯¯¯
    masm386 -v -ML -Mx -d__MASM386__ -W2$ $*.asm,$*.obj;

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
    @echo ®®® Building $*.exe using Zortech options ¯¯¯
    @echo ¯¯¯
    $(ZORTECHLINKER) /nod /map $(OBJS) $(ABOUT),$*.exe,$*.map, $(LIBS) os2386 zpf,$*.def
    rc $*.res $*.exe

.obj.dll :
    @echo ¯¯¯
    @echo ®®® Building $*.dll using Zortech options ¯¯¯
    @echo ¯¯¯
    $(ZORTECHLINKER) /nod /map /dosseg $(OBJS) $(ZORTECHINITOBJ),$*.dll,$*.map, $(LIBS) os2386 zpf,$*.def
    rc $*.res $*.dll


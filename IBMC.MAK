#====================================================================
# IBM C/Set 2 for OS/2
# Compiler Definition File 
#
# Real World Programming for OS/2
# Copyright (c) 1993 Blain, Delimon, and English
#====================================================================

#=====================================================================
# Inference rules
#=====================================================================

IBMCLINKOPTS = /noi
ABOUT        = ..\COMMON\ABOUT.OBJ

.c.obj : 
    @echo ¯¯¯
    @echo ®®® Compiling $*.c using IBM C/Set options ¯¯¯
    @echo ¯¯¯
    icc -C -Fm+ -Gm+ -D__IBM__ -SSM -Ti+ -W3  $(IBMCOPTS) $*.c > $*.err
    type $*.err

.asm.obj :
    @echo ¯¯¯
    @echo ®®® Assembling $*.asm using IBMC options ¯¯¯
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
    @echo ®®® Building $*.exe using IBMC options ¯¯¯
    @echo ¯¯¯
    link386 /noe /map $(IBMCLINKOPTS) $(OBJS) $(ABOUT),$*.exe, $*.map, $(LIBS) DDE4MBm,$*.def
    rc $*.res $*.exe

.obj.dll :
    @echo ¯¯¯
    @echo ®®® Building $*.dll using IBMC options ¯¯¯
    @echo ¯¯¯
    link386 /noi /noe /map $(OBJS),$*.dll, $*.map,DDE4MBm,$*.def
    rc $*.res $*.dll

!include $(MAKEFILE)



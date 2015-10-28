#====================================================================
# Borland C++ for OS/2
# Compiler Definition File 
#
# Real World Programming for OS/2
# Copyright (c) 1993 Blain, Delimon, and English
#====================================================================

#=====================================================================
# Inference rules
#=====================================================================

BORLANDEXEOPTS  = -m -aa -Toe -L$(LIB)
BORLANDDLLOPTS  = -m -Tod -L$(LIB)
ABOUT           = ..\common\about.obj

.c.obj : 
    @echo ¯¯¯
    @echo ®®® Compiling $*.c using Borland options ¯¯¯
    @echo ¯¯¯
    bcc -c -K -w -D__BORLAND__ $(BORLANDOPTS) $*.c > $*.err
    type $*.err

.asm.obj :
    @echo ¯¯¯
    @echo ®®® Assembling $*.asm using MASM386 options ¯¯¯
    @echo ¯¯¯
    tasm -ml -mx -d__TASM__ -oi -w2 $*.asm,$*.obj;

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
    @echo ®®® Building $*.exe using Borland options ¯¯¯
    @echo ¯¯¯
    tlink $(BORLANDEXEOPTS) c02 $(OBJS) $(ABOUT),$*.exe,$*.map,$(LIBS) c2 os2386,$*.def
    rc $*.res $*.exe 

.obj.dll :
    @echo ¯¯¯
    @echo ®®® Building $*.dll using Borland options ¯¯¯
    @echo ¯¯¯
    tlink $(BORLANDDLLOPTS) $(OBJS) $(BORLANDINITOBJ),$*.dll,$*.map,$(LIBS) os2386,$*.def
    rc $*.res $*.dll

!include $(MAKEFILE)

# MEMINST.OBJ must be built this way to force the _INSTANCEDATA segment
# to be placed in a separate segment from the _DATA segment
# The -oi option of tasm creates the correct kind of object record
# for the linker
meminst.obj : meminst.c
    bcc -c -K -w -S -D__BORLAND__ -zR_INSTANCEDATA -zTFAR_DATA $(BORLANDOPTS) $*.c > $*.err
    tasm -ml -mx -d__TASM__ -oi -w2 $*.asm,$*.obj;
    del $*.asm

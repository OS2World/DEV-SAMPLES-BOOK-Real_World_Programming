/* --------------------------------------------------------------------
                        Memory DLL Instance Data
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#include <os2.h>

#include "meminst.h"

#ifndef __ZORTECH__

/* IBMC compiler defines data segments with #pragma data_seg */
#ifdef __IBMC__
#pragma data_seg ( _INSTANCEDATA )
#endif

INSTANCEDATA_STRUCT InstanceData =
{
    NULL,             /* pMemPrivate           */
    NULL,             /* pMemSharedHead        */
    0L,               /* ulTotalAlloc          */
    0L,               /* ulTotalSubAlloc       */
    0L                /* ulTotalSharedSubAlloc */
};
#else
/* Zortech Compiler does not have a command line switch to rename the
   data segment.  Use the c_common data segment by not initializing
   InstanceData. */
INSTANCEDATA_STRUCT InstanceData;
#endif

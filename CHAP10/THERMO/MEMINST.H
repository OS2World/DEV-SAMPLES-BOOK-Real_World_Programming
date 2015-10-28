/* --------------------------------------------------------------------
                          MemInst Header File
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* MemDLL instance data structure */
typedef struct tagINSTANCE_STRUCT
{
    PVOID pMemPrivate;
    PVOID pMemSharedHead;
    ULONG ulTotalAlloc;
    ULONG ulTotalSubAlloc;
    ULONG ulTotalSharedSubAlloc;
} INSTANCEDATA_STRUCT;

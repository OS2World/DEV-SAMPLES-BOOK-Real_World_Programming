/* --------------------------------------------------------------------
                          MemDLL Header File
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

PVOID   EXPENTRY PrivateAllocate (ULONG);
VOID    EXPENTRY PrivateFree (PVOID);
VOID    EXPENTRY PrivateQuery (PULONG,PULONG);
PVOID   EXPENTRY SharedAllocate (ULONG);
VOID    EXPENTRY SharedFree (PVOID);
VOID    EXPENTRY SharedQuery (PULONG,PULONG);

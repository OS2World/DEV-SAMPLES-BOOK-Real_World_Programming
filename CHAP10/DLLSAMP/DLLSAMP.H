/* --------------------------------------------------------------------
                         DLL Sample Header File
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* Exported Functions */
VOID EXPENTRY CenterWindow (HWND);
VOID EXPENTRY FillWindow (HWND, HPS, ULONG);

/* Function typedefs for run-time dynamic linking */
#ifndef __IBMC__
typedef VOID (APIENTRY *PFNCENTERWINDOW) (HWND);
typedef VOID (APIENTRY *PFNFILLWINDOW) (HWND, HPS, ULONG);
#else
typedef VOID (APIENTRY _PFNCENTERWINDOW) (HWND);
typedef _PFNCENTERWINDOW *PFNCENTERWINDOW;
typedef VOID (APIENTRY _PFNFILLWINDOW) (HWND, HPS, ULONG);
typedef _PFNFILLWINDOW *PFNFILLWINDOW;
#endif


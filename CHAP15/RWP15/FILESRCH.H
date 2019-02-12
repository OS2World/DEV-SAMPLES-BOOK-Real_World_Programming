/* --------------------------------------------------------------------
                        File Search Header File
                              Chapter 15

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* Exported Function */
MRESULT EXPENTRY FileSearchDlgProc  (HWND,ULONG,MPARAM,MPARAM);

VOID DoFileSearch (HWND);

/* FileSearch Dialog Controls             */

#define IDC_DRIVELIST           300

#define IDC_FILENAME            400
#define IDC_DIRLIST             401
#define IDC_CURRENTDIR          402
#define IDC_SEARCHING           403
#define IDC_SEARCH              404

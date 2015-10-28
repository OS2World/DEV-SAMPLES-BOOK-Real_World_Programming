/* --------------------------------------------------------------------
                           Drives Header File
                              Chapter 15

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* Exported Function */
MRESULT EXPENTRY DrivesDlgProc  (HWND,ULONG,MPARAM,MPARAM);

VOID QueryDrives (HWND);

typedef struct
{
    CHAR  szDrive[2];
    CHAR  szLocation[7];
    CHAR  szRemovable[4];
    CHAR  szFileSystem[16];
    ULONG ulDescriptionIndex;
} DRIVEINFO;

/* Drives Dialog Controls             */

#define IDC_DRIVELIST                300
#define IDC_LOCATION                 301
#define IDC_REMOVABLE                302
#define IDC_FILESYSTEM               303
#define IDC_DESCRIPTION              304
#define IDC_VOLUMELABEL              305
#define IDC_TOTALSPACE               306
#define IDC_ALLOCATED                307
#define IDC_AVAILABLE                308

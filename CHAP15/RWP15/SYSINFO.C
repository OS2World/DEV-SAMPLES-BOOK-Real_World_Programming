/* --------------------------------------------------------------------
                          System Information
                              Chapter 15

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_DOS
#define INCL_WIN
#define INCL_ERRORS

#include <os2.h>
#include <stdio.h>
#include "sysinfo.h"

/* Local Functions    */
VOID QueryAllValues (HWND);

/* External variables */
extern HAB       hab;                    /* Handle to anchor block       */

CHAR szSysValues[QSV_MAX][18] =
    {
        "MAX_PATH_LENGTH  ",
        "MAX_TEXT_SESSIONS",
        "MAX_PM_SESSIONS  ",
        "MAX_VDM_SESSIONS ",
        "BOOT_DRIVE       ",
        "DYN_PRI_VARIATION",
        "MAX_WAIT         ",
        "MIN_SLICE        ",
        "MAX_SLICE        ",
        "PAGE_SIZE        ",
        "VERSION_MAJOR    ",
        "VERSION_MINOR    ",
        "VERSION_REVISION ",
        "MS_COUNT         ",
        "TIME_LOW         ",
        "TIME_HIGH        ",
        "TOTPHYSMEM       ",
        "TOTRESMEM        ",
        "TOTAVAILMEM      ",
        "MAXPRMEM         ",
        "MAXSHMEM         ",
        "TIMER_INTERVAL   ",
        "MAX_COMP_LENGTH  "
    };

/* ----------------------- Local Functions ----------------------- */

VOID QueryAllValues (HWND hWnd)
{ 
    ULONG ulValues[QSV_MAX];
    CHAR  szText[31];
    ULONG ulInx;

    DosQuerySysInfo (QSV_MAX_PATH_LENGTH, QSV_MAX, &ulValues, sizeof(ulValues));

    for (ulInx = 0; ulInx < QSV_MAX; ulInx++)
    {
        sprintf (szText, "%17s   0x%08lx", szSysValues[ulInx], ulValues[ulInx]);
        WinUpper (hab, 0, 0, szText);
        szText[21] = 'x';
        WinSendDlgItemMsg (hWnd, IDC_SYSINFOLIST, LM_INSERTITEM, (MPARAM)LIT_END,
            szText);
    }        
    return;
}

/* ----------------------  Dialog Function ----------------------- */

MRESULT EXPENTRY SysInfoDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    HWND    hWndListBox;
    SWP     Swp;

    switch (msg)
    {           
        case WM_INITDLG:
            QueryAllValues (hWnd);
            break;

        case WM_WINDOWPOSCHANGED:
            if (((PSWP)mp1)->fl & SWP_SHOW)
            {
                /* Center the listbox horizontally in the window */
                hWndListBox = WinWindowFromID (hWnd, IDC_SYSINFOLIST);
                WinQueryWindowPos (hWndListBox, &Swp);
                WinSetWindowPos (hWndListBox, 0L,
                    (((PSWP)mp1)->cx - Swp.cx) >> 1, Swp.y, 0L, 0L, SWP_MOVE);
            }
            bHandled = FALSE;
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefDlgProc (hWnd, msg, mp1, mp2);

    return (mReturn);
}


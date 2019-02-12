/* --------------------------------------------------------------------
                         Miscellaneous Program
                              Chapter 15

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include "misc.h"
#include "pibtib.h"
#include "drives.h"
#include "filesrch.h"
#include "sysinfo.h"
#include "..\common\about.h"

/* Window and Dialog Functions      */
MRESULT EXPENTRY ClientWndProc  (HWND,ULONG,MPARAM,MPARAM);

/* Global Variables                 */
HAB      hab;
HWND     hWndFrame, 
         hWndClient;
CHAR     szTitle[64];

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                            FCF_ICON     | FCF_MENU;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, ClientWndProc, 0, 0);
    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    /* Create the main application window */
    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, WS_VISIBLE,
        &flFrameFlags, szClientClass, szTitle, 0, 0, ID_APPNAME, &hWndClient);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}

/* ----------------------- Window Function ----------------------- */

MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;

    switch (msg)
    {
        case WM_ERASEBACKGROUND:
            mReturn = MRFROMLONG(1L);
            break;

		  case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDM_PIBTIB:
                    WinDlgBox (HWND_DESKTOP, hWnd, PIBTIBDlgProc,
                            0L, IDD_PIBTIB, NULL);
                    break;

                case IDM_DRIVES:
                    WinDlgBox (HWND_DESKTOP, hWnd, DrivesDlgProc,
                            0L, IDD_DRIVES, NULL);
                    break;

                case IDM_FILESEARCH:
                    DoFileSearch (hWnd);
                    break;

                case IDM_SYSINFO:
                    WinDlgBox (HWND_DESKTOP, hWnd, SysInfoDlgProc,
                            0L, IDD_SYSINFO, NULL);
                    break;

                case IDM_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;
            }
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}

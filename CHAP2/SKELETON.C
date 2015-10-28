/* --------------------------------------------------------------------
                           Skeleton Program                              
                              Chapter 2

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include "skeleton.h"
#include "..\common\about.h"

MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);

HAB   hab;
HWND  hWndFrame, 
      hWndClient;
CHAR  szTitle[64];

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                            FCF_ICON | FCF_MENU;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, (PFNWP)ClientWndProc, 0, 0);
    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, WS_VISIBLE,
        &flFrameFlags, szClientClass, szTitle, 0, 0, ID_APPNAME, &hWndClient);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}

MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    HPS     hps;
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;

    switch (msg)
    {
        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            WinEndPaint (hps);
            break;

        case WM_ERASEBACKGROUND:
            mReturn = MRFROMLONG(1L);
            break;

		  case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDM_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;
            }

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}


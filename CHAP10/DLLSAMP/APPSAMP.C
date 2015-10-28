/* --------------------------------------------------------------------
                           APPSAMP Program
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN

#include <os2.h>
#include "appsamp.h"
#include "dllsamp.h"
#include "..\..\common\about.h"

MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);

HAB   hab;
HWND  hWndFrame = 0,
      hWndClient;
CHAR  szTitle[64];
ULONG ulColors[3]    = { 0x00FF0000, 0x0000FF00, 0x000000FF };
ULONG ulCurrentColor = 0L;

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                            FCF_MENU     | FCF_ICON;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, ClientWndProc, 0, 0);
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
            /* Call DLL function to fill the window */
            FillWindow (hWnd, hps, ulColors[ulCurrentColor]);
            WinEndPaint (hps);
            break;

        case WM_ERASEBACKGROUND:
            mReturn = MRFROMLONG(1L);
            break;

        case WM_COMMAND:
            switch (LOUSHORT(mp1))
            {
                case IDM_RED:
                case IDM_GREEN:
                case IDM_BLUE:
                    ulCurrentColor = LOUSHORT(mp1) - IDM_RED;
                    hps = WinGetPS (hWnd);
                    /* Call DLL function to fill the window */
                    FillWindow (hWnd, hps, ulColors[ulCurrentColor]);
                    WinReleasePS (hps);
                    break;

                case IDM_CENTER:
                    /* Call DLL function to center the window */
                    CenterWindow (hWndFrame);
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


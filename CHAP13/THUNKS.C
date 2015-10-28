/* --------------------------------------------------------------------
                            Thunks Program
                              Chapter 13

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include "thunks.h"
#include "..\common\about.h"

/* Window and Dialog Functions      */
MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY BookInfoDlgProc (HWND,ULONG,MPARAM,MPARAM);

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
                case IDM_BOOKINFO:
                    WinDlgBox (HWND_DESKTOP, hWnd, BookInfoDlgProc,
                            0L, IDD_BOOKINFO, NULL);
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

MRESULT EXPENTRY BookInfoDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT         mReturn  = 0;
    BOOL            bHandled = TRUE;
    BOOKINFORMATION BookInfo;
    USHORT          usInx;
    PSZ             pszAuthor;

    switch (msg)
    {
        case WM_INITDLG:
            /* Call the thunk routine to call into the 16-bit function */
#if defined(__BORLAND__) || (__IBMC__)
            GetBookInformation (&BookInfo);
#else
            _THUNK_GetBookInformation (&BookInfo);
#endif

            WinSetDlgItemText (hWnd, IDC_TITLE, BookInfo.pszBookTitle);

            for (usInx = 0; usInx < BookInfo.usNumAuthors; usInx++)
            {
                /* Call the thunk routine to call into the 16-bit function */
#if defined(__BORLAND__) || (__IBMC__)
                pszAuthor = GetAuthorByIndex (usInx);
#else
                pszAuthor = _THUNK_GetAuthorByIndex (usInx);
#endif

                WinSendDlgItemMsg (hWnd, IDC_AUTHORLIST,
                    LM_INSERTITEM, (MPARAM)LIT_END, (MPARAM)pszAuthor);
            }
            break;   

        default:
           bHandled = FALSE;
           break;
    }

    if (!bHandled)
        mReturn = WinDefDlgProc (hWnd, msg, mp1, mp2);

    return (mReturn);
}

              

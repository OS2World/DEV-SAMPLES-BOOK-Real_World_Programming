/* --------------------------------------------------------------------
                          Thread Information
                              Chapter 15

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_DOS
#define INCL_WIN

#include <os2.h>
#include <string.h>
#include <stdio.h>
#include "pibtib.h"

/* External variables */
extern HAB      hab;                    /* Handle to anchor block       */

/* Local Functions    */
VOID ParseEnvironment (HWND,PCHAR,USHORT);
VOID SetNumberField   (HWND,ULONG,USHORT);

char szProcType[4][18] =
    {
        "Unknown",
        "Not Window Compat",
        "Window Compat",
        "Window API"
    };

/* ----------------------- Local Functions ----------------------- */

VOID ParseEnvironment (HWND hWnd, PCHAR pEnv, USHORT usCtlID)
{
    /* Parse the environment buffer.  Each line is terminated by a NULL
       byte and the last line in the buffer includes an additional NULL
       byte */
    while (*pEnv)
    {
        WinSendDlgItemMsg (hWnd, usCtlID, LM_INSERTITEM, (MPARAM)LIT_END, pEnv);
        pEnv += strlen(pEnv) + 1;
    }

    return;
}

VOID SetNumberField (HWND hWnd, ULONG ulNumber, USHORT usCtlID)
{
    CHAR    szNumber[9];

    /* Convert number to hexadecimal string */
    sprintf (szNumber, "%8lx", ulNumber);
    WinUpper (hab, 0, 0, szNumber);

    /* Set the text of the control */
    WinSetDlgItemText (hWnd, usCtlID, szNumber);

    return;
}

/* ----------------------  Dialog Function ----------------------- */

MRESULT EXPENTRY PIBTIBDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    PPIB    ppib;
    PTIB    ptib;

                                         
    switch (msg)
    {           
        case WM_INITDLG:
            /* Query the thread information */
            DosGetInfoBlocks (&ptib, &ppib);

            SetNumberField (hWnd, ppib->pib_ulpid,                IDC_PID);
            SetNumberField (hWnd, ppib->pib_ulppid,               IDC_PPID);
            SetNumberField (hWnd, ppib->pib_hmte,                 IDC_MODULEHANDLE);
            SetNumberField (hWnd, ppib->pib_flstatus,             IDC_STATUS);
            WinSetDlgItemText (hWnd, IDC_TYPE, szProcType[ppib->pib_ultype & 0x03]);
            SetNumberField (hWnd, (ptib->tib_ptib2)->tib2_ultid,  IDC_TID);
            SetNumberField (hWnd, (ptib->tib_ptib2)->tib2_ulpri,  IDC_PRIORITY);

            /* The OS/2 2.0 Toolkit defines this field as tib_arbpointer */
            /* The OS/2 2.1 Toolkit defines this field as tib_ordinal    */
            SetNumberField (hWnd, (ULONG)ptib->tib_ordinal,       IDC_SLOT);

            SetNumberField (hWnd, (ULONG)ptib->tib_pstack,        IDC_STACKBASE);
            SetNumberField (hWnd, (ULONG)ptib->tib_pstacklimit,   IDC_STACKEND);
            WinSendDlgItemMsg (hWnd, IDC_COMMANDLINE, EM_SETTEXTLIMIT, 
                MPFROMSHORT(255), 0L);
            WinSetDlgItemText (hWnd, IDC_COMMANDLINE, ppib->pib_pchcmd);
            ParseEnvironment (hWnd, ppib->pib_pchenv, IDC_ENVIRONMENT);
            break;

        case WM_SYSCOMMAND:
		      switch (SHORT1FROMMP(mp1))
            {
				    case SC_CLOSE:
                    WinDismissDlg (hWnd, FALSE);
                    bHandled = TRUE;
                    break;
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


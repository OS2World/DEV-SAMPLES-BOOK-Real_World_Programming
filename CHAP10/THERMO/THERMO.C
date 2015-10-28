/* --------------------------------------------------------------------
                            Thermo Program
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_DOS
#define INCL_WIN

#include <os2.h>
#include <stdio.h>
#include "thermo.h"
#include "memdll.h"
#include "controls.h"
#include "..\..\common\about.h"

/* Exported Functions */

MRESULT EXPENTRY MainDlgProc (HWND,ULONG,MPARAM,MPARAM);

/* Local Functions */

VOID  AddListItem (HWND,PVOID,ULONG);
VOID  FreeAllListItems (HWND,ULONG,BOOL);
VOID  FreeListItem (HWND,ULONG,BOOL);
ULONG ParseItem (PSZ);
VOID  SetNumberField (HWND,ULONG,USHORT);
VOID  UpdateNumbers (HWND);

/* Global Variables */

HAB      hab;                    /* Handle to anchor block       */
CHAR     szTitle[64];

/* Table of colors */
CHAR szColors[15][10] =
    { 
      "Black",
      "Blue",
      "Brown",
      "Cyan",
      "DarkBlue",
      "DarkCyan",
      "DarkGray",
      "DarkGreen",
      "DarkPink",
      "DarkRed",
      "Green",
      "Pink",
      "Red",
      "Yellow",
      "White"
    };

/* Array of pointers to color strings (for spinner control) */
PVOID pszColors[15] = 
    {
        &szColors[0],
        &szColors[1],
        &szColors[2],
        &szColors[3],
        &szColors[4],
        &szColors[5],
        &szColors[6],
        &szColors[7],
        &szColors[8],
        &szColors[9],
        &szColors[10],
        &szColors[11],
        &szColors[12],
        &szColors[13],
        &szColors[14]
    };

/* Table of color indexes matching the szColor entries */
SHORT sColorMap[15] =
    {
      CLR_BLACK,
      CLR_BLUE,
      CLR_BROWN,
      CLR_CYAN,
      CLR_DARKBLUE,
      CLR_DARKCYAN,
      CLR_DARKGRAY,
      CLR_DARKGREEN,
      CLR_DARKPINK,
      CLR_DARKRED,
      CLR_GREEN,
      CLR_PINK,
      CLR_RED,
      CLR_YELLOW,
      CLR_WHITE
    };

/* Undocument menu message */
#define MM_QUERYITEMBYPOS      0x01f3
#define MAKE_16BIT_POINTER(p)  ((PVOID)MAKEULONG(LOUSHORT(p),(HIUSHORT(p) << 3) | 7))

int main()
{
    HAB     hab;
    HMQ     hmq;

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    WinDlgBox (HWND_DESKTOP, HWND_DESKTOP, MainDlgProc, 0,
        ID_APPNAME, NULL);

    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}

VOID AddAboutToSystemMenu(HWND hWndFrame)
{
    MENUITEM mi;
    HWND     hWndSysSubMenu;

    WinSendMsg (WinWindowFromID (hWndFrame, FID_SYSMENU),
         MM_QUERYITEMBYPOS, 0L, MAKE_16BIT_POINTER(&mi));
    hWndSysSubMenu = mi.hwndSubMenu;
    mi.iPosition   = MIT_END;
    mi.afStyle     = MIS_SEPARATOR;
    mi.afAttribute = 0;
    mi.id          = -1;
    mi.hwndSubMenu = 0;
    mi.hItem       = 0;
    WinSendMsg (hWndSysSubMenu, MM_INSERTITEM, MPFROMP (&mi), NULL);
    mi.afStyle     = MIS_TEXT;
    mi.id          = IDM_ABOUT;
    WinSendMsg (hWndSysSubMenu, MM_INSERTITEM, MPFROMP (&mi), "About...");
    return;
}

VOID AddListItem (HWND hWnd, PVOID pMem, ULONG ulCtlID)
{
    CHAR  szNumber[9];
    SHORT sIndex;

    /* Convert address to hexadecimal string */
    sprintf (szNumber, "%08lx", (ULONG)pMem);
    WinUpper (hab, 0, 0, szNumber);

    /* Insert formatted memory address into listbox */
    sIndex = (SHORT)WinSendDlgItemMsg (hWnd, ulCtlID, LM_INSERTITEM, 
                        (MPARAM)LIT_END, szNumber);

    /* Select the item just inserted */
    WinSendDlgItemMsg (hWnd, ulCtlID, LM_SELECTITEM, (MPARAM)sIndex, (MPARAM)TRUE);

    return;
}

VOID FreeAllListItems (HWND hWnd, ULONG ulCtlID, BOOL bPrivate)
{
    CHAR  szNumber[9];
    SHORT sIndex;

    /* Query number of items in the listbox */
    sIndex = (SHORT)WinSendDlgItemMsg (hWnd, ulCtlID, LM_QUERYITEMCOUNT, 0L, 0L);

    while (sIndex--)
    {
        /* Query the text of the last item in the listbox */
        WinSendDlgItemMsg (hWnd, ulCtlID,
            LM_QUERYITEMTEXT, MPFROM2SHORT(sIndex,9), szNumber);

        /* Convert text to address and free memory */
        if (bPrivate)
            PrivateFree ((PVOID)ParseItem (szNumber));
        else
            SharedFree ((PVOID)ParseItem (szNumber));
    }

    /* Delete all items in the listbox */
    WinSendDlgItemMsg (hWnd, ulCtlID, LM_DELETEALL, 0L, 0L);

    return;
}

VOID FreeListItem (HWND hWnd, ULONG ulCtlID, BOOL bPrivate)
{
    CHAR  szNumber[9];
    SHORT sIndex;

    /* Query the selected item in the listbox */
    sIndex = (SHORT)WinSendDlgItemMsg (hWnd, ulCtlID, LM_QUERYSELECTION, 
                 (MPARAM)LIT_FIRST, 0L);

    if (sIndex != LIT_NONE)
    {
        /* Query the text of the selected item in the listbox */
        WinSendDlgItemMsg (hWnd, ulCtlID,
            LM_QUERYITEMTEXT, MPFROM2SHORT(sIndex,9), szNumber);

        /* Convert text to address and free memory */
        if (bPrivate)
            PrivateFree ((PVOID)ParseItem (szNumber));
        else
            SharedFree ((PVOID)ParseItem (szNumber));

        /* Delete the selected item from the listbox */
        WinSendDlgItemMsg (hWnd, ulCtlID, LM_DELETEITEM, (MPARAM)sIndex, 0L);
    }

    /* Select the first item in the listbox */
    WinSendDlgItemMsg (hWnd, ulCtlID, LM_SELECTITEM, (MPARAM)0L, (MPARAM)TRUE);

    return;
}

ULONG ParseItem (PSZ szItem)
{
    ULONG CurPos = 0;
    ULONG ulMem  = 0;

    /* Convert hexadecimal string to number */
    for (; CurPos < 8; CurPos++)
        ulMem = ulMem*16 +
            ((szItem[CurPos] <= '9') ? (szItem[CurPos] - '0') :
                                       (szItem[CurPos] - 'A' + 10));

    return (ulMem);
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

VOID UpdateNumbers (HWND hWnd)
{
    ULONG ulAlloc, 
          ulSubAlloc,
          ulSharedAlloc,
          ulSharedSubAlloc;

    /* Query private memory allocation info */
    PrivateQuery (&ulAlloc, &ulSubAlloc);

    /* Query shared memory allocation info */
    SharedQuery (&ulSharedAlloc, &ulSharedSubAlloc);

    SetNumberField (hWnd, ulAlloc,          IDC_PRIVATEALLOC);
    SetNumberField (hWnd, ulSubAlloc,       IDC_PRIVATESUBALLOC);
    SetNumberField (hWnd, ulSharedAlloc,    IDC_SHAREDALLOC);
    SetNumberField (hWnd, ulSharedSubAlloc, IDC_SHAREDSUBALLOC);

    /* Free and Free All buttons are enabled only if memory allocated */
    WinEnableControl (hWnd, IDB_PRIVATEFREE, ulSubAlloc);
    WinEnableControl (hWnd, IDB_PRIVATEFREEALL, ulSubAlloc);
    WinEnableControl (hWnd, IDB_SHAREDFREE, ulSharedSubAlloc);
    WinEnableControl (hWnd, IDB_SHAREDFREEALL, ulSharedSubAlloc);

    /* Set the private memory allocation amount as the thermometer value */
    WinSendDlgItemMsg (hWnd, IDC_THERMOMETER, THM_SETVALUE, (MPARAM)ulSubAlloc, 0L);

    return;
}


MRESULT EXPENTRY MainDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    PVOID   pMem;
    ULONG   ulAlloc,
            ulSubAlloc;
                                         
    switch (msg)
    {           
        case WM_INITDLG:
            /* Center the dialog on the screen */
            CenterWindow (hWnd);

            /* Add the About menu item to the system menu */
            AddAboutToSystemMenu (hWnd);

            /* Set the thickness of the border around the dialog */
            WinSendDlgItemMsg (hWnd, IDC_BEVEL1, BVM_SETTHICKNESS, (MPARAM)4, 0L);

            /* Query the suballocation info to set thermometer range */
            PrivateQuery (&ulAlloc, &ulSubAlloc);
            WinSendDlgItemMsg (hWnd, IDC_THERMOMETER, THM_SETRANGE, 0L, (MPARAM)ulAlloc);
            WinSendDlgItemMsg (hWnd, IDC_THERMOMETER, THM_SETVALUE, 0L, 0L);
            WinSendDlgItemMsg (hWnd, IDC_THERMOMETER, THM_SETCOLOR, 
                (MPARAM)MAKELONG(CLR_RED, CLR_PALEGRAY), 0L);

            /* Initialize the memory numbers */
            UpdateNumbers (hWnd);

            /* Initialize the thermometer color spinner control */
            WinSendDlgItemMsg (hWnd, IDC_THERMOCOLOR, SPBM_SETTEXTLIMIT, (MPARAM)9L, (MPARAM)0L);
            WinSendDlgItemMsg (hWnd, IDC_THERMOCOLOR, SPBM_SETARRAY, (MPARAM)pszColors, (MPARAM)15L);
            WinSendDlgItemMsg (hWnd, IDC_THERMOCOLOR, SPBM_SETCURRENTVALUE, (MPARAM)12L, (MPARAM)0L);

            /* Set the focus to the first pushbutton */
            WinSetFocus (HWND_DESKTOP, WinWindowFromID (hWnd, IDB_PRIVATEALLOC));
            break;

        case WM_CONTROL:
            switch (LOUSHORT(mp1))
            {
                case IDC_THERMOCOLOR:
                    if (HIUSHORT(mp1) == SPBN_ENDSPIN)
                    {
                        ULONG ulSelect;

                        /* Query selection index from spinner */
                        WinSendDlgItemMsg (hWnd, IDC_THERMOCOLOR,
                            SPBM_QUERYVALUE, (MPARAM)&ulSelect, 0L);

                        /* Set the current color of the thermometer */
                        WinSendDlgItemMsg (hWnd, IDC_THERMOMETER, THM_SETCOLOR, 
                            (MPARAM)MAKELONG(sColorMap[ulSelect], CLR_PALEGRAY), 0L);
                    }
                    break;
            }
            break;

        case WM_COMMAND:
        case WM_SYSCOMMAND:
		      switch (SHORT1FROMMP(mp1))
            {
                case IDB_PRIVATEALLOC:
                    /* Allocate an arbitrary amount of 1000 bytes */
                    if ((pMem = PrivateAllocate (1000L)) != 0)
                        AddListItem (hWnd, pMem, IDC_PRIVATELIST);
                    break;

                case IDB_PRIVATEFREE:
                    FreeListItem (hWnd, IDC_PRIVATELIST, TRUE);
                    break;

                case IDB_PRIVATEFREEALL:
                    FreeAllListItems (hWnd, IDC_PRIVATELIST, TRUE);
                    break;

                case IDB_SHAREDALLOC:
                    /* Allocate an arbitrary amount of 1000 bytes */
                    if ((pMem = SharedAllocate (1000L)) != 0)
                        AddListItem (hWnd, pMem, IDC_SHAREDLIST);
                    break;

                case IDB_SHAREDFREE:
                    FreeListItem (hWnd, IDC_SHAREDLIST, FALSE);
                    break;

                case IDB_SHAREDFREEALL:
                    FreeAllListItems (hWnd, IDC_SHAREDLIST, FALSE);
                    break;

                case IDM_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;

                case SC_CLOSE:
                    WinDismissDlg (hWnd, FALSE);
                    bHandled = TRUE;
                    break;

                default:
                    bHandled = (msg == WM_COMMAND);
            }

            /* If button was pressed then update the numbers */
		      if (msg == WM_COMMAND)
                UpdateNumbers (hWnd);

            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefDlgProc (hWnd, msg, mp1, mp2);

    return (mReturn);
}

/* --------------------------------------------------------------------
                           MemMap Program
                              Chapter 9

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include "memmap.h"
#include "..\..\common\about.h"

/* Exported Functions */

MRESULT EXPENTRY MainDlgProc (HWND,ULONG,MPARAM,MPARAM);

/* Local Functions */

VOID  CenterWindow (HWND);
VOID  MemError (PSZ);
ULONG ParseItem (PSZ,ULONG);
VOID  QueryMemory (VOID);

HAB   hab;
CHAR  szTitle[64];
HWND  hWndFrame;
HWND  hWndMemList;
HWND  hWndStartAddress;
ULONG ulStartAddress    = 0L;
CHAR  szStartAddress[9] = "00000000";
PVOID pStartAddress     = (PVOID)szStartAddress;

/* Undocumented menu message */
#define MM_QUERYITEMBYPOS      0x01f3
#define MAKE_16BIT_POINTER(p)  ((PVOID)MAKEULONG(LOUSHORT(p),(HIUSHORT(p) << 3) | 7))

int main()
{
    HMQ   hmq;
    QMSG  qmsg;

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    hWndFrame = WinLoadDlg (HWND_DESKTOP, HWND_DESKTOP,
        MainDlgProc, 0, ID_APPNAME, NULL);
    WinSendMsg (hWndFrame, WM_SETICON,
        (MPARAM)WinLoadPointer (HWND_DESKTOP, 0, ID_APPNAME), NULL);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
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

VOID CenterWindow (HWND hWnd)
{
    ULONG ulScrWidth, ulScrHeight;
    RECTL Rectl;

	 ulScrWidth  = WinQuerySysValue (HWND_DESKTOP, SV_CXSCREEN);
	 ulScrHeight = WinQuerySysValue (HWND_DESKTOP, SV_CYSCREEN);
    WinQueryWindowRect (hWnd,&Rectl);
    WinSetWindowPos (hWnd, HWND_TOP, (ulScrWidth-Rectl.xRight)/2,
	      (ulScrHeight-Rectl.yTop)/2, 0, 0, SWP_MOVE | SWP_ACTIVATE);
	 return;
}

VOID MemError (PSZ pszMessage)
{
    WinMessageBox (HWND_DESKTOP, hWndFrame, pszMessage, 
        "Memory Map", 0, MB_ERROR | MB_OK);
    return;
}

ULONG ParseItem (PSZ szItem, ULONG CurPos)
{
    ULONG EndPos = CurPos + 8;
    ULONG ulMem  = 0;
    for (; CurPos < EndPos; CurPos++)
    {
        if (szItem[CurPos] != ' ')
            ulMem = ulMem*16 +
                ((szItem[CurPos] <= '9') ? (szItem[CurPos] - '0') :
                                           (szItem[CurPos] - 'A' + 10));
    }
    return (ulMem);
}

VOID QueryMemory ()
{
    char   szText[90];
    ULONG  ulAddress,
           ulEndAddress,
           ulPage,
           ulSize,
           ulFlags;
    APIRET RetCode;

    WinSetPointer (HWND_DESKTOP,
        WinQuerySysPointer (HWND_DESKTOP, SPTR_WAIT, FALSE));
    WinEnableWindowUpdate (hWndMemList, FALSE);
    ulAddress    = ulStartAddress;
    ulEndAddress = ulStartAddress + 0x02000000;  /* 32 MB max */
    if (ulEndAddress > 0x20000000)
        ulEndAddress = 0x20000000;
    ulPage       = 0L;
    sprintf (szText, "%08lx  . . . . . . . . . . . . . . . .         ",
        ulAddress);
    WinUpper (hab, 0, 0, szText);
    while (ulAddress < ulEndAddress)
    {
        ulSize = 0xFFFFFFFF;
        if (!(RetCode = DosQueryMem ((PVOID)ulAddress, &ulSize, &ulFlags)))
        {
            if (ulFlags == PAG_FREE)
                ulSize = 0x1000;
            else
                ulSize = (ulSize + 0xFFF) & 0xFFFFF000;
            if (ulFlags & PAG_BASE)
                szText[42] = '*';
            while (ulSize && (ulAddress < 0x20000000))
            {
                if (ulFlags & PAG_COMMIT)
                    szText[10 + ulPage*2] = 'C';
                else if (!(ulFlags & PAG_FREE))
                    szText[10 + ulPage*2] = 'A';
                ulAddress += 0x1000;
                ulSize    -= 0x1000;
                ulPage++;
                if (ulPage == 16)
                {
                    if (ulFlags & PAG_SHARED)
                        sprintf (&szText[43], "%s", "Shared ");
                    else if (ulFlags & PAG_FREE)
                        sprintf (&szText[43], "%s", "Free   ");
                    else
                        sprintf (&szText[43], "%s", "Private");
                    if (WinInsertLboxItem (
                           WinWindowFromID (hWndFrame, IDL_MEMLIST), 
                           LIT_END, szText) < 0)
                        break;
                    ulPage = 0;
                    sprintf (szText, "%08lx  . . . . . . . . . . . . . . . .         ",
                        ulAddress);
                    WinUpper (hab, 0, 0, szText);
                }
            }
        }
        else if (RetCode == ERROR_INVALID_ADDRESS)
        {
            ulAddress += 0x10000;
            sprintf (&szText[43], "%s", "Invalid");
            WinInsertLboxItem (WinWindowFromID (hWndFrame, IDL_MEMLIST), 
                LIT_END, szText);
            sprintf (szText, "%08lx  . . . . . . . . . . . . . . . .         ",
                ulAddress);
            WinUpper (hab, 0, 0, szText);
        }
        else
        {
            sprintf (szText, "Error querying memory address %08lx",ulAddress);
            MemError (szText);
            break;
        }
    }
    WinSetPointer (HWND_DESKTOP, 
        WinQuerySysPointer (HWND_DESKTOP,SPTR_ARROW,FALSE));
    WinEnableWindowUpdate (hWndMemList, TRUE);
}


MRESULT EXPENTRY MainDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
                                         
    switch (msg)
    {
        case WM_INITDLG:
            CenterWindow (hWnd);

            /* Add the About menu item to the system menu */
            AddAboutToSystemMenu (hWnd);

            hWndMemList      = WinWindowFromID (hWnd, IDL_MEMLIST);
            hWndStartAddress = WinWindowFromID (hWnd, IDB_STARTADDRESS);

            WinSendMsg (hWndStartAddress, SPBM_SETTEXTLIMIT, (MPARAM)8, (MPARAM)0);
            WinSendMsg (hWndStartAddress, SPBM_SETARRAY, (MPARAM)&pStartAddress, (MPARAM)1L);
            WinSendMsg (hWndStartAddress, SPBM_SETCURRENTVALUE, (MPARAM)0L, (MPARAM)0L);
            WinSetFocus (HWND_DESKTOP, hWndStartAddress);
            WinPostMsg (hWnd, WM_COMMAND, (MPARAM)IDB_REFRESH, 0L);
            break;

        case WM_CONTROL:
            switch (LOUSHORT(mp1))
            {
                case IDL_MEMLIST:
                    if (HIUSHORT(mp1) == LN_SELECT)
                    {
                        WinQueryLboxItemText (hWndMemList,
                            WinQueryLboxSelectedItem((HWND)mp2),
                            szStartAddress,9);
                        ulStartAddress = ParseItem (szStartAddress, 0);
                        WinSendMsg (hWndStartAddress, SPBM_SETARRAY, (MPARAM)&pStartAddress, (MPARAM)1L);
                    }
                    break;

                case IDB_STARTADDRESS:
                    if (HIUSHORT(mp1) == SPBN_UPARROW)
                        ulStartAddress = (ulStartAddress + 0x10000) & 0x1FFFFFFF;
                    else if (HIUSHORT(mp1) == SPBN_DOWNARROW)
                        ulStartAddress = (ulStartAddress - 0x10000) & 0x1FFFFFFF;
                    sprintf (szStartAddress,"%08lx",ulStartAddress);
                    WinUpper (hab, 0, 0, szStartAddress);
                    WinSendMsg (hWndStartAddress, SPBM_SETARRAY, (MPARAM)&pStartAddress, (MPARAM)1L);
                    break;
            }
            break;

		  case WM_COMMAND:
		  case WM_SYSCOMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDB_REFRESH:
                {
                    WinSendDlgItemMsg (hWndFrame, IDL_MEMLIST, LM_DELETEALL, 0L, 0L);
                    QueryMemory ();
                    WinSendMsg (hWndMemList, LM_SELECTITEM, (MPARAM)0, (MPARAM)TRUE);
                    break;
                }

                case IDM_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;

                case SC_CLOSE:
                    WinPostMsg (hWnd, WM_QUIT, 0L, 0L);
                    break;

                default:
                    bHandled = (msg == WM_COMMAND);
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


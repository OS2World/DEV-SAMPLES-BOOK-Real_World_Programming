/* --------------------------------------------------------------------
                           Memory Program
                              Chapter 9

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#include <os2.h>
#include <stdio.h>
#include "memory.h"
#include "..\..\common\about.h"

#define WAIT_POINTER  WinSetPointer (HWND_DESKTOP, \
                       WinQuerySysPointer (HWND_DESKTOP,SPTR_WAIT,FALSE));
#define ARROW_POINTER WinSetPointer (HWND_DESKTOP, \
                       WinQuerySysPointer (HWND_DESKTOP,SPTR_ARROW,FALSE));

/* Exported Functions */

MRESULT EXPENTRY AllocateDlgProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY MainDlgProc (HWND,ULONG,MPARAM,MPARAM);
    
/* Local Functions */

VOID  CenterWindow (HWND);
VOID  ChangeAccessRights (USHORT);
VOID  ChangeCommitState (BOOL);
PSZ   FormatMemListItem (PVOID,LONG,ULONG,ULONG);
PSZ   FormatPageListItem (PVOID,ULONG);
VOID  MemError (PSZ);
ULONG ParseItem (PSZ,ULONG);
VOID  QueryMemory (SHORT);

HAB   hab;
CHAR  szTitle[64];
char  szListItem[40];
char  szPageItem[20];
HWND  hWndFrame;            /* Application Frame window handle */
HWND  hWndMemList;          /* Memory Listbox window handle    */
HWND  hWndPageList;	       /* Page Listbox window handle      */
BOOL  bCommitted = FALSE;   /* Current settings in listbox     */
BOOL  bShared    = FALSE;
BOOL  bRead      = TRUE;
BOOL  bWrite     = TRUE;
BOOL  bExecute   = TRUE;
BOOL  bTiled     = FALSE;

#define FONTNAMELEN 21   /* length + 1 */
CHAR  szListboxFont[] = "10.System Monospaced";

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

PSZ FormatMemListItem (PVOID pMem, LONG lAllocRequest,
                       ULONG ulAllocated, ULONG ulCommitted)
{
    sprintf (szListItem,"%08lx %8lx %8lx %8lx",
        (ULONG)pMem, (ULONG)lAllocRequest, ulAllocated, ulCommitted);
    WinUpper (hab, 0, 0, szListItem);
    return (szListItem);
}

PSZ FormatPageListItem (PVOID pMem, ULONG ulFlags)
{
    sprintf (szPageItem, "%08lx       ", pMem);
    szPageItem[9]  = (CHAR)(ulFlags & PAG_READ    ? 'R' : ' ');
    szPageItem[10] = (CHAR)(ulFlags & PAG_WRITE   ? 'W' : ' ');
    szPageItem[11] = (CHAR)(ulFlags & PAG_EXECUTE ? 'E' : ' ');
    szPageItem[12] = (CHAR)(ulFlags & PAG_COMMIT  ? 'C' : ' ');
    szPageItem[13] = (CHAR)(ulFlags & PAG_SHARED  ? 'S' : ' ');
    WinUpper (hab, 0, 0, szPageItem);
    return (szPageItem);
}

VOID MemError (PSZ pszMessage)
{
    WinMessageBox (HWND_DESKTOP, hWndFrame, pszMessage, 
        "Memory Allocation", 0, MB_ERROR | MB_OK);
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

VOID ChangeAccessRights (USHORT usRights)
{
    ULONG ulAddress;
    SHORT sSel    = -1;
    ULONG ulFlags = 0L;
    BOOL  bError  = FALSE;

    WAIT_POINTER
    ulFlags = (ULONG)(usRights & 0x0007);
    while ((sSel = (SHORT)WinSendMsg (hWndPageList,
        LM_QUERYSELECTION, MPFROMSHORT(sSel),(MPARAM)NULL)) != LIT_NONE)
    {
        WinQueryLboxItemText (hWndPageList, sSel, szPageItem, 20);
        ulAddress = ParseItem(szPageItem, 0);
        if (!DosSetMem ((PVOID)ulAddress, 0x1000, ulFlags))
        {
            ULONG ulNewFlags;
            ULONG ulSize = 0x1000;
            DosQueryMem ((PVOID)ulAddress, &ulSize, &ulNewFlags);
            WinSetLboxItemText (hWndPageList, sSel, 
                FormatPageListItem ((PVOID)ulAddress, ulNewFlags));
        }
        else
        {
            char szError[40];
            if (szPageItem[12] != 'C')
            {
                if (!bError)
                    MemError ("Cannot change access rights of decommitted pages");
                bError = TRUE;
            }
            else
            {
                sprintf (szError, "Unable to change rights for page %08lx", 
                    ulAddress);
                MemError (szError);
                break;
            }
        }
    }
    ARROW_POINTER
    return;
}

VOID ChangeCommitState (BOOL bCommit)
{
    ULONG ulAddress;
    ULONG ulFlags;
    ULONG ulCommitted;  
    SHORT sSel = -1;

    WAIT_POINTER
    ulCommitted = ParseItem(szListItem, 27);
    while ((sSel = (SHORT)WinSendMsg (hWndPageList,
        LM_QUERYSELECTION, MPFROMLONG(sSel),(MPARAM)NULL)) != LIT_NONE)
    {
        WinQueryLboxItemText (hWndPageList, sSel, szPageItem, 20);
        ulAddress = ParseItem(szPageItem,0);
        if (bCommit)
        {
            /* If already committed nothing to do */
            if (szPageItem[12] == 'C')
                continue;
            ulFlags = PAG_COMMIT;
            if (szPageItem[9] == 'R')
                ulFlags |= PAG_READ;
            if (szPageItem[10] == 'W')
                ulFlags |= PAG_WRITE;
            if (szPageItem[11] == 'E')
                ulFlags |= PAG_EXECUTE;
        }
        else
        {
            /* If already decommitted nothing to do */
            if (szPageItem[12] == ' ')
                continue;
            ulFlags = PAG_DECOMMIT;
        }
        if (!DosSetMem ((PVOID)ulAddress, 0x1000, ulFlags))
        {
            /* Requery the item.  Decommitting causes flags to be reset 
               to default (flags defined when memory was allocated) */
            ULONG ulSize = 0x1000;
            DosQueryMem ((PVOID)ulAddress, &ulSize, &ulFlags);
            WinSetLboxItemText (hWndPageList, sSel, 
                FormatPageListItem ((PVOID)ulAddress, ulFlags));
            if (bCommit)
                ulCommitted += 0x1000;
            else
                ulCommitted -= 0x1000;
        }
        else
        {
            char szError[40];
            sprintf (szError, "Unable to %s page %08lx",
                bCommit ? "commit" : "decommit", ulAddress);
            MemError (szError);
            break;
        }
    }
    sprintf (&szListItem[27], "%8lx", ulCommitted);
    WinUpper (hab, 0, 0, szListItem);
    WinSetLboxItemText (hWndMemList, 
        WinQueryLboxSelectedItem(hWndMemList), szListItem);
    ARROW_POINTER
    return;
}

VOID QueryMemory (SHORT sSel)
{
    ULONG ulAddress,
          ulAllocated,
          ulCommitted,
          ulSize,
          ulFlags;
    BOOL  bError = FALSE;
    BOOL  bFirst = TRUE;

    WAIT_POINTER
    WinQueryLboxItemText (hWndMemList, sSel, szListItem, 40);
    WinEnableWindowUpdate (hWndPageList, FALSE);
    WinSendMsg (hWndPageList, LM_DELETEALL, 0L, 0L);

    ulAddress   = ParseItem (szListItem, 0);
    ulCommitted = 0L;
    ulAllocated = 0L;
    ulSize      = 0xFFFFFFFF;
    while (!DosQueryMem ((PVOID)ulAddress, &ulSize, &ulFlags) &&
           (bFirst || !(ulFlags & (PAG_BASE | PAG_FREE))))
    {
        bFirst       = FALSE;
        ulAllocated += ulSize;
        if (ulFlags & PAG_COMMIT)
            ulCommitted += ulSize;
        while (ulSize)
        {
            if (WinInsertLboxItem (hWndPageList, LIT_END,
                    FormatPageListItem ((PVOID)ulAddress, ulFlags)) < 0)
            {
                if (!bError)
                    MemError ("Unable to add any more list items");
                bError = TRUE;
            }
            ulAddress += 0x1000;
            ulSize    -= 0x1000;
        }
        ulSize      = 0xFFFFFFFF;
    }
    WinEnableWindowUpdate (hWndPageList, TRUE);
    WinSendMsg (hWndPageList, LM_SELECTITEM, (MPARAM)0L, (MPARAM)TRUE);
    sprintf (&szListItem[18], "%8lx %8lx", ulAllocated, ulCommitted);
    WinUpper (hab, 0, 0, szListItem);
    WinSetLboxItemText (hWndMemList, sSel, szListItem);
    ARROW_POINTER
    
    return;
}

MRESULT EXPENTRY AllocateDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    LONG lMemAlloc;                            

    switch (msg)
    {
        case WM_INITDLG:
            CenterWindow (hWnd);
            WinCheckButton (hWnd, 
                (bCommitted ? IDB_COMMITTED : IDB_UNCOMMITTED), TRUE);
            WinCheckButton (hWnd, 
                (bShared    ? IDB_SHARED    : IDB_NONSHARED), TRUE);
            WinCheckButton (hWnd, IDB_READ,    bRead);
            WinCheckButton (hWnd, IDB_WRITE,   bWrite);
            WinCheckButton (hWnd, IDB_EXECUTE, bExecute);
            WinCheckButton (hWnd, IDB_TILED,   bTiled);
            WinSendDlgItemMsg (hWnd, IDB_NUMBYTES, SPBM_SETTEXTLIMIT, 
                (MPARAM)7, (MPARAM)0);
            WinSendDlgItemMsg (hWnd, IDB_NUMBYTES, SPBM_SETLIMITS,
                (MPARAM)0x00100000,(MPARAM)1L);
            WinSendDlgItemMsg (hWnd, IDB_NUMBYTES, SPBM_SETCURRENTVALUE,
                (MPARAM)1,(MPARAM)0L);
            WinSendDlgItemMsg (hWnd, IDB_REPEAT, SPBM_SETTEXTLIMIT, 
                (MPARAM)4, (MPARAM)0);
            WinSendDlgItemMsg (hWnd, IDB_REPEAT, SPBM_SETLIMITS,
                (MPARAM)9999L,(MPARAM)1L);
            WinSendDlgItemMsg (hWnd, IDB_REPEAT, SPBM_SETCURRENTVALUE,
                (MPARAM)1,(MPARAM)0L);
            WinSetFocus (HWND_DESKTOP, WinWindowFromID (hWnd, IDB_NUMBYTES));
            return ((MRESULT)TRUE);

        case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case DID_OK:
                    WinSendDlgItemMsg (hWnd, IDB_NUMBYTES, SPBM_QUERYVALUE, 
                        (MPARAM)&lMemAlloc, (MPARAM)0L);
                    if (lMemAlloc)
                    {
                        ULONG flags = 0L;
                        BOOL  bOkay;
                        PVOID pMem;
                        ULONG ulRepeat;

                        if (lMemAlloc < 0)
                            lMemAlloc = -lMemAlloc;
                        bCommitted = WinQueryButtonCheckstate (hWnd, IDB_COMMITTED);
                        bShared    = WinQueryButtonCheckstate (hWnd, IDB_SHARED);
                        bRead      = WinQueryButtonCheckstate (hWnd, IDB_READ);
                        bWrite     = WinQueryButtonCheckstate (hWnd, IDB_WRITE);
                        bExecute   = WinQueryButtonCheckstate (hWnd, IDB_EXECUTE);
                        bTiled     = WinQueryButtonCheckstate (hWnd, IDB_TILED);
                        if (bCommitted)
                            flags |= PAG_COMMIT;
                        if (bShared)
                            flags |= OBJ_GETTABLE | OBJ_GIVEABLE; 
                        if (bRead)
                            flags |= PAG_READ;
                        if (bWrite)
                            flags |= PAG_WRITE;
                        if (bExecute)
                            flags |= PAG_EXECUTE;
                        if (bTiled)
                            flags |= OBJ_TILE;

                        WinSendDlgItemMsg (hWnd, IDB_REPEAT, SPBM_QUERYVALUE, 
                            (MPARAM)&ulRepeat, (MPARAM)0L);

                        while (ulRepeat--)
                        {
                            if (!bShared)
                                bOkay = !DosAllocMem ((PPVOID)&pMem, lMemAlloc, flags);
                            else
                                bOkay = !DosAllocSharedMem ((PPVOID)&pMem, NULL, lMemAlloc,
                                            flags);
                            if (bOkay)
                            {
                                SHORT sSel;
                                sSel = (SHORT)WinInsertLboxItem (hWndMemList,
                                    LIT_SORTASCENDING,
                                    FormatMemListItem (pMem, lMemAlloc, 0L, 0L));
                                QueryMemory (sSel);
                                WinSendMsg (hWndMemList, LM_SELECTITEM, 
                                    (MPARAM)sSel, (MPARAM)TRUE);
                            }
                            else
                            {
                                MemError ("Unable to allocate memory");
                                break;
                            }
                        }
                    }

                /* FALL THROUGH */

                case DID_CANCEL:
                    WinDismissDlg (hWnd, TRUE);
                    return (0);
            }
            break;
    }
    return (WinDefDlgProc (hWnd, msg, mp1, mp2));
}


MRESULT EXPENTRY MainDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    SHORT   sSel;
    RECTL   Rectl;
    BOOL    bHandled       = TRUE;
    MRESULT mReturn        = 0;

    static  HWND hMenus[4] = { 0, 0, 0, 0 };

    switch (msg)
    {
        case WM_INITDLG:
            CenterWindow (hWnd);
            hWndMemList  = WinWindowFromID (hWnd, IDL_MEMLIST);
            hWndPageList = WinWindowFromID (hWnd, IDL_PAGELIST);

            /* Set the system monospaced font for the listboxes */
            WinSetPresParam (hWndMemList,
                PP_FONTNAMESIZE, FONTNAMELEN, szListboxFont);
            WinSetPresParam (hWndPageList,
                PP_FONTNAMESIZE, FONTNAMELEN, szListboxFont);

            /* Load the popup menus */
            hMenus[0] = WinLoadMenu (HWND_OBJECT, 0, FREE_MENU);
            hMenus[1] = WinLoadMenu (HWND_OBJECT, 0, SELECT_MENU);
            hMenus[2] = WinLoadMenu (HWND_OBJECT, 0, STATE_MENU);
            hMenus[3] = WinLoadMenu (HWND_OBJECT, 0, RIGHTS_MENU);

            /* Add the About menu item to the system menu */
            AddAboutToSystemMenu (hWnd);
            break;

        case WM_CONTROL:
            switch (LOUSHORT(mp1))
            {
                case IDL_MEMLIST:
                    if (HIUSHORT(mp1) == LN_SELECT)
                        QueryMemory ((SHORT)WinQueryLboxSelectedItem((HWND)mp2));
                    break;
            }
            break;

		  case WM_COMMAND:
		  case WM_SYSCOMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDB_ALLOCATE:
                    WinDlgBox (HWND_DESKTOP, hWnd, AllocateDlgProc,
                        0, IDD_ALLOCATE, NULL);
                    break;

                case IDB_SELECT:
                case IDB_FREE:
                case IDB_STATE:
                case IDB_RIGHTS:
                    WinQueryWindowRect (WinWindowFromID(hWnd, LOUSHORT(mp1)), &Rectl);
                    WinMapWindowPoints (WinWindowFromID(hWnd, LOUSHORT(mp1)), hWnd,
                         (PPOINTL)&Rectl, 2);
                    WinPopupMenu (hWnd, hWnd,
                        hMenus[LOUSHORT(mp1) / 100 - 1],
                        Rectl.xLeft, Rectl.yTop,
                        LOUSHORT(mp1) + 1,
                        PU_HCONSTRAIN | PU_VCONSTRAIN | PU_KEYBOARD |
                        PU_MOUSEBUTTON1 | PU_SELECTITEM);
                    break;

                case IDM_SELECT_NONE:
                       WinSendMsg (hWndPageList, LM_SELECTITEM, (MPARAM)LIT_NONE, 0L);
                       break;

                case IDM_SELECT_ALL:
                   {
                       SHORT sCount;
                       sCount = (SHORT)WinQueryLboxCount (hWndPageList);
                       while (sCount--)
                           WinSendMsg (hWndPageList, LM_SELECTITEM, (MPARAM)sCount, (MPARAM)TRUE);
                   }
                   break;

                case IDM_COMMIT_SELECTED:
                case IDM_DECOMMIT_SELECTED:
                    ChangeCommitState (LOUSHORT(mp1) == IDM_COMMIT_SELECTED);
                    break;

                case IDM_READ:
                case IDM_WRITE:
                case IDM_EXECUTE:
                case IDM_READWRITE:
                case IDM_READEXECUTE:
                case IDM_WRITEEXECUTE:
                case IDM_READWRITEEXECUTE:
                    ChangeAccessRights (LOUSHORT(mp1));
                    break;

                case IDM_FREE_SELECTED:
                    if ( (sSel = (SHORT)WinQueryLboxSelectedItem (hWndMemList)) != 
                            LIT_NONE)
                    {
                        char szMem[9];
                        ULONG ulMem;

                        WinQueryLboxItemText (hWndMemList, sSel, szMem, 9);
                        ulMem = ParseItem (szMem,0);
                        if (!DosFreeMem ((PVOID)ulMem))
                        {
                            WinDeleteLboxItem (hWndMemList, sSel);
                            WinSendMsg (hWndPageList, LM_DELETEALL, 0L, 0L);
                            if (sSel >= (SHORT)WinQueryLboxCount(hWndMemList))
                                sSel--;
                            WinSendMsg (hWndMemList, LM_SELECTITEM, (MPARAM)sSel, (MPARAM)TRUE);
                        }
                        else
                            MemError ("Error freeing memory");
                    }
                    break;

                case IDM_FREE_ALL:
                {
                    char szMem[9];
                    ULONG ulMem;
                    while (WinQueryLboxItemText (hWndMemList, 0, szMem, 9) > 0)
                    {
                        ulMem = ParseItem (szMem,0);
                        if (!DosFreeMem ((PVOID)ulMem))
                            WinDeleteLboxItem (hWndMemList, 0);
                        else
                        {
                            MemError ("Error freeing memory");
                            break;
                        }
                    }
                    WinSendMsg (hWndPageList, LM_DELETEALL, 0L, 0L);
                    break;
                }

                case IDM_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;

                case SC_CLOSE:
                    WinDestroyWindow (hMenus[0]);
                    WinDestroyWindow (hMenus[1]);
                    WinDestroyWindow (hMenus[2]);
                    WinDestroyWindow (hMenus[3]);
                    WinPostMsg (hWnd, WM_COMMAND, (MPARAM)IDM_FREE_ALL, 0L);
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



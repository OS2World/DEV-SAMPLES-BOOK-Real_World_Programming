/* --------------------------------------------------------------------
                          MiniMDI Program
                              Chapter 3

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include <stdio.h>
#include "minimdi.h"
#include "..\common\about.h"

/* Window and Dialog Functions      */
MRESULT EXPENTRY ClientWndProc  (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY ChildWndProc   (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY ToolBarDlgProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY WinInfoDlgProc (HWND,ULONG,MPARAM,MPARAM);

/* Window Subclass Functions        */
MRESULT EXPENTRY ChildSubclassProc    (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY FrameSubclassProc    (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY MenuSubclassProc     (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY SysMenuSubclassProc  (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY TitleBarSubclassProc (HWND,ULONG,MPARAM,MPARAM);

/* Local Functions                  */
VOID ArrangeIcons (HWND);
VOID CascadeChildWindows (HWND);
VOID CenterWindow (HWND);
VOID CloseAllChildWindows (HWND);
VOID CreateChildWindow (HWND);
VOID GetNextWindowPos (ULONG, PSWP);
VOID InitializeAppWindow (HWND);
BOOL IsWindowMaximized (HWND);
BOOL IsWindowMinimized (HWND);
VOID PaintTitleBar (HWND, HPS);
VOID SwitchToNextChildWindow (VOID);
VOID TileChildWindows (HWND);

/* Undocumented PM Functions        */
extern BOOL APIENTRY Win32StretchPointer(HPS hps, SHORT x, SHORT y,
                       SHORT cx, SHORT cy, HPOINTER hptr, USHORT fs);

/* Undocumented WinDrawBorder Flags */
#define DB_RAISED        0x0400
#define DB_CORNERBORDER  0x8000

/* User-Defined messages            */
#define WM_USER_POSITION      WM_USER+1
#define WM_USER_CHILDACTIVATE WM_USER+2

/* User-Defined Window Extra Words  */
#define QWL_COLOR_RGB  QWL_USER                        /* ULONG  */
#define QWL_MOVE       QWL_COLOR_RGB + sizeof(ULONG)   /* BOOL   */ 
#define QWL_TRACKING   QWL_MOVE      + sizeof(BOOL)    /* BOOL   */
#define QWL_OPEN       QWL_TRACKING  + sizeof(BOOL)    /* BOOL   */
#define QWL_EXTRA      QWL_OPEN      + sizeof(BOOL)

/* Global Variables                 */
HAB      hab;
HWND     hWndFrame, 
         hWndClient,
         hWndToolBar,
         hWndActiveChild;
CHAR     szTitle[64];
BOOL     bAppIsActive     = FALSE;
ULONG    ulNumChildren    = 0L;
ULONG    ulNumMinChildren = 0L;

/* Pointer Handles                  */
HPOINTER hPtrHeart        = 0;
HPOINTER hPtrClient       = 0;
HPOINTER hPtrSysMenu      = 0;
HPOINTER hPtrMenu         = 0;
HPOINTER hPtrMinMax       = 0;
HPOINTER hPtrLeft         = 0;
HPOINTER hPtrTop          = 0;
HPOINTER hPtrLeftTop      = 0;
HPOINTER hPtrRightTop     = 0;
HPOINTER hPtrTitleBar     = 0;
HPOINTER hPtrPopupMenu    = 0;

/* Subclassed PM Window Callbacks   */
PFNWP    pfnFrameProc;                 /* Frame Window       */    
PFNWP    pfnTitleBarProc;              /* TitleBar Window    */
PFNWP    pfnSysMenuProc;               /* System Menu Window */
PFNWP    pfnMenuProc;                  /* Menu Window        */

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
    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
        &flFrameFlags, szClientClass, szTitle, 0, 0, ID_APPNAME, &hWndClient);

    /* Subclass the frame control windows */
    pfnFrameProc    = WinSubclassWindow (hWndFrame,(PFNWP)FrameSubclassProc);
    pfnTitleBarProc = WinSubclassWindow (
      WinWindowFromID (hWndFrame, FID_TITLEBAR),(PFNWP)TitleBarSubclassProc);
    pfnSysMenuProc  = WinSubclassWindow (
      WinWindowFromID (hWndFrame, FID_SYSMENU),(PFNWP)SysMenuSubclassProc);
    pfnMenuProc     = WinSubclassWindow (
      WinWindowFromID (hWndFrame, FID_MENU),(PFNWP)MenuSubclassProc);

    InitializeAppWindow (hWndClient);

    WinShowWindow (hWndFrame, TRUE);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}

/* ----------------------- Local Functions ----------------------- */

VOID ArrangeIcons (HWND hWnd)
{
    PSWP  pSwp;

    /* Allocate memory to hold the SWP structures */
    if (!DosAllocMem ((PPVOID)&pSwp, sizeof(SWP) * ulNumMinChildren, fALLOC))
    {
        HWND  hWndChild;
        ULONG ulChildCnt;
        HENUM hEnum;

        ulChildCnt  = 0;

        /* Begin child window enumeration */
        hEnum = WinBeginEnumWindows (hWnd);

        /* Get handle to each child frame window */
        while ((hWndChild = WinGetNextWindow (hEnum)) != 0)
        {
            if ( IsWindowMinimized (hWndChild) )
            {
                pSwp[ulChildCnt].fl     = SWP_MOVE | SWP_FOCUSDEACTIVATE;
                pSwp[ulChildCnt].x      = 
                pSwp[ulChildCnt].y      = 0;
                pSwp[ulChildCnt++].hwnd = hWndChild;
                /* Destroy the current minimize position -- a little
                   PM trick */
                WinSetWindowUShort (hWndChild,QWS_XMINIMIZE,(USHORT)-1);
                WinSetWindowUShort (hWndChild,QWS_YMINIMIZE,(USHORT)-1);
            }
        }

        /* End child window enumeration */
        WinEndEnumWindows (hEnum);

        /* Arrange the iconic windows   */
        WinSetMultWindowPos (hab, pSwp, ulChildCnt);          

        /* Free the allocated memory    */
        DosFreeMem ((PVOID)pSwp);
    }

    return;
}

VOID CascadeChildWindows (HWND hWnd)
{
    PSWP  pSwp;
    ULONG ulChildCnt = (ulNumChildren - ulNumMinChildren);
                                           
    /* Allocate memory to hold the SWP structures */
    if (!DosAllocMem ((PPVOID)&pSwp, sizeof(SWP) * ulChildCnt, fALLOC))
    {
        HWND  hWndChild;

        ulChildCnt = 0L;

        /* Query the bottommost child frame window in the z-order */
        hWndChild  = WinQueryWindow (hWnd, QW_BOTTOM);

        while (hWndChild)
        {
            /* Skip ICONTEXT windows and minimized windows */
            if ( WinQueryWindowUShort (hWndChild, QWS_ID) &&
                 !IsWindowMinimized (hWndChild) )
            {
                GetNextWindowPos (ulChildCnt, &pSwp[ulChildCnt]);
                pSwp[ulChildCnt].fl = SWP_MOVE | SWP_SIZE | SWP_SHOW;

                /* If child is maximized then restore it */
                if (IsWindowMaximized (hWndChild))
                    pSwp[ulChildCnt].fl |= SWP_RESTORE;

                pSwp[ulChildCnt++].hwnd = hWndChild;
            }             

            /* Query the previous child frame window in the z-order */                                        
            hWndChild = WinQueryWindow (hWndChild, QW_PREV);
        }

        /* Arrange the cascaded windows */
        WinSetMultWindowPos (hab, pSwp, ulChildCnt);

        /* Free the allocated memory    */
        DosFreeMem ((PVOID)pSwp);
    }

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

VOID CloseAllChildWindows (HWND hWnd)
{
    HWND  hWndChild;
    HENUM hEnum;

    /* Begin child window enumeration */
    hEnum = WinBeginEnumWindows (hWnd);

    /* Get handle to each child frame window */
    while ((hWndChild = WinGetNextWindow (hEnum)) != 0)
        WinDestroyWindow (hWndChild);

    /* End child window enumeration */
    WinEndEnumWindows (hEnum);

    return;
}

VOID CreateChildWindow (HWND hWnd)
{
   HWND  hWndChild;
   HWND  hWndChildFrame;
   CHAR  szTitle[23];
   ULONG flChildFrameFlags  = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                              FCF_MINMAX   | FCF_ICON | FCF_NOBYTEALIGN;

   if ((hWndChildFrame = WinCreateStdWindow (hWnd, 0L, &flChildFrameFlags, 
          "CHILD", "", 0L, 0L, ID_CHILDWINDOW, &hWndChild)) != 0)
   {
       SWP Swp;

       /* If the currently active child is maximized restore it */
       if (hWndActiveChild && IsWindowMaximized (hWndActiveChild))
           WinSetWindowPos (WinQueryWindow (hWndActiveChild, QW_PARENT),
               0L, 0L, 0L, 0L, 0L, SWP_RESTORE);

       WinSetOwner (hWndChild, hWndChildFrame);

       sprintf (szTitle, "Child Window %04x:%04x", 
              HIUSHORT(hWndChildFrame), LOUSHORT(hWndChildFrame));
       WinSetWindowText  (hWndChildFrame, szTitle);

       GetNextWindowPos (ulNumChildren - 1L, &Swp);
       WinSetWindowPos (hWndChildFrame, 0L, Swp.x, Swp.y, Swp.cx, Swp.cy,
           SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ACTIVATE);

       /* Force a repaint of the title bar window */
       WinInvalidateRect (WinWindowFromID(hWndFrame,FID_TITLEBAR),
           NULL, FALSE);
   }

   return;                                                           
}

VOID GetNextWindowPos (ULONG ulCnt, PSWP pSwp)
{
    RECTL Rectl;
    ULONG ulXDiff,
          ulYDiff,
          ulWindowsPerStack;

    WinQueryWindowRect (hWndClient, &Rectl);
    if (ulNumMinChildren)
        Rectl.yBottom += WinQuerySysValue (HWND_DESKTOP, SV_CYICON) * 2;

    ulXDiff  = WinQuerySysValue (HWND_DESKTOP, SV_CXSIZEBORDER) +
          WinQuerySysValue (HWND_DESKTOP, SV_CXMINMAXBUTTON) / 2;
    ulYDiff  =  WinQuerySysValue (HWND_DESKTOP, SV_CYSIZEBORDER) +
          WinQuerySysValue (HWND_DESKTOP, SV_CYMINMAXBUTTON);

    ulWindowsPerStack  = (Rectl.yTop - Rectl.yBottom) / (3 * ulYDiff);
    pSwp->cx = Rectl.xRight - (ulWindowsPerStack * ulXDiff);
    pSwp->cy = (Rectl.yTop - Rectl.yBottom) - (ulWindowsPerStack * ulYDiff);
    ulWindowsPerStack++;
    pSwp->x  = (ulCnt % ulWindowsPerStack) * ulXDiff;
    pSwp->y  = Rectl.yTop - pSwp->cy - ((ulCnt % ulWindowsPerStack) * ulYDiff);

    return;
}

VOID InitializeAppWindow (HWND hWnd)
{
   WinRegisterClass (hab, "CHILD", ChildWndProc, 0, QWL_EXTRA);

   hWndToolBar = WinLoadDlg (HWND_DESKTOP, hWnd, ToolBarDlgProc,  
       0L, IDD_TOOLBAR, NULL);

   /* Create three children initially */
   CreateChildWindow (hWnd);   
   CreateChildWindow (hWnd);   
   CreateChildWindow (hWnd);   

   WinPostMsg (hWndToolBar, WM_USER_POSITION, 0L, 0L);
   WinPostMsg (hWnd, WM_COMMAND, (MPARAM)IDM_TILE, 0L);
   return;                                                           
}

BOOL IsWindowMaximized (HWND hWnd)
{
   /* If hWnd is the client window query the frame window (ie. the parent) */
   if (WinQueryWindowUShort (hWnd, QWS_ID) == FID_CLIENT)
       hWnd = WinQueryWindow (hWnd, QW_PARENT);
   return (WinQueryWindowULong (hWnd, QWL_STYLE) & WS_MAXIMIZED);
}

BOOL IsWindowMinimized (HWND hWnd)
{
   if (WinQueryWindowUShort (hWnd, QWS_ID) == FID_CLIENT)
       hWnd = WinQueryWindow (hWnd, QW_PARENT);
   return (WinQueryWindowULong (hWnd, QWL_STYLE) & WS_MINIMIZED);
}

VOID PaintTitleBar (HWND hWnd, HPS hps)
{
    RECTL Rectl;
    BOOL  bActive;
    SHORT Scale,
          X;
    ULONG inx;

    /* If window is not visible then nothing to paint */
    if (!WinIsWindowVisible (hWnd))
        return;

    /* Get the current active state of the title bar */
    bActive = (BOOL)(WinQueryWindowULong (hWnd, QWL_USER) & 1);

    /* Draw the correct border for the title bar */
    WinQueryWindowRect (hWnd, &Rectl);
    WinDrawBorder (hps, &Rectl, bActive ? 0L : 1L, 1L, SYSCLR_TITLEBOTTOM, 
        bActive ? SYSCLR_ACTIVETITLE : SYSCLR_INACTIVETITLE,
        DB_RAISED | DB_CORNERBORDER | DB_STANDARD | DB_INTERIOR);

    /* Draw a heart for each child window that exists. 
       Scale the pointer to fit in the title bar height minus 2 minus the
       nominal border width times 2.
       Since the WC_TITLEBAR class is not CS_CLIPSIBLINGS it is possible to
       write over the minmax window.  Only output as many hearts as will
       fit in the title bar window.
       Use the Win32StretchPointer API so we get a transparent bitblt. */

    Scale = (SHORT)(Rectl.yTop - Rectl.yBottom - 2 -
            2 * WinQuerySysValue (HWND_DESKTOP, SV_CYBORDER));

    if (bAppIsActive)
    {
        if ((inx = ulNumChildren) != 0)
        {
            X = (SHORT)(WinQuerySysValue (HWND_DESKTOP, SV_CXBORDER) + 1);
            while ( inx && ((LONG)(X + Scale) < Rectl.xRight) )
            {
                Win32StretchPointer (hps, X, 2, Scale, Scale, hPtrHeart,
                    DP_NORMAL);
                inx--;
                X += (SHORT)(Scale + 2);
            }
        }
        else
        {
            POINTL Ptl;
            Ptl.x = WinQuerySysValue (HWND_DESKTOP, SV_CXBORDER) + 1;
            Ptl.y = 2;
            GpiCharStringAt (hps, &Ptl, 21L, (PSZ)"MiniMDI - No children");
        }
    }
    else
    {
        POINTL Ptl;
        Ptl.x = WinQuerySysValue (HWND_DESKTOP, SV_CXBORDER) + 1;
        Ptl.y = 2;
        GpiCharStringAt (hps, &Ptl, 20L, (PSZ)"MiniMDI - Not Active");
    }

    return;
} 

VOID SwitchToNextChildWindow ()
{
    HWND hWndNext,
         hWndActiveFrame;

    hWndNext        = 
    hWndActiveFrame = WinQueryWindow (hWndActiveChild, QW_PARENT);

    /* We need to query the next until we find a non-WC_ICONTEXT window */
    while ( ((hWndNext = WinQueryWindow (hWndNext, QW_NEXT)) != 0) &&
            !WinQueryWindowUShort (hWndNext, QWS_ID))
    {
    }
    if (hWndNext)
    {
        /* If current active child is maximized then restore it */
        WinSetWindowPos (hWndNext, HWND_TOP, 0, 0, 0, 0,
            SWP_ZORDER | SWP_ACTIVATE);
        WinSetWindowPos (hWndActiveFrame, HWND_BOTTOM, 0, 0, 0, 0,
            IsWindowMaximized (hWndActiveFrame) ? SWP_RESTORE | SWP_ZORDER : 
                                                  SWP_ZORDER);
    }

    return;
}

VOID TileChildWindows (HWND hWnd)
{

    PSWP  pSwp;
    ULONG ulChildCnt = (ulNumChildren - ulNumMinChildren);

    /* Allocate memory to hold the SWP structures */
    if (!DosAllocMem ((PPVOID)&pSwp, sizeof(SWP) * ulChildCnt, fALLOC))
    {
        ULONG ulSquare,
              ulNumRows,
              ulNumCols,
              ulExtraCols,
              ulWidth,
              ulHeight;
        RECTL Rectl;
        HWND  hWndChild;

        for (ulSquare = 2; ulSquare * ulSquare <= ulChildCnt; ulSquare++)
        {}

        ulNumCols   = ulSquare - 1;
        ulNumRows   = ulChildCnt / ulNumCols;
        ulExtraCols = ulChildCnt % ulNumCols;

        WinQueryWindowRect (hWnd, &Rectl);
        if (ulNumMinChildren)
            Rectl.yBottom += WinQuerySysValue (HWND_DESKTOP, SV_CYICON) * 2;
        if (Rectl.xRight > 0L && (Rectl.yBottom < Rectl.yTop))
        {
            HENUM hEnum;

            /* Begin child window enumeration */
            hEnum = WinBeginEnumWindows (hWnd);

            /* Get handle to each child frame window */
            if ((hWndChild = WinGetNextWindow (hEnum)) != 0)
            {
                ULONG ulCurRow, 
                      ulCurCol;

                ulChildCnt = 0L;
                ulHeight   = (Rectl.yTop - Rectl.yBottom) / ulNumRows;

                for (ulCurRow = 0; ulCurRow < ulNumRows; ulCurRow++)
                {
                    if ((ulNumRows - ulCurRow) <= ulExtraCols)
                        ulNumCols++;
                    for (ulCurCol = 0; ulCurCol < ulNumCols; ulCurCol++)
                    {
                        ulWidth = Rectl.xRight / ulNumCols;

                        /* Skip ICONTEXT windows and minimized windows */
                        while ( hWndChild &&
                                ( !WinQueryWindowUShort (hWndChild, QWS_ID) ||
                                  IsWindowMinimized (hWndChild) ) )
                            hWndChild = WinGetNextWindow (hEnum);

                        if (hWndChild)
                        {
                            pSwp[ulChildCnt].fl     = 
                                SWP_MOVE | SWP_SIZE | SWP_SHOW;

                            /* If child is maximized then restore it */
                            if (IsWindowMaximized (hWndChild))
                                pSwp[ulChildCnt].fl |= SWP_RESTORE;

                            pSwp[ulChildCnt].x      = ulWidth * ulCurCol;
                            pSwp[ulChildCnt].y      = 
                                Rectl.yTop - (ulHeight * (ulCurRow + 1));
                            pSwp[ulChildCnt].cx     = ulWidth;
                            pSwp[ulChildCnt].cy     = ulHeight;
                            pSwp[ulChildCnt++].hwnd = hWndChild;

                            /* Get handle to next child frame window */
                            hWndChild = WinGetNextWindow (hEnum);
                        }       
                    }
                    if ((ulNumRows - ulCurRow) <= ulExtraCols)
                    {
                        ulNumCols--;
                        ulExtraCols--;
                    }
                }
            }

            /* End child window enumeration */
            WinEndEnumWindows (hEnum);
        }

        /* Arrange the tiled windwos */
        WinSetMultWindowPos (hab, pSwp, ulChildCnt);

        /* Free the allocated memory    */
        DosFreeMem ((PVOID)pSwp);
    }

    return;
}

/* ----------------- Window and Dialog Functions ----------------- */

MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    RECTL   Rectl;
    HPS     hps;

    switch (msg)
    {
        case WM_CREATE:
            hPtrHeart       = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_HEART);
            hPtrClient      = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_CLIENT);
            hPtrSysMenu     = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_SYSMENU);
            hPtrMenu        = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_MENU);
            hPtrMinMax      = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_MINMAX);
            hPtrLeft        = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_LEFT);
            hPtrTop         = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_TOP);
            hPtrLeftTop     = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_LEFTTOP);
            hPtrRightTop    = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_RIGHTTOP);
            hPtrTitleBar    = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_TITLEBAR);
            hPtrPopupMenu   = WinLoadPointer (HWND_DESKTOP, 0L, IDPTR_POPUPMENU);
            break;

        case WM_MOUSEMOVE:
            WinSetPointer (HWND_DESKTOP, hPtrClient);
            break;

        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            WinQueryWindowRect (hWnd, &Rectl);
            WinFillRect (hps, &Rectl, CLR_WHITE);
            WinEndPaint (hps);
            mReturn = 0L;
            break;

        case WM_FOCUSCHANGE:
            if ( WinQueryWindowULong (hWnd, QWL_HMQ) !=
                 WinQueryWindowULong ((HWND)mp1, QWL_HMQ) )
            {
                if (SHORT1FROMMP(mp2))
                    /* Application is gaining activation */
                    bAppIsActive = TRUE;
                else
                    /* Application is losing activation */
                    bAppIsActive = FALSE;

                /* Force a repaint of the title bar window */
                WinInvalidateRect (WinWindowFromID(hWndFrame,FID_TITLEBAR),
                    NULL, FALSE);
            }    
            bHandled = FALSE; 
            break;

        case WM_INITMENU:
            if (SHORT1FROMMP(mp1) == IDM_ARRANGEMENU)
            {
                BOOL bEnable = (ulNumChildren != ulNumMinChildren);
                WinEnableMenuItem ((HWND)mp2, IDM_TILE,    bEnable);
                WinEnableMenuItem ((HWND)mp2, IDM_CASCADE, bEnable);
                WinEnableMenuItem ((HWND)mp2, IDM_ARRANGE, ulNumMinChildren);
            }
            else if (SHORT1FROMMP(mp1) == IDM_WINDOW)
            {
                WinEnableMenuItem ((HWND)mp2, IDM_NEXTCHILD, ulNumChildren > 1);
                WinEnableMenuItem ((HWND)mp2, IDM_CLOSEALL,  ulNumChildren);
            }
            break;

		  case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDM_NEWCHILD:
                    CreateChildWindow (hWnd);
                    break;

                case IDM_NEXTCHILD:
                    if (hWndActiveChild)
                        SwitchToNextChildWindow ();
                    break;

				    case IDM_TILE:
                    TileChildWindows (hWnd);
						  break;

                case IDM_CASCADE:
                    CascadeChildWindows (hWnd);
                    break;

                case IDM_ARRANGE:
                    ArrangeIcons (hWnd);
                    break;

                case IDM_CLOSEALL:
                    CloseAllChildWindows (hWnd);
                    break;

                case IDM_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;
            }
            break;

        case WM_DESTROY:
            WinDestroyPointer (hPtrHeart);
            WinDestroyPointer (hPtrClient);
            WinDestroyPointer (hPtrSysMenu);     
            WinDestroyPointer (hPtrMenu);        
            WinDestroyPointer (hPtrMinMax);      
            WinDestroyPointer (hPtrLeft);        
            WinDestroyPointer (hPtrTop);         
            WinDestroyPointer (hPtrLeftTop);     
            WinDestroyPointer (hPtrRightTop);    
            WinDestroyPointer (hPtrTitleBar);    
            WinDestroyPointer (hPtrPopupMenu);   
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}

MRESULT EXPENTRY ChildWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mReturn = 0L;
    ULONG   ulColorRGB;
    PSWP    pSwp;
    RECTL   Rectl;
    HPS     hps;

    switch (msg)
    {
        case WM_CREATE:
            /* Initialize window extra words */
            WinSetWindowULong (hWnd, QWL_COLOR_RGB, 0x00FFFFFF);
            WinSetWindowULong (hWnd, QWL_MOVE,      TRUE);
            WinSetWindowULong (hWnd, QWL_TRACKING,  FALSE);
            WinSetWindowULong (hWnd, QWL_OPEN,      TRUE);
            ulNumChildren++;

            /* If this is first child window then enable the toolbar */
            if (ulNumChildren == 1)
                WinEnableWindow (hWndToolBar, TRUE);
            break;
            
        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            WinQueryWindowRect (hWnd, &Rectl);
            /* Set color state in the hps to RGB mode */
            GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0L, 0L, NULL);
            ulColorRGB = WinQueryWindowULong (hWnd, QWL_COLOR_RGB);
            WinFillRect (hps, &Rectl, ulColorRGB);
            WinEndPaint (hps);
            break;

        case WM_MINMAXFRAME:
            pSwp = (PSWP)mp1;

            /* Are we becoming minimized? */
            if (pSwp->fl & SWP_MINIMIZE)
                ulNumMinChildren++;
            else 
            {
                /* hWnd is the client window handle -- use the frame */
                HWND hWndFrame = WinQueryWindow (hWnd, QW_PARENT);

                if (IsWindowMinimized (hWndFrame))
                {
                    /* Is the child window allowed to restore itself? */
                    if (WinQueryWindowULong (hWnd, QWL_OPEN))
                    {
                        ulNumMinChildren--;
                        WinSetWindowUShort (hWndFrame, QWS_XMINIMIZE, (USHORT)-1);
                        WinSetWindowUShort (hWndFrame, QWS_YMINIMIZE, (USHORT)-1);
                    }
                    else
                    {
                        /* Don't allow the window to restore itself.
                           Reset the window position */
                        WinQueryWindowPos (hWndFrame, pSwp);
                        mReturn = (MRESULT)TRUE;
                    }
                }
            }
            break;

        case WM_ACTIVATE:
            if (SHORT1FROMMP(mp1))
            {
                hWndActiveChild = hWnd;

                /* Notify toolbar of new active child window */
                WinSendMsg (hWndToolBar, WM_USER_CHILDACTIVATE, 0L, 0L);
            }
            break;

        case WM_CLOSE:
            /* Don't let the WM_CLOSE go to WinDefWindowProc.  The default
               processing for this is to post a WM_QUIT message to the 
               application.  Since this is a child closing we don't want
               that to occur -- just destroy the window. */
            WinDestroyWindow (WinQueryWindow (hWnd, QW_PARENT));
            break;

        case WM_DESTROY:
            if (IsWindowMinimized (hWnd))
                ulNumMinChildren--;

            ulNumChildren--;

            /* If there are no children left then disable the toolbar */
            if (ulNumChildren == 0)
                WinEnableWindow (hWndToolBar, FALSE);

            /* Force a repaint of the title bar window */
            WinInvalidateRect (WinWindowFromID(hWndFrame,FID_TITLEBAR),
                NULL, FALSE);
            break;

        default:
            mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);
            break;
    }

    return (mReturn);
}

MRESULT EXPENTRY ToolBarDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mReturn  = 0L;
    BOOL    bHandled = TRUE;
    HWND    hWndOwner;
    RECTL   Rectl;

    switch (msg)
    {
        case WM_INITDLG:
            WinSendDlgItemMsg (hWnd, IDC_RED, SPBM_SETTEXTLIMIT, 
                (MPARAM)3, (MPARAM)0);
            WinSendDlgItemMsg (hWnd, IDC_RED, SPBM_SETLIMITS,
                (MPARAM)255L, (MPARAM)0L);
            WinSendDlgItemMsg (hWnd, IDC_BLUE, SPBM_SETTEXTLIMIT, 
                (MPARAM)3, (MPARAM)0);
            WinSendDlgItemMsg (hWnd, IDC_BLUE, SPBM_SETLIMITS,
                (MPARAM)255L, (MPARAM)0L);
            WinSendDlgItemMsg (hWnd, IDC_GREEN, SPBM_SETTEXTLIMIT, 
                (MPARAM)3, (MPARAM)0);
            WinSendDlgItemMsg (hWnd, IDC_GREEN, SPBM_SETLIMITS,
                (MPARAM)255L, (MPARAM)0L);
            WinSetFocus (HWND_DESKTOP, WinWindowFromID (hWnd, IDC_WININFO));
            break;

        case WM_USER_POSITION:
            /* Position the toolbar in the lower left corner of the owner */
            hWndOwner = WinQueryWindow (hWnd, QW_OWNER);
            WinQueryWindowRect (hWndOwner, &Rectl);
            WinMapWindowPoints (hWndOwner, HWND_DESKTOP, (PPOINTL)&Rectl, 1L);
            WinSetWindowPos (hWnd, 0, Rectl.xLeft, Rectl.yBottom, 0L, 0L, 
                SWP_MOVE | SWP_SHOW);
            break;

        case WM_USER_CHILDACTIVATE:
            {
                ULONG ulRGB = WinQueryWindowULong (hWndActiveChild, QWL_COLOR_RGB);
                WinCheckButton (hWnd, IDC_MOVE, 
                    (USHORT)WinQueryWindowULong (hWndActiveChild, QWL_MOVE));
                WinCheckButton (hWnd, IDC_TRACK, 
                    (USHORT)WinQueryWindowULong (hWndActiveChild, QWL_TRACKING));
                WinCheckButton (hWnd, IDC_OPEN, 
                    (USHORT)WinQueryWindowULong (hWndActiveChild, QWL_OPEN));
                WinSendDlgItemMsg (hWnd, IDC_RED,   SPBM_SETCURRENTVALUE, 
                   (MPARAM)((ulRGB & 0x00FF0000) >> 16), (MPARAM)0L);
                WinSendDlgItemMsg (hWnd, IDC_GREEN, SPBM_SETCURRENTVALUE, 
                   (MPARAM)((ulRGB & 0x0000FF00) >>  8), (MPARAM)0L);
                WinSendDlgItemMsg (hWnd, IDC_BLUE, SPBM_SETCURRENTVALUE, 
                   (MPARAM)(ulRGB & 0x000000FF), (MPARAM)0L);
            }
            break;

        case WM_CONTROL:
            if (hWndActiveChild)
            {
                BOOL bChecked;
                switch (SHORT1FROMMP(mp1))
                {
                    case IDC_MOVE:
                        if (SHORT2FROMMP(mp1) == BN_CLICKED)
                        {
                           bChecked = WinQueryButtonCheckstate (hWnd, IDC_MOVE);
                           WinSetWindowULong (hWndActiveChild, QWL_MOVE, bChecked);
                           WinSubclassWindow (
                               WinQueryWindow (hWndActiveChild, QW_PARENT),
                               (PFNWP)ChildSubclassProc);
                        }
                        break;

                    case IDC_TRACK:
                        if (SHORT2FROMMP(mp1) == BN_CLICKED)
                        {
                           bChecked = WinQueryButtonCheckstate (hWnd, IDC_TRACK);
                           WinSetWindowULong (hWndActiveChild, QWL_TRACKING, bChecked);
                           WinSubclassWindow (
                               WinQueryWindow (hWndActiveChild, QW_PARENT),
                               (PFNWP)ChildSubclassProc);
                        }
                        break;

                    case IDC_OPEN:
                        if (SHORT2FROMMP(mp1) == BN_CLICKED)
                        {
                            bChecked = WinQueryButtonCheckstate (hWnd, IDC_OPEN);
                            WinSetWindowULong (hWndActiveChild, QWL_OPEN, bChecked);
                        }
                        break;

                    case IDC_RED:
                    case IDC_GREEN:
                    case IDC_BLUE:
                        if (SHORT2FROMMP(mp1) == SPBN_ENDSPIN)
                        {
                            ULONG ulRed, 
                                  ulGreen,
                                  ulBlue;

                            WinSendDlgItemMsg (hWnd, IDC_RED,
                                SPBM_QUERYVALUE, (MPARAM)&ulRed, 0L);
                            WinSendDlgItemMsg (hWnd, IDC_GREEN,
                                SPBM_QUERYVALUE, (MPARAM)&ulGreen, 0L);
                            WinSendDlgItemMsg (hWnd, IDC_BLUE,
                                SPBM_QUERYVALUE, (MPARAM)&ulBlue, 0L);
                            WinSetWindowULong (hWndActiveChild, QWL_COLOR_RGB,
                                (ulRed << 16) | (ulGreen << 8) | ulBlue);

                            /* Force the child to repaint NOW */
                            WinInvalidateRect (hWndActiveChild, NULL, FALSE);
                            WinUpdateWindow (hWndActiveChild);
                        }
                        break;
                }
            }
            break;

		  case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDC_WININFO:
                    if (hWndActiveChild)
                    {
                        WinEnableWindow (hWndFrame, FALSE);
                        WinDlgBox (HWND_DESKTOP, hWnd, WinInfoDlgProc,
                            0L, IDD_WININFO, NULL);
                        WinEnableWindow (hWndFrame, TRUE);
                    }
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

MRESULT EXPENTRY WinInfoDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mReturn  = 0;
    BOOL    bHandled = TRUE;
    CHAR    szTitle[40];
    HWND    hWndActiveFrame;
    POINTL  Ptl;
    SWP     Swp;

    switch (msg)
    {
        case WM_INITDLG:
            CenterWindow (hWnd);
            hWndActiveFrame = WinQueryWindow (hWndActiveChild, QW_PARENT);
            WinQueryWindowText (hWndActiveFrame, sizeof(szTitle), szTitle);
            WinQueryWindowPos (hWndActiveFrame, &Swp);
            Ptl.x = Swp.x;
            Ptl.y = Swp.y;
            WinMapWindowPoints (hWndActiveFrame, hWndFrame, &Ptl, 1L);

            WinSendDlgItemMsg  (hWnd, IDC_TITLE, EM_SETTEXTLIMIT, 
                (MPARAM)sizeof(szTitle), 0L);
            WinSetDlgItemText  (hWnd, IDC_TITLE, szTitle);
            WinSetDlgItemShort (hWnd, IDC_XPOS,   (SHORT)Ptl.x,   TRUE);
            WinSetDlgItemShort (hWnd, IDC_YPOS,   (SHORT)Ptl.y,   TRUE);
            WinSetDlgItemShort (hWnd, IDC_WIDTH,  (USHORT)Swp.cx, FALSE);
            WinSetDlgItemShort (hWnd, IDC_HEIGHT, (USHORT)Swp.cy, FALSE);
            if (Swp.fl & SWP_MINIMIZE)
                WinSetDlgItemText (hWnd, IDC_STATE, "Minimized");
            else if (Swp.fl & SWP_MAXIMIZE)
                WinSetDlgItemText (hWnd, IDC_STATE, "Maximized");
            else
                WinSetDlgItemText (hWnd, IDC_STATE, "Normal");
            WinSetFocus (HWND_DESKTOP, WinWindowFromID (hWnd, IDC_TITLE));
            break;

		  case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case DID_OK:
                    WinQueryDlgItemText (hWnd, IDC_TITLE, sizeof(szTitle),
                        szTitle);
                    WinSetWindowText (
                       WinQueryWindow (hWndActiveChild, QW_PARENT), szTitle);
                    WinDismissDlg (hWnd, DID_OK);
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

/* ------------------ Window Subclass Functions ------------------ */

MRESULT EXPENTRY ChildSubclassProc (HWND hWnd, ULONG msg, MPARAM mp1,
                                    MPARAM mp2)
{
    BOOL    bHandled = FALSE;
    MRESULT mReturn = 0L;

    switch (msg)
    {
        case WM_TRACKFRAME:
            if ( ((SHORT1FROMMP(mp1) & TF_MOVE) == TF_MOVE) &&
                 !WinQueryWindowULong (
                    WinWindowFromID (hWnd, FID_CLIENT), QWL_MOVE))
                bHandled = TRUE;
            break;

        case WM_QUERYTRACKINFO:
            if (WinQueryWindowULong (
                   WinWindowFromID (hWnd, FID_CLIENT), QWL_TRACKING))
            {
                PTRACKINFO pti;
                RECTL      Rectl;

                /* Let the PM frame window function initialize TRACKINFO */
                mReturn = (*pfnFrameProc) (hWnd, msg, mp1, mp2);

                WinQueryWindowRect (hWnd, &Rectl);
                pti                    = (PTRACKINFO)mp2;
                pti->ptlMinTrackSize.x = 75L;
                pti->ptlMinTrackSize.y = 50L;
                pti->ptlMaxTrackSize.x = Rectl.xRight + 40;
                pti->ptlMaxTrackSize.y = Rectl.yTop   + 40;
                bHandled = TRUE;
            }
    }

    if (!bHandled)
        mReturn = (*pfnFrameProc) (hWnd, msg, mp1, mp2);

    return (mReturn);
}

MRESULT EXPENTRY FrameSubclassProc (HWND hWnd, ULONG msg, MPARAM mp1,
                                       MPARAM mp2)
{
    MRESULT  mReturn = 0L;
    HPOINTER hPtr;

    switch (msg)
    {
        case WM_CONTROLPOINTER:
            switch (SHORT1FROMMP(mp1))
            {
                case FID_SYSMENU:
                    mReturn = (MRESULT)hPtrSysMenu;
                    break;
                case FID_MENU:
                    mReturn = (MRESULT)hPtrMenu;
                    break;
                case FID_MINMAX:
                    mReturn = (MRESULT)hPtrMinMax;
                    break;
                default:
                    mReturn = mp2;    /* Use the default pointer */
            }
            break;

        case WM_MOUSEMOVE:
            switch (SHORT1FROMMP(mp2))
            {
                case TF_LEFT:
                case TF_RIGHT:
                   hPtr = hPtrLeft;
                   break;
                case TF_TOP:
                case TF_BOTTOM:
                   hPtr = hPtrTop;
                   break;
                case TF_LEFT | TF_TOP:
                case TF_RIGHT | TF_BOTTOM:
                   hPtr = hPtrLeftTop;
                   break;
                case TF_RIGHT | TF_TOP:
                case TF_LEFT | TF_BOTTOM:
                   hPtr = hPtrRightTop;
                   break;
                default:
                   hPtr = 0;
            }
            if (hPtr)
                WinSetPointer (HWND_DESKTOP, hPtr);
            else
                mReturn = (*pfnFrameProc) (hWnd, msg, mp1, mp2);
            break;

        default:
            mReturn = (*pfnFrameProc) (hWnd, msg, mp1, mp2);
            break;
    }

    return (mReturn);
}

MRESULT EXPENTRY MenuSubclassProc (HWND hWnd, ULONG msg, MPARAM mp1,
                                       MPARAM mp2)
{
    if (msg == WM_CONTROLPOINTER)
        return ((MRESULT)hPtrPopupMenu);
    else
        return (*pfnMenuProc) (hWnd, msg, mp1, mp2);     
}

MRESULT EXPENTRY SysMenuSubclassProc (HWND hWnd, ULONG msg, MPARAM mp1,
                                       MPARAM mp2)
{
    if (msg == WM_CONTROLPOINTER)
        return ((MRESULT)hPtrPopupMenu);
    else
        return (*pfnSysMenuProc) (hWnd, msg, mp1, mp2);
}

MRESULT EXPENTRY TitleBarSubclassProc (HWND hWnd, ULONG msg, MPARAM mp1,
                                       MPARAM mp2)
{
    MRESULT mReturn = 0L;
    ULONG   ulCurState,
            ulNewState;
    HPS     hps;

    switch (msg)
    {
        case WM_MOUSEMOVE:
            WinSetPointer (HWND_DESKTOP, hPtrTitleBar);
            break;

        case TBM_QUERYHILITE:
            mReturn = (MRESULT)(WinQueryWindowULong (hWnd,QWL_USER) & 0x0001);
            break;

        case TBM_SETHILITE:
            mReturn = (MRESULT)TRUE;
            ulCurState = ulNewState = WinQueryWindowULong (hWnd,QWL_USER);
            if (LOUSHORT(mp1))
                ulNewState |= 1;
            else
                ulNewState &= ~1;

            /* Only need to update if the state is changing */
            if (ulCurState == ulNewState)
                break;
            WinSetWindowULong (hWnd,QWL_USER,ulNewState);

            hps = WinGetPS (hWnd);
            PaintTitleBar (hWnd,hps);
            WinReleasePS (hps);
            break;

        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            PaintTitleBar (hWnd,hps);
            WinEndPaint (hps);
            break;

        default:
            mReturn = (*pfnTitleBarProc) (hWnd, msg, mp1, mp2);
            break;
    }

    return (mReturn);
}


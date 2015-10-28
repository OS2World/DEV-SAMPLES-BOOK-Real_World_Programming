/* --------------------------------------------------------------------
                            Menu Program
                              Chapter 5

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdio.h>
#include "menuxtra.h"
#include "menu.h"
#include "..\common\about.h"

MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);

HAB   hab;
CHAR  szTitle[64];
CHAR  szColor[24];
HWND  hWndFrame;            /* Application Frame window handle  */
HWND  hWndClient;           /* Application Client window handle */
HWND  hWndMenu;
HWND  hColorMenu;
HWND  hWndEditMenu;

/* Attributes set by menu */
LONG   lWindowColor   = 0x00FFFFFF;
LONG   lTextColor     = 0x00000000;
LONG   flCmd          = DT_CENTER | DT_VCENTER;
CHAR   szText[40]     = "Menu Text String";
USHORT AlignFlags[6]  = { DT_LEFT, DT_CENTER, DT_RIGHT,
                        DT_TOP,  DT_VCENTER, DT_BOTTOM };
USHORT StyleFlags[3]  = { 0, DT_UNDERSCORE, DT_STRIKEOUT };
USHORT HAlign         = IDM_ALIGNCENTER, 
       VAlign         = IDM_ALIGNMIDDLE,
       Style          = IDM_STYLENORMAL;
                        /*    RED         GREEN       BLUE   */
LONG   lColorTable[3] = { 0x00FF0000, 0x0000FF00, 0x000000FF };
BOOL   bWindowColor   = TRUE;
BOOL   bUndoAvailable = FALSE;
BOOL   bSelected      = FALSE;
 
HBITMAP hbmCheck1,
        hbmUnCheck1,
        hbmCheck2,
        hbmUnCheck2;

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                         FCF_MINMAX | FCF_SHELLPOSITION | FCF_TASKLIST | 
                         FCF_MENU | FCF_ICON;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, ClientWndProc, CS_SIZEREDRAW, 0);
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

VOID AddBitmapMenu (HWND hWndMenu)
{
    MENUITEM mi;

    mi.iPosition   = MIT_END;
    mi.afStyle     = MIS_TEXT | MIS_SUBMENU;
    mi.afAttribute = 0;       
    mi.id          = BITMAP_MENU;
    mi.hwndSubMenu = WinLoadMenu (HWND_OBJECT,0,BITMAP_MENU);
    mi.hItem       = 0;
    WinSendMsg (hWndMenu, MM_INSERTITEM, &mi, "Bitmaps");
    return;
} 
                  
VOID AddEditMenuItem (HWND hWndMenu)
{
    MENUITEM mi;

    hWndEditMenu = WinCreateMenu (HWND_OBJECT, NULL);
    WinSetWindowUShort (hWndEditMenu, QWS_ID, IDM_EDIT);
    mi.iPosition   = MIT_END;
    mi.afStyle     = MIS_TEXT;
    mi.afAttribute = MIA_DISABLED;
    mi.id          = IDM_UNDO;
    mi.hwndSubMenu = 0;
    mi.hItem       = 0;
    WinSendMsg (hWndEditMenu, MM_INSERTITEM, &mi, "~Undo");
    mi.id          = IDM_COPY;
    WinSendMsg (hWndEditMenu, MM_INSERTITEM, &mi, "~Copy");
    mi.id          = IDM_CUT;                         
    WinSendMsg (hWndEditMenu, MM_INSERTITEM, &mi, "Cu~t");
    mi.id          = IDM_PASTE;
    WinSendMsg (hWndEditMenu, MM_INSERTITEM, &mi, "~Paste");
    mi.iPosition   = 1;
    mi.afStyle     = MIS_SEPARATOR;
    mi.afAttribute = 0;          
    mi.id          = -1;
    WinSendMsg (hWndEditMenu, MM_INSERTITEM, &mi, "");
    mi.iPosition   = 0;
    mi.afStyle     = MIS_TEXT | MIS_SUBMENU;
    mi.afAttribute = 0;
    mi.id          = IDM_EDIT;
    mi.hwndSubMenu = hWndEditMenu;
    WinSendMsg (hWndMenu, MM_INSERTITEM, &mi, "~Edit");
    return;
}

VOID DrawOwnerItem (POWNERITEM poi)
{
    ULONG            yOffset;
    RECTL            Rectl;
    BITMAPINFOHEADER bmInfoOwnerDraw;
    ULONG            ropCode;

    /* Get the bitmap handle for the bitmap */
    GpiQueryBitmapParameters ((HBITMAP)poi->hItem,&bmInfoOwnerDraw);

    /* Determine the offset of the portion of the bitmap to draw */
    if (poi->fsAttribute & MIA_CHECKED)
        yOffset = bmInfoOwnerDraw.cx / 2;
    else
        yOffset = 0;
    if (poi->fsAttribute & MIA_HILITED)
        yOffset += bmInfoOwnerDraw.cx / 4;

    /* Determine ROP code to use */
    if (poi->fsAttribute & MIA_DISABLED)
        ropCode = DBM_HALFTONE;
    else
        ropCode = DBM_NORMAL;

    /* Set the source rectangle and draw the bitmap.  */
    Rectl.xLeft   = yOffset;
    Rectl.yBottom = 0L;
    Rectl.xRight  = yOffset + bmInfoOwnerDraw.cx / 4;
    Rectl.yTop    = bmInfoOwnerDraw.cy;
    WinDrawBitmap (poi->hps,(HBITMAP)poi->hItem,&Rectl,(PPOINTL)&poi->rclItem,
        0L, 0L, ropCode);

    /* Turn off attributes by default */
    poi->fsAttribute = 
        (poi->fsAttributeOld &= ~(MIA_CHECKED | MIA_HILITED | MIA_FRAMED));
    return;
}

VOID MeasureOwnerItem (POWNERITEM poi)
{
    BITMAPINFOHEADER bmInfoMenuAttached,
                     bmInfoMenuCheck,
                     bmInfoMenu;
    HBITMAP hbm = WinGetSysBitmap (HWND_DESKTOP,SBMP_MENUATTACHED);
    GpiQueryBitmapParameters (hbm,&bmInfoMenuAttached);
    GpiDeleteBitmap (hbm);
    hbm = WinGetSysBitmap (HWND_DESKTOP,SBMP_MENUCHECK);
    GpiQueryBitmapParameters (hbm,&bmInfoMenuCheck);
    GpiDeleteBitmap (hbm);
    GpiQueryBitmapParameters ((HBITMAP)poi->hItem,&bmInfoMenu);
    poi->rclItem.xRight = (bmInfoMenu.cx / 4) - 
         bmInfoMenuAttached.cx - bmInfoMenuCheck.cx;
    poi->rclItem.yTop   = bmInfoMenu.cy;
    return;
}

VOID PlacePopup (HWND hWndOwner, HWND hWndMenu, 
       ULONG x, ULONG y, BOOL bRight, ULONG ulFlags)
{
    /* We need to get the height of the menu because WinPopupMenu will
       position the menu with the lower left corner of the menu at the
       coordinates passed.  By sending a WM_ADJUSTWINDOWPOS message to 
       the menu it will fill in the swp structure of the menu with its
       size.
    */
    SWP   Swp;
    ULONG ulStyle;

    ulStyle  = WinQueryWindowULong (hWndMenu,QWL_STYLE) & ~MS_ACTIONBAR;
    WinSetWindowULong (hWndMenu,QWL_STYLE,ulStyle);
    WinQueryWindowPos(hWndMenu, (PSWP)&Swp);
    Swp.fl               = SWP_MOVE | SWP_SIZE;
    Swp.hwndInsertBehind = HWND_TOP;

    /* Set owner to frame here so that WM_MEASUREITEM messages are
       sent to the frame window for processing -- in case menu is
       ownerdraw */
    WinSetOwner  (hWndMenu,hWndOwner);
    WinSendMsg(hWndMenu, WM_ADJUSTWINDOWPOS, (MPARAM)(PSWP)&Swp, NULL);

    WinPopupMenu (hWndOwner,hWndOwner,hWndMenu,
        bRight ? x : x - Swp.cx, y - Swp.cy, 0,
        PU_HCONSTRAIN | PU_VCONSTRAIN | PU_KEYBOARD |
        PU_MOUSEBUTTON1 | ulFlags);

    return;
}

VOID SetMenuCheckMarks (HPS hps, HWND hWndMenu)
{
    hbmCheck1   = GpiLoadBitmap (hps,0,IDBMP_CHECK1,0L,0L);
    hbmUnCheck1 = GpiLoadBitmap (hps,0,IDBMP_UNCHECK1,0L,0L);
    hbmCheck2   = GpiLoadBitmap (hps,0,IDBMP_CHECK2,0L,0L);
    hbmUnCheck2 = GpiLoadBitmap (hps,0,IDBMP_UNCHECK2,0L,0L);
 
    /* Set the checkmarks for the horizontal alignment */
    WinSetCheckMark (hWndMenu,IDM_ALIGNLEFT,hbmCheck1,TRUE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNLEFT,hbmUnCheck1,FALSE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNCENTER,hbmCheck1,TRUE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNCENTER,hbmUnCheck1,FALSE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNRIGHT,hbmCheck1,TRUE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNRIGHT,hbmUnCheck1,FALSE);

    /* Set the checkmarks for the vertical alignment */
    WinSetCheckMark (hWndMenu,IDM_ALIGNTOP,hbmCheck2,TRUE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNTOP,hbmUnCheck2,FALSE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNMIDDLE,hbmCheck2,TRUE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNMIDDLE,hbmUnCheck2,FALSE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNBOTTOM,hbmCheck2,TRUE);
    WinSetCheckMark (hWndMenu,IDM_ALIGNBOTTOM,hbmUnCheck2,FALSE);
    return;
}

VOID SetOwnerDrawBitmapHandles (HPS hps, HWND hWndMenu)
{
    MENUITEM mi;
    USHORT   usCnt;
    
    usCnt = LOUSHORT(WinSendMsg (hWndMenu, MM_QUERYITEMCOUNT, 0L, 0L));

    while (usCnt)
    {
        WinSendMsg (hWndMenu, MM_QUERYITEMBYPOS, 
            MPFROM2SHORT (usCnt,0), MAKE_16BIT_POINTER (&mi));
        /* The menu item id is the same as the bitmap id */
        mi.hItem = (ULONG)GpiLoadBitmap (hps,0,mi.id,0L,0L);
        WinSendMsg (hWndMenu, MM_SETITEMHANDLEBYPOS,
            MPFROM2SHORT (usCnt,0), (MPARAM)mi.hItem);
        usCnt--;
    }
    return;
}

MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    HPS     hPS;

    switch (msg)
    {
        case WM_CREATE:
            /* Retrieve the handle of the FID_MENU window */
            hWndMenu   = WinWindowFromID (WinQueryWindow(hWnd,QW_PARENT),FID_MENU);

            /* Load the color popup menu */
            hColorMenu = WinLoadMenu (HWND_OBJECT,0,COLOR_MENU);

            hPS = WinGetPS (hWnd);

            /* Load and set the checkmark bitmaps */
            SetMenuCheckMarks (hPS, hWndMenu);

            /* Load and set the owner draw bitmap handles */
            SetOwnerDrawBitmapHandles (hPS,hColorMenu);

            WinReleasePS (hPS);

            /* Add the Edit menu item to the main menu */
            AddEditMenuItem (hWndMenu);

            /* Add the Bitmap menu item to the main menu */
            AddBitmapMenu (hWndMenu);

            /* Add the About menu item to the system menu */
            AddAboutToSystemMenu (WinQueryWindow (hWnd, QW_PARENT));
            break;

        case WM_DESTROY:
            /* Delete the bitmaps for the menu items */
            GpiDeleteBitmap (hbmCheck1);
            GpiDeleteBitmap (hbmUnCheck1);
            GpiDeleteBitmap (hbmCheck2);
            GpiDeleteBitmap (hbmUnCheck2);
            WinDestroyWindow (hColorMenu);
            break;

        case WM_BUTTON1DOWN:
            bWindowColor = TRUE;
            WinLoadString (hab, 0, IDS_WINDOWCOLOR, sizeof(szColor), szColor);
            WinSendMsg (hColorMenu,MM_SETITEMTEXTBYPOS,
                MPFROMSHORT(0),MAKE_16BIT_POINTER(szColor));
            /* Call WinDefWindowProc so that window is activated */
            mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);
            PlacePopup (hWnd, hColorMenu, 
                SHORT1FROMMP(mp1),SHORT2FROMMP(mp1),FALSE,
                PU_NONE);  /* Allow button to be released */
            break;

        case WM_BUTTON2DOWN:
            bWindowColor = FALSE;
            WinLoadString (hab, 0, IDS_TEXTCOLOR, sizeof(szColor), szColor);
            WinSendMsg (hColorMenu,MM_SETITEMTEXTBYPOS,
                MPFROMSHORT(0),MAKE_16BIT_POINTER(szColor));
            /* Call WinDefWindowProc so that window is activated */
            mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);
            PlacePopup (hWnd, hColorMenu, 
                SHORT1FROMMP(mp1),SHORT2FROMMP(mp1),TRUE,
                PU_MOUSEBUTTON2DOWN);
            break;

		  case WM_COMMAND:
		  case WM_SYSCOMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDM_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;

                case IDM_STYLENORMAL:
                case IDM_STYLEUNDERSCORE:
                case IDM_STYLESTRIKEOUT:
                    WinCheckMenuItem (hWndMenu, Style, FALSE);
                    Style = LOUSHORT(mp1);
                    WinCheckMenuItem (hWndMenu, LOUSHORT(mp1), TRUE);
                    flCmd &= ~(DT_UNDERSCORE | DT_STRIKEOUT);
                    flCmd |= StyleFlags[LOUSHORT(mp1) - IDM_STYLENORMAL];
                    WinInvalidateRect (hWnd,NULL,FALSE);
                    break;

                case IDM_ALIGNLEFT:
                case IDM_ALIGNCENTER:
                case IDM_ALIGNRIGHT:
                    WinCheckMenuItem (hWndMenu, HAlign, FALSE);
                    HAlign = LOUSHORT(mp1);
                    WinCheckMenuItem (hWndMenu, LOUSHORT(mp1), TRUE);
                    flCmd &= ~(DT_LEFT | DT_CENTER | DT_RIGHT);
                    flCmd |= AlignFlags[LOUSHORT(mp1) - IDM_ALIGNLEFT];
                    WinInvalidateRect (hWnd,NULL,FALSE);
                    break;

                case IDM_ALIGNTOP:
                case IDM_ALIGNMIDDLE:
                case IDM_ALIGNBOTTOM:
                    WinCheckMenuItem (hWndMenu, VAlign, FALSE);
                    VAlign = LOUSHORT(mp1);
                    WinCheckMenuItem (hWndMenu, LOUSHORT(mp1), TRUE);
                    flCmd &= ~(DT_TOP | DT_VCENTER | DT_BOTTOM);
                    flCmd |= AlignFlags[LOUSHORT(mp1) - IDM_ALIGNLEFT];
                    WinInvalidateRect (hWnd,NULL,FALSE);
                    break;

                case IDM_BMPITEM1:
                case IDM_BMPITEM2:
                {
                    USHORT usChecked;

                    /* Toggle the checkmark flag */
                    usChecked = 
                        LOUSHORT(WinIsMenuItemChecked (hWndMenu, LOUSHORT(mp1)));
                    usChecked ^= MIA_CHECKED;
                    WinCheckMenuItem (hWndMenu, LOUSHORT(mp1), usChecked);
                    break;
                } 
                                 
                case IDBMP_ODITEM1:
                case IDBMP_ODITEM2:
                case IDBMP_ODITEM3:
                {
                    USHORT usChecked;

                    /* Toggle the checkmark flag */
                    usChecked = 
                        LOUSHORT(WinIsMenuItemChecked (hColorMenu, LOUSHORT(mp1)));
                    usChecked ^= MIA_CHECKED;
                    WinCheckMenuItem (hColorMenu, LOUSHORT(mp1), usChecked);

                    /* Toggle the color field */
                    if (bWindowColor)
                        lWindowColor ^= lColorTable[LOUSHORT(mp1)-IDBMP_ODITEM1];
                    else
                        lTextColor   ^= lColorTable[LOUSHORT(mp1)-IDBMP_ODITEM1];
                    WinInvalidateRect (hWnd,NULL,FALSE);
                    break;
                }                 

				    case SC_CLOSE:
					     WinPostMsg (hWnd,WM_QUIT,0L,0L);
						  break;

                default:
                    bHandled = (msg == WM_COMMAND);
            }
            break;

        case WM_MEASUREITEM:
            MeasureOwnerItem ((POWNERITEM)mp2);
            break;

        case WM_DRAWITEM:
            DrawOwnerItem ((POWNERITEM)mp2);
            mReturn = (MRESULT)TRUE;
            break;

        case WM_INITMENU:
            if (SHORT1FROMMP(mp1) == IDM_EDIT)
            {
                ULONG ulFormat;
                WinEnableMenuItem ((HWND)mp2, IDM_UNDO, bUndoAvailable);
                WinEnableMenuItem ((HWND)mp2, IDM_COPY, bSelected);
                WinEnableMenuItem ((HWND)mp2, IDM_CUT,  bSelected);
                WinEnableMenuItem ((HWND)mp2, IDM_PASTE, 
                WinQueryClipbrdFmtInfo (hab, CF_TEXT, &ulFormat));
            }
            else if ((HWND)mp2 == hColorMenu)
            {
                USHORT Pos;
                for (Pos = 0; Pos < 3; Pos++)
                    WinSendMsg (hColorMenu, MM_SETITEMATTRBYPOS,
                        MPFROM2SHORT(Pos+1,0),
                        MPFROM2SHORT(MIA_CHECKED, 
                          bWindowColor ? 
                          ((lWindowColor & lColorTable[Pos]) ? MIA_CHECKED : 0) :
                          ((lTextColor & lColorTable[Pos]) ? MIA_CHECKED : 0)));
            }
            break;

        case WM_PAINT:
            hPS = WinBeginPaint (hWnd,0,0);
            {
                RECTL Rectl;
                GpiCreateLogColorTable (hPS,LCOL_RESET,LCOLF_RGB,0L,0L,NULL);
                WinQueryWindowRect (hWnd, &Rectl);
                WinFillRect (hPS, &Rectl, lWindowColor);
                WinDrawText (hPS,-1L, szText, &Rectl,
                    lTextColor, lWindowColor, flCmd);
            }
            WinEndPaint (hPS);
            break;

        case WM_ERASEBACKGROUND:
            mReturn = (MRESULT)1;
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}



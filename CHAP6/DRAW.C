/* --------------------------------------------------------------------
                         GPI Drawing Program
                              Chapter 6

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI
#define INCL_GPIPOLYGON
#include <os2.h>
#include "draw.h"
#include "thermo.h"
#include "..\common\about.h"

extern LONG APIENTRY GpiPolygons( HPS, ULONG, PPOLYGON, ULONG, ULONG );

#ifdef  POLYGON_EXCL
#undef  POLYGON_EXCL
#endif
#define POLYGON_EXCL 1

MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);

HAB     hab;
HWND    hWndFrame, 
        hWndClient;
HWND    hMenu;
CHAR    szTitle[64];
CHAR    szFontFace[]    = "8.Helv";

USHORT  usMode = IDM_THERM;
USHORT  usLineJoin = IDM_BEVEL;
USHORT  usLineEnd  = IDM_FLAT_END;
PVOID   pMem;
RECTL   ThermRectl;
int     iPercent = 0;
BOOL    bTimer = FALSE;
BOOL    bRising = TRUE;
POINTL  ptlStar [5][5];
POINTL  ptlSpline [7];
USHORT  usJoinStyles [] =
{LINEJOIN_BEVEL, LINEJOIN_ROUND, LINEJOIN_MITRE};
USHORT  usEndStyles [] =
{LINEEND_FLAT, LINEEND_ROUND, LINEEND_SQUARE};

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                            FCF_ICON | FCF_MENU;
    CHAR  szClientClass[] = "CLIENT";
    int   iClientHeight;

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, ClientWndProc, CS_SIZEREDRAW, 0);
    WinLoadString (hab, 0,IDS_APPNAME,sizeof(szTitle),szTitle);

    DosAllocMem((PPVOID)&pMem,0x1000,PAG_READ | PAG_WRITE);
    DosSubSet(pMem,DOSSUB_INIT | DOSSUB_SPARSE_OBJ,8192);

    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
        &flFrameFlags, szClientClass, szTitle, 0, 0, ID_APPNAME, &hWndClient);

    iClientHeight = WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) -
                    WinQuerySysValue(HWND_DESKTOP,SV_CYICON);
    WinSetWindowPos (hWndFrame,HWND_TOP,0,
        WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) - iClientHeight,
        WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN),iClientHeight,
        SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER | SWP_SIZE | SWP_MOVE);

    hMenu  = WinWindowFromID (hWndFrame,FID_MENU);


    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    DosFreeMem(pMem);
    return (0);
}


void APIENTRY calculate_points(MPARAM lNewSize)
/*-----------------------------------------------------------------*\

   This function will update the point arrays for the stars and 
   splines when the window is resized.

\*-----------------------------------------------------------------*/
{
    int        i,j;
    int        X,Y;
    int        iWidth,iHeight;
    POINTL     ptl[5];

    /* Calculate the location of the 4 stars */
    iWidth  = SHORT1FROMMP(lNewSize);
    iHeight = SHORT2FROMMP(lNewSize);
    X = (iWidth / 3) - 1;
    Y = (iHeight / 2) - 1;
    ptl[0].x = -(X / 3);
    ptl[0].y = -(Y / 2);
    ptl[1].x = X / 2;
    ptl[1].y = Y / 6;
    ptl[2].x = -(X / 2);
    ptl[2].y = Y / 6;
    ptl[3].x = -ptl[0].x;
    ptl[3].y = ptl[0].y;
    ptl[4].x = 0;
    ptl[4].y = Y / 2;

    for (i = 0;i <= 4;i++)
    {
        if ((i == 0) || (i == 2))
            X = iWidth / 4;
        else
            X = iWidth - (iWidth / 4);
        if ((i == 2) || (i == 3))
            Y = iHeight / 4;
        else
            Y = iHeight - (iHeight / 4);

        for (j = 0;j <= 4;j++)
        {
            ptlStar[i][j].x = X + ptl[j].x;
            ptlStar[i][j].y = Y + ptl[j].y;
        }
    }

    /*  Calculate the points for the spline */
    ptlSpline[0].x = iWidth / 4;
    ptlSpline[0].y = iHeight / 2;
    ptlSpline[1].x = iWidth * 3 / 8;
    ptlSpline[1].y = iHeight * 3 / 4;
    ptlSpline[2].x = iWidth - ptlSpline[1].x;
    ptlSpline[2].y = ptlSpline[1].y;
    ptlSpline[3].x = iWidth - ptlSpline[0].x;
    ptlSpline[3].y = ptlSpline[0].y;
    ptlSpline[4].x = ptlSpline[2].x;
    ptlSpline[4].y = iHeight / 4;
    ptlSpline[5].x = ptlSpline[1].x;
    ptlSpline[5].y = ptlSpline[4].y;
    ptlSpline[6].x = ptlSpline[0].x;
    ptlSpline[6].y = ptlSpline[0].y;
}


void APIENTRY process_command(HWND hWnd,MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   All WM_COMMAND messages received by the client window will be
   processed here.  

\*-----------------------------------------------------------------*/
{
    char szTemp [MAX_STRING];

    switch(SHORT1FROMMP(mp1))
    {
        case IDM_THERM:
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMode,TRUE),
                MPFROM2SHORT(MIA_CHECKED,FALSE));
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_THERM,TRUE),
                MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
            usMode = SHORT1FROMMP(mp1);
            WinInvalidateRect(hWndClient,0,FALSE);
            break;
        case IDM_TIMER:
            if (usMode != IDM_THERM)
                break;
            if (bTimer)
            {
                WinStopTimer(hab,hWnd,1);
                bTimer = FALSE;
                WinLoadString (hab,0,IDS_TIMERON,MAX_STRING,(PSZ)szTemp);
                WinSendMsg (hMenu,MM_SETITEMTEXT,MPFROMSHORT(IDM_TIMER),
                    (MPARAM)szTemp);
            }
            else
            {
                WinStartTimer (hab,hWnd,1,0);
                bTimer = TRUE;
                WinLoadString (hab,0,IDS_TIMEROFF,MAX_STRING,(PSZ)szTemp);
                WinSendMsg (hMenu,MM_SETITEMTEXT,MPFROMSHORT(IDM_TIMER),
                    (MPARAM)szTemp);
            }
            break;
        case IDM_AREA_STARS:                                                 
        case IDM_POLY_STARS:                                                 
        case IDM_FILL_PATH:
        case IDM_OUTLINE_PATH:
        case IDM_CONVERT_ALT_PATH:
        case IDM_CONVERT_WIND_PATH:
        case IDM_STROKE_PATH:
        case IDM_CLIP_PATH:
        case IDM_CLIP_STROKE_PATH:
            if (bTimer)
                WinSendMsg(hWnd,WM_COMMAND,MPFROMSHORT(IDM_TIMER),0);
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMode,TRUE),
                MPFROM2SHORT(MIA_CHECKED,FALSE));
            usMode = SHORT1FROMMP(mp1);
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMode,TRUE),
                MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
            WinInvalidateRect(hWndClient,0,FALSE);
            break;
        case IDM_BEVEL:
        case IDM_ROUND:
        case IDM_MITRE:
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usLineJoin,TRUE),
                MPFROM2SHORT(MIA_CHECKED,FALSE));
            usLineJoin = SHORT1FROMMP(mp1);
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usLineJoin,TRUE),
                MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
            WinInvalidateRect(hWndClient,0,FALSE);
            break;
        case IDM_FLAT_END:
        case IDM_ROUND_END:
        case IDM_SQUARE_END:
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usLineEnd,TRUE),
                MPFROM2SHORT(MIA_CHECKED,FALSE));
            usLineEnd = SHORT1FROMMP(mp1);
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usLineEnd,TRUE),
                MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
            WinInvalidateRect(hWndClient,0,FALSE);
            break;
        case IDM_ABOUT:
            DisplayAbout (hWnd, szTitle);
            break;
    }
    mp2;
}


void APIENTRY process_paint(HWND hWnd)
/*-----------------------------------------------------------------*\

   This function will paint the client area based on the current 
   display mode. The following modes are supported:

   IDM_THERM        Draw a thermometer with the current fill percentage
   IDM_AREA_STARS   Draw 4 stars with all combinations of fill and outline
                    using areas.
   IDM_POLY_STARS   Draw 4 stars with all combinations of fill and outline
                    using the polygon primitive.
   IDM_FILL_PATH    Fill a football shaped path created with 2 spines.
   IDM_OUTLINE_PATH Outline a football shaped path created with 2 spines.
   IDM_CONVERT_ALT_PATH  Modifies a path and then fills it with alternate fill mode
   IDM_CONVERT_WIND_PATH Modifies a path and then fills it with winding fill mode
   IDM_STROKE_PATH  Draw a football shaped wide line using 2 spines.
   IDM_CLIP_STROKE_PATH Converts a path and then select it as the clipping
                    region.
   IDM_CLIP_PATH    Select the path created with 2 splines as a clipping 
                    region and fill the client area with bitmaps. 


\*-----------------------------------------------------------------*/
{
    HPS        hPS;
    BOOL       bAreas = FALSE;
    RECTL      Rectl;
    POLYGON    poly;
    LINEBUNDLE lb;
    AREABUNDLE ab;
    HBITMAP    hBitmap;
    PSZ        pLabel;
    int        iRows, iColumns;
    int        i,j;
    BITMAPINFOHEADER bi;
    POINTL     ptl;


    WinSetPresParam(hWnd,PP_FONTNAMESIZE,strlen(szFontFace)+1,(PVOID)szFontFace);
    hPS = WinBeginPaint(hWnd,0,&Rectl);
    WinFillRect(hPS,(PRECTL)&Rectl,CLR_PALEGRAY);
    ab.lColor = CLR_BLUE;

    switch (usMode)
    {
        case IDM_THERM:
            DrawThermometer(hPS,(PRECTL)&ThermRectl,0,CLR_RED,CLR_PALEGRAY,iPercent);
            break;
        case IDM_AREA_STARS:
            bAreas    = TRUE;
            ab.lColor = CLR_RED;
        case IDM_POLY_STARS:
            DosSubAlloc(pMem,(PPVOID)&pLabel,MAX_STRING);

            GpiSetAttrs (hPS,PRIM_AREA,ABB_COLOR,0,&ab);
            poly.ulPoints = 5;

            GpiSetCurrentPosition (hPS,&ptlStar[0][0]);
            if (bAreas)
            {
                GpiBeginArea (hPS,BA_NOBOUNDARY | BA_WINDING);
                GpiPolyLine (hPS,5L,&ptlStar[0][0]);
                GpiEndArea (hPS);
            }
            else
            {
               poly.aPointl = &ptlStar[0][0];
               GpiPolygons (hPS,1L,&poly,BA_NOBOUNDARY | BA_WINDING,POLYGON_EXCL);
            }
            i = WinLoadString (hab,0,IDS_NOB_WIND,MAX_STRING,pLabel);
            GpiCharStringAt (hPS,&ptlStar[0][2],i,pLabel);

            GpiSetCurrentPosition (hPS,&ptlStar[1][0]);
            if (bAreas)
            {
                GpiBeginArea (hPS,BA_BOUNDARY | BA_WINDING);
                GpiPolyLine (hPS,5L,&ptlStar[1][0]);
                GpiEndArea (hPS);
            }
            else
            {
                poly.aPointl = &ptlStar[1][0];
                GpiPolygons (hPS,1L,&poly,BA_BOUNDARY | BA_WINDING,POLYGON_EXCL);
            }
            i = WinLoadString (hab,0,IDS_BOUND_WIND,MAX_STRING,pLabel);
            GpiCharStringAt (hPS,&ptlStar[1][2],i,pLabel);

            GpiSetCurrentPosition (hPS,&ptlStar[2][0]);
            if (bAreas)
            {
                GpiBeginArea (hPS,BA_NOBOUNDARY | BA_ALTERNATE);
                GpiPolyLine (hPS,5L,&ptlStar[2][0]);
                GpiEndArea (hPS);
            }
            else
            {
                poly.aPointl = &ptlStar[2][0];
                GpiPolygons (hPS,1L,&poly,BA_NOBOUNDARY | BA_ALTERNATE,POLYGON_EXCL);
            }
            i = WinLoadString (hab,0,IDS_NOB_ALT,MAX_STRING,pLabel);
            GpiCharStringAt (hPS,&ptlStar[2][2],i,pLabel);

            GpiSetCurrentPosition (hPS,&ptlStar[3][0]);
            if (bAreas)
            {
                GpiBeginArea (hPS,BA_BOUNDARY | BA_ALTERNATE);
                GpiPolyLine (hPS,5L,&ptlStar[3][0]);
                GpiEndArea (hPS);
            }
            else
            {
                poly.aPointl = &ptlStar[3][0];
                GpiPolygons (hPS,1L,&poly,BA_BOUNDARY | BA_ALTERNATE,POLYGON_EXCL);
            }
            i = WinLoadString (hab,0,IDS_BOUND_ALT,MAX_STRING,pLabel);
            GpiCharStringAt (hPS,&ptlStar[3][2],i,pLabel);
            DosSubFree(pMem,pLabel,MAX_STRING);
            break;

        case IDM_FILL_PATH:
        case IDM_OUTLINE_PATH:
            ab.usSymbol      = PATSYM_HORIZ;
            ab.lColor        = CLR_WHITE;
            ab.lBackColor    = CLR_BLUE;
            ab.usBackMixMode = BM_OVERPAINT;
            GpiSetAttrs (hPS,PRIM_AREA,ABB_COLOR | ABB_SYMBOL | ABB_BACK_COLOR |
                ABB_BACK_MIX_MODE,0,&ab);
            GpiBeginPath (hPS,1);
            GpiMove (hPS,ptlSpline);

            GpiPolySpline(hPS,3,&ptlSpline[1]);
            GpiPolySpline(hPS,3,&ptlSpline[4]);
            GpiEndPath (hPS);

            if (usMode == IDM_FILL_PATH)
                GpiFillPath(hPS,1,FPATH_ALTERNATE);
            else
            {
                lb.lColor = CLR_RED;
                GpiSetAttrs(hPS,PRIM_LINE,ABB_COLOR,0,&lb);
                GpiOutlinePath(hPS,1,0);
            }
            break;

        case IDM_CONVERT_ALT_PATH:
        case IDM_CONVERT_WIND_PATH:
        case IDM_STROKE_PATH:
            ab.usSymbol   = PATSYM_VERT;
            ab.lColor     = CLR_BLUE;
            GpiSetAttrs (hPS,PRIM_AREA,ABB_COLOR | ABB_SYMBOL,0,&ab);
            lb.lGeomWidth = 20;
            lb.usJoin = usJoinStyles[usLineJoin - IDM_BEVEL];
            lb.usEnd  = usEndStyles [usLineEnd - IDM_FLAT_END];
            GpiSetAttrs (hPS,PRIM_LINE,LBB_GEOM_WIDTH | LBB_JOIN | LBB_END,
                0,&lb);
            GpiBeginPath (hPS,1);
            GpiMove (hPS,ptlSpline);
            GpiPolySpline(hPS,3,&ptlSpline[1]);
            GpiPolySpline(hPS,3,&ptlSpline[4]);
            GpiEndPath (hPS);

            if (usMode == IDM_STROKE_PATH)
                GpiStrokePath(hPS,1,FPATH_ALTERNATE);
            else
            {
                GpiModifyPath (hPS,1,MPATH_STROKE);
                if (usMode == IDM_CONVERT_ALT_PATH)
                    GpiFillPath(hPS,1,FPATH_ALTERNATE);
                else
                    GpiFillPath(hPS,1,FPATH_WINDING);
            }
            break;

        case IDM_CLIP_STROKE_PATH:
        case IDM_CLIP_PATH:
            GpiBeginPath (hPS,1);
            GpiMove (hPS,ptlSpline);
            GpiPolySpline(hPS,3,&ptlSpline[1]);
            GpiPolySpline(hPS,3,&ptlSpline[4]);
            GpiEndPath (hPS);

            if (usMode == IDM_CLIP_STROKE_PATH)
            {
               lb.lGeomWidth = 20;
               lb.usJoin = usJoinStyles[usLineJoin - IDM_BEVEL];
               lb.usEnd  = usEndStyles [usLineEnd - IDM_FLAT_END];
               GpiSetAttrs (hPS,PRIM_LINE,LBB_GEOM_WIDTH | LBB_JOIN | LBB_END,
                   0,&lb);
            GpiModifyPath (hPS,1,MPATH_STROKE);
            }
            GpiSetClipPath(hPS,1,FPATH_ALTERNATE | SCP_AND);
            hBitmap = GpiLoadBitmap(hPS,0,ID_STAR,0,0);

            GpiQueryBitmapParameters(hBitmap,(PBITMAPINFOHEADER)&bi);
            iRows = Rectl.xRight / bi.cx; 
            iColumns = Rectl.yTop / bi.cy;
            ptl.x = 0;
            ptl.y = 0;
            for (i = 0;i <= iRows;i++)
            {
                for (j = 0;j <= iColumns; j++)
                {
                   WinDrawBitmap(hPS,hBitmap,0,&ptl,0,0,DBM_NORMAL);
                   ptl.y += bi.cy;
                }
                ptl.y = 0;
                ptl.x += bi.cx;
            }
            break;
    }
    WinEndPaint (hPS);
}


void APIENTRY update_thermometer (HWND hWnd)
/*-----------------------------------------------------------------*\

   This function will process the timer message for the thermometer
   causing it to redraw cycling through the fill percentages 0 to 100.

\*-----------------------------------------------------------------*/
{
    HPS     hPS;

    hPS = WinGetPS(hWnd);
    DrawTemperature(hPS,(PRECTL)&ThermRectl,CLR_RED,CLR_PALEGRAY,iPercent);
    WinReleasePS (hPS);
    if (bRising)
    {
        iPercent++;
        if (iPercent > 100)
        {
            bRising = FALSE;
            iPercent--;
        }
    }
    else
    {
        iPercent--;
        if (iPercent < 0)
        {
            bRising = TRUE;
            iPercent++;
        }
    }
}



MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

   This is the application callback which handles the messages 
   necessary to maintain the client window.

\*-----------------------------------------------------------------*/
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;

    switch (msg)
    {
        case WM_CREATE:
            ThermRectl.xLeft   = 50;
            ThermRectl.yBottom = 50;
            ThermRectl.yTop    = 350;
            ThermRectl.xRight  = 100;
            break;

        case WM_COMMAND:
            process_command(hWnd,mp1,mp2);
            break;

        case WM_TIMER:
            update_thermometer(hWnd);
            break;

        case WM_PAINT:
            process_paint(hWnd);

            break;

        case WM_ERASEBACKGROUND:
            WinFillRect((HPS)LONGFROMMP(mp1),PVOIDFROMMP(mp2),CLR_PALEGRAY);
            mReturn = MRFROMLONG(0L);
            break;

        case WM_SIZE:
            calculate_points(mp2);
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}



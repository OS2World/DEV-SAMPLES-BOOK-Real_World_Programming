/* --------------------------------------------------------------------
                             Controls DLL
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* Bevel Control
     Class Name  - BEVELCLASS
     Styles
        BVS_BEVELIN     - Draw bevel inwards
        BVS_BEVELOUT    - Draw bevel outwards
        BVS_RECTANGLE   - Draw a beveled rectangle
        BVS_LINE        - Draw a beveled trough or fence
        BVS_FILL        - Fill interior
        BVS_PREVID      - Size bevel to encompass all previous sibling
                          windows until window with specified ID is reached.
                          The windows ID is specified as the ID of the
                          window to stop at or'd with BVS_IDMASK
                          x Position is horizontal gap between bevel and windows
                          y Position is vertical gap between bevel and windows
                          cx is border width (0 is nominal width border)
        BVS_PREVWINDOW  - Size bevel to encompass the previous sibling window
                          x Position is horizontal gap between bevel and windows
                          y Position is vertical gap between bevel and windows
                          cx is border width (0 is nominal width border)
     Messages
        BVM_SETCOLOR     - mp1 MAKELONG(bordercolorindex, interiorcolorindex)
                           mp2 0L
        BVM_SETTHICKNESS - mp1 MPFROMSHORT (bevelthickness)
        BVM_RESIZE       - mp1 MAKELONG(horizontalgap, verticalgap)

   Thermometer control
     Class Name  - THERMOMETER CLASS
     Messages
        THM_SETRANGE    - mp1 Low value of range
                          mp2 High value of range
        THM_SETVALUE    - mp1 Current value
        THM_SETCOLOR    - mp1 MAKELONG(fillcolorindex, backgroundcolorindex)
*/

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_GPIPRIMITIVES

#include <os2.h>
#include "controls.h"

/* Window functions */

MRESULT EXPENTRY BevelWndProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY ThermometerWndProc (HWND,ULONG,MPARAM,MPARAM);

/* Undocumented WinDrawBorder flags */

#define DB_RAISED       0x0400
#define DB_DEPRESSED    0x0800
#define DB_TROUGH       0x1000
#define DB_FENCE        0x2000
#define DB_FIELD        0x4000
#define DB_CORNERBORDER 0x8000

/* Shared data placed into initialized variables data segment */

HMODULE hModDll = 0;

/* Bevel window extra bytes */

#define QWL_COLORS          0
#define QWS_THICKNESS       QWL_COLORS    + sizeof(ULONG)
#define BEVELEXTRA          QWS_THICKNESS + sizeof(USHORT)

/* Thermometer window extra bytes */

#define QWL_LOWERRANGE      0
#define QWL_UPPERRANGE      QWL_LOWERRANGE + sizeof(ULONG)
#define QWL_VALUE           QWL_UPPERRANGE + sizeof(ULONG)
#define QWL_COLOR           QWL_VALUE      + sizeof(ULONG)
#define THERMOMETEREXTRA    QWL_COLOR      + sizeof(ULONG)


#ifdef __IBMC__
ULONG _System _DLL_InitTerm (HMODULE hModule, ULONG ulTerminating)
#else
ULONG _stdcall InitializeControlDll (HMODULE hModule, ULONG ulTerminating)
#endif
{
    BOOL bResult;

    if (ulTerminating & 0x01)
        return (FALSE);
    else
    {
        /* Register the window control classes */
        bResult = 
            WinRegisterClass (
                (HAB)0, BEVELCLASS, BevelWndProc, CS_HITTEST | CS_SIZEREDRAW, BEVELEXTRA) &&
            WinRegisterClass (
                (HAB)0, THERMOMETERCLASS, ThermometerWndProc, CS_SIZEREDRAW, THERMOMETEREXTRA);
    }
 
    /* Save the module handle when we get loaded.  The module handle
       will be the same for all instances of the DLL */

    hModDll = hModule;

    return ((ULONG)bResult);
} 


VOID EXPENTRY CenterWindow (HWND hWnd)
{
    ULONG ulScrWidth, ulScrHeight;
    RECTL Rectl;

    ulScrWidth  = WinQuerySysValue (HWND_DESKTOP, SV_CXSCREEN);
    ulScrHeight = WinQuerySysValue (HWND_DESKTOP, SV_CYSCREEN);
    WinQueryWindowRect (hWnd, &Rectl);
    WinSetWindowPos (hWnd, HWND_TOP, (ulScrWidth-Rectl.xRight)/2,
        (ulScrHeight-Rectl.yTop)/2, 0, 0, SWP_MOVE | SWP_ACTIVATE);
    return;
}

VOID QueryBorderRect (HWND hWnd, ULONG ulStyle, PRECTL pRectl)
{
    HWND   hWndPrev;
    SWP    Swp;
    USHORT usID;
    RECTL  RectlPrev;

    /* If style is BVS_PREVID then size the bevel to encompass
       all previous sibling windows until a window with the
       specifed ID is located.

       If style is BVS_PREVWINDOW then size the bevel to encompass
       the previous sibling window. */

    WinSetRect ((HAB)0L, pRectl, 0L, 0L, 0L, 0L);

    if (ulStyle & BVS_PREVWINDOW)
    {
        if ((hWndPrev = WinQueryWindow (hWnd, QW_PREV)) != 0)
        {
            WinQueryWindowPos (hWndPrev, &Swp);
            WinSetRect ((HAB)0L, pRectl, Swp.x, Swp.y, 
                Swp.x+Swp.cx, Swp.y+Swp.cy);
        }
    }
    else
    {
        hWndPrev = hWnd;
        usID     = WinQueryWindowUShort (hWnd, QWS_ID);
        usID    &= ~BVS_IDMASK;

        /* Union the bounding rectangle of all sibling windows
           until window with ID usID is reached */

        while ((hWndPrev = WinQueryWindow (hWndPrev, QW_PREV)) != 0)
        {
            WinQueryWindowPos (hWndPrev, &Swp);
            WinSetRect ((HAB)0L, &RectlPrev, Swp.x, Swp.y, Swp.x+Swp.cx, Swp.y+Swp.cy);
            WinUnionRect ((HAB)0L, pRectl, pRectl, &RectlPrev);
            if (WinQueryWindowUShort (hWndPrev, QWS_ID) == usID)
                break;
        }
    }
    return;
}

MRESULT EXPENTRY BevelWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    RECTL   Rectl;
    HPS     hps;
    ULONG   ulStyle;
    ULONG   ulDrawStyle;
    ULONG   ulColors;
    ULONG   ulThickness;

    switch (msg)
    {
        case WM_CREATE:
            /* Initialize border and interior colors */
            WinSetWindowULong (hWnd, QWL_COLORS,
                MAKELONG (SYSCLR_BUTTONDARK, SYSCLR_BUTTONMIDDLE));

            ulStyle = WinQueryWindowULong (hWnd, QWL_STYLE);
            if (ulStyle & (BVS_PREVID | BVS_PREVWINDOW))
            {
                SWP    SwpBevel;

                /* Query bevel window parameters
                   x Position specifies the horizontal gap between 
                     bevel and sibling windows
                   y Position specifies the vertical gap between
                     bevel and sibling windows
                   cx specifies the width of the bevel */
               
                WinQueryWindowPos (hWnd, &SwpBevel);
                QueryBorderRect (hWnd, ulStyle, &Rectl);

                /* Position the bevel window */
                WinSetWindowPos (hWnd, 0,
                    Rectl.xLeft   - SwpBevel.x, 
                    Rectl.yBottom - SwpBevel.y,
                    Rectl.xRight  - Rectl.xLeft   + (SwpBevel.x << 1),
                    Rectl.yTop    - Rectl.yBottom + (SwpBevel.y << 1), 
                    SWP_MOVE | SWP_SIZE);

                /* If width is zero use nominal width bevel */
                if (SwpBevel.cx == 0)
                    SwpBevel.cx = 1;
                WinSetWindowUShort (hWnd, QWS_THICKNESS, (USHORT)SwpBevel.cx);
            }
            else
                WinSetWindowUShort (hWnd, QWS_THICKNESS, 1);
            break;
                                                                        
        case BVM_RESIZE:
            ulStyle = WinQueryWindowULong (hWnd, QWL_STYLE);
            if (ulStyle & (BVS_PREVID | BVS_PREVWINDOW))
            {
                RECTL Rectl;

                QueryBorderRect (hWnd, ulStyle, &Rectl);

                /* Position the bevel window */
                WinSetWindowPos (hWnd, 0,
                    Rectl.xLeft   - SHORT1FROMMP(mp1), 
                    Rectl.yBottom - SHORT2FROMMP(mp1),
                    Rectl.xRight  - Rectl.xLeft   + (SHORT1FROMMP(mp1) << 1),
                    Rectl.yTop    - Rectl.yBottom + (SHORT2FROMMP(mp1) << 1), 
                    SWP_MOVE | SWP_SIZE);
            }
            break;

        case BVM_SETCOLOR:
            /* LOUSHORT(mp1) == Border color
               HIUSHORT(mp1) == Interior color */
            WinSetWindowULong (hWnd, QWL_COLORS, (ULONG)mp1);

            /* Repaint bevel with new color */
            WinInvalidateRect (hWnd, NULL, FALSE);
            break;

        case BVM_SETTHICKNESS:
            WinSetWindowUShort (hWnd, QWS_THICKNESS, SHORT1FROMMP(mp1));
            break;

        case WM_PAINT:
            hps         = WinBeginPaint (hWnd,0,0);
            ulColors    = WinQueryWindowULong (hWnd, QWL_COLORS);
            ulThickness = (ULONG)WinQueryWindowUShort (hWnd, QWS_THICKNESS);
            ulStyle     = WinQueryWindowULong (hWnd, QWL_STYLE) & 0x0000FFFF;

            ulDrawStyle = 0L;
            if (ulStyle & BVS_BEVELOUT)
                if (ulStyle & BVS_LINE)
                    ulDrawStyle |= DB_FENCE;
                else 
                    ulDrawStyle |= DB_RAISED;  
            else if (ulStyle & BVS_LINE)
                ulDrawStyle |= DB_TROUGH;
            else
                ulDrawStyle |= DB_DEPRESSED;
            if (ulStyle & BVS_FILL)
                ulDrawStyle |= DB_INTERIOR;

            WinQueryWindowRect (hWnd, &Rectl);
            WinDrawBorder (hps, &Rectl, ulThickness, ulThickness,
                LOUSHORT(ulColors), HIUSHORT(ulColors), ulDrawStyle);

            WinEndPaint (hps);
            mReturn = 0L;
            break;

        case WM_HITTEST:
            /* Allow mouse message to pass through this window */
            mReturn = (MRESULT)HT_TRANSPARENT;
            break;


        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}

ULONG CalculatePercentage (HWND hWnd)
{
    LONG lLowerRange;
    LONG lUpperRange;
    LONG lValue;

    lLowerRange = WinQueryWindowULong (hWnd, QWL_LOWERRANGE);
    lUpperRange = WinQueryWindowULong (hWnd, QWL_UPPERRANGE);
    lValue      = WinQueryWindowULong (hWnd, QWL_VALUE);
    if (lUpperRange <= lLowerRange)
        return (1L);
    else
        return ( ((lValue - lLowerRange) * 100) / (lUpperRange - lLowerRange) );
}

VOID DrawPercentages (HPS hps, PRECTL pRectl)
{
    ULONG  ulTop;
    ULONG  ulBottom;
    POINTL Ptl;
    RECTL  Rectl;
    ULONG  ulHeight;

    Rectl.xLeft  = pRectl->xRight * 6 / 10;
    Rectl.xRight = pRectl->xRight * 3 / 4 - 2;
    Ptl.x        = pRectl->xRight * 3 / 4;

    ulTop        = pRectl->yTop * 95 / 100;
    ulBottom     = pRectl->yTop / 4;
    ulHeight     = ulTop - ulBottom;

    Rectl.yBottom = ulTop;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_FENCE);
    Ptl.y         = Rectl.yBottom - 2;
    GpiCharStringAt (hps, &Ptl, 4, "100%");

    Rectl.yBottom = ulBottom + (ulHeight * 9) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_TROUGH);

    Rectl.yBottom = ulBottom + (ulHeight * 8) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_FENCE);
    Ptl.y         = Rectl.yBottom - 2;
    GpiCharStringAt (hps, &Ptl, 3, "80%");

    Rectl.yBottom = ulBottom + (ulHeight * 7) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_TROUGH);

    Rectl.yBottom = ulBottom + (ulHeight * 6) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_FENCE);
    Ptl.y         = Rectl.yBottom - 2;
    GpiCharStringAt (hps, &Ptl, 3, "60%");

    Rectl.yBottom = ulBottom + (ulHeight * 5) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_TROUGH);

    Rectl.yBottom = ulBottom + (ulHeight * 4) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_FENCE);
    Ptl.y         = Rectl.yBottom - 2;
    GpiCharStringAt (hps, &Ptl, 3, "40%");

    Rectl.yBottom = ulBottom + (ulHeight * 3) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_TROUGH);

    Rectl.yBottom = ulBottom + (ulHeight * 2) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_FENCE);
    Ptl.y         = Rectl.yBottom - 2;
    GpiCharStringAt (hps, &Ptl, 3, "20%");

    Rectl.yBottom = ulBottom + (ulHeight * 1) / 10;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_TROUGH);

    Rectl.yBottom = ulBottom;
    Rectl.yTop    = Rectl.yBottom + 2;
    WinDrawBorder (hps, &Rectl, 1L, 1L, CLR_BLACK, CLR_WHITE, DB_FENCE);
    Ptl.y         = Rectl.yBottom - 2;
    GpiCharStringAt (hps, &Ptl, 2, "0%");

    return;
}

BOOL APIENTRY DrawTemperature (HPS hPS,PRECTL pRectl,LONG lColor,LONG lBackColor,
                               LONG lPercent)
/*-----------------------------------------------------------------*\

   This function will fill draw the vertical bar portion of the
   thermometer. lColor is used to fill lPercent of the bar starting
   at the top of the bulb. lBackColor is used to fill the remainder
   of the bar. The rectangle identified by pRectl should be identical
   to that supplied to DrawThermometer.

   hPS        Presentation space to render in.
   pRectl     Bounding box of the thermometer
   lColor     Color index used to fill bulb and stem of thermometer.
   lBackColor Color to fill the remainder of the stem (100 - lPercent)
   lPercent   Amount to fill the thermometer. Valid range is 0 to 100.


\*-----------------------------------------------------------------*/
{
    AREABUNDLE ab;
    RECTL      Rectl;
    int        iWidth  = (pRectl->xRight - pRectl->xLeft) / 3;
    int        iBoxHeight = pRectl->yTop - pRectl->yBottom;
    int        iBarHeight = (iBoxHeight * 70) / 100;

    /* Range check the percent */
    if (lPercent > 100)
        lPercent = 100;
    else if (lPercent < 0)
        lPercent = 0;

    Rectl.xLeft = pRectl->xLeft + iWidth;
    Rectl.yTop  = pRectl->yTop - (iBoxHeight * 5) / 100;
    if (lPercent)
        Rectl.yBottom = Rectl.yTop - ((iBarHeight * (100 - lPercent)) / 100);
    else
        Rectl.yBottom = Rectl.yTop - iBarHeight;
    Rectl.xRight  = pRectl->xRight - iWidth;

    ab.lColor = lBackColor;
    GpiSetAttrs(hPS,PRIM_AREA,ABB_COLOR,0,(PBUNDLE)&ab);
    GpiMove (hPS,(PPOINTL)&Rectl);
    GpiBox (hPS,DRO_FILL,(PPOINTL)&Rectl.xRight,0,0);

    Rectl.yTop = Rectl.yBottom - 1;
    Rectl.yBottom = pRectl->yBottom + (iBoxHeight * 20) / 100;
    ab.lColor = lColor;
    GpiSetAttrs(hPS,PRIM_AREA,ABB_COLOR,0,(PBUNDLE)&ab);
    GpiMove (hPS,(PPOINTL)&Rectl);
    GpiBox (hPS,DRO_FILL,(PPOINTL)&Rectl.xRight,0,0);
    return (TRUE);
}


BOOL APIENTRY DrawThermometer (HPS hPS,PRECTL pRectl,LONG lStyle,LONG lColor,
      LONG lBackColor, LONG lPercent)
/*-----------------------------------------------------------------*\

   This function will draw the outine of a thermometer scaled to
   the supplied rectangle in the give presentation space . The bulb 
   of the rectangle is filled with lColor and the remainder is drawn 
   by the DrawTemperature function. 

   hPS        Presentation space to render in.
   pRectl     Bounding box of the thermometer
   lStyle     Currently undefined
   lColor     Color index used to fill bulb and stem of thermometer.
   lBackColor Color to fill the remainder of the stem (100 - lPercent)
   lPercent   Amount to fill the thermometer. Valid range is 0 to 100.

\*-----------------------------------------------------------------*/
{
    LINEBUNDLE lb;
    AREABUNDLE ab;
    ARCPARAMS  arcparms; 
    POINTL     lPtCenter;
    POINTL     lStartPt;
    POINTL     lEndPt;
    POINTL     lPt;
    FIXED      fxStartAngle,fxSweepAngle;   
    RECTL      BulbRectl;
    int        iBoxHeight = pRectl->yTop - pRectl->yBottom;

    /* Calculate the height of the bulb to be 25% of the total height */
    BulbRectl = *pRectl;
    BulbRectl.yTop = BulbRectl.yBottom + (iBoxHeight * 25) / 100;

    /* define the bounding box for the arc */
    arcparms.lP = (BulbRectl.xRight - BulbRectl.xLeft) / 2L;
    arcparms.lQ = (BulbRectl.yTop - BulbRectl.yBottom) / 2L;
    arcparms.lR = 0L;
    arcparms.lS = 0L;
    GpiSetArcParams(hPS, (PARCPARAMS)&arcparms);

    /* Calculate the center point of bulb */
    lPtCenter.x = BulbRectl.xLeft + arcparms.lP;   
    lPtCenter.y = BulbRectl.yBottom + arcparms.lQ + 1;

    /*-------------------------------------------------------------------*\
                           Draw the bulb
    \*-------------------------------------------------------------------*/
    /* Draw an invisble arc from the current location to the start angle */
    fxStartAngle = MAKEFIXED(120,0);
    fxSweepAngle = MAKEFIXED(90,0);
    lb.usType = LINETYPE_INVISIBLE;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,MAKEFIXED(0,0));
    GpiQueryCurrentPosition(hPS,(PPOINTL)&lStartPt);

    /* Draw the left highlighted outline of the bulb */
    lb.lColor = CLR_WHITE;
    lb.usType = LINETYPE_SOLID;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,fxSweepAngle);

    /* Draw the right shadowed outline of the bulb */
    fxStartAngle = MAKEFIXED(210,0);
    fxSweepAngle = MAKEFIXED(210,0);
    lb.lColor = CLR_DARKGRAY;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,fxSweepAngle);
    GpiQueryCurrentPosition(hPS,(PPOINTL)&lEndPt);

    /* Fill the bulb of the thermometer with the supplied color */
    arcparms.lP -= 4;
    arcparms.lQ -= 4;
    GpiSetArcParams(hPS, (PARCPARAMS)&arcparms);
    ab.lColor = lColor;
    GpiSetAttrs(hPS,PRIM_AREA,ABB_COLOR,0,(PBUNDLE)&ab);
    GpiMove (hPS,&lPtCenter);
    GpiFullArc(hPS,DRO_FILL,MAKEFIXED(1,0));

    /*-------------------------------------------------------------------*\
                  Draw the reflection on the bulb 
    \*-------------------------------------------------------------------*/
    arcparms.lP = (arcparms.lP * 3) / 4;
    arcparms.lQ = (arcparms.lQ * 3) / 4;
    GpiSetArcParams(hPS, (PARCPARAMS)&arcparms);
    fxStartAngle = MAKEFIXED(120,0);
    fxSweepAngle = MAKEFIXED(90,0);
    lb.usType = LINETYPE_INVISIBLE;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,MAKEFIXED(0,0));

    lb.lColor = CLR_WHITE;
    lb.usType = LINETYPE_SOLID;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,fxSweepAngle);

    arcparms.lP = (arcparms.lP * 2) / 3;
    arcparms.lQ = (arcparms.lQ * 2) / 3;
    GpiSetArcParams(hPS, (PARCPARAMS)&arcparms);
    fxStartAngle = MAKEFIXED(120,0);
    fxSweepAngle = MAKEFIXED(90,0);
    lb.usType = LINETYPE_INVISIBLE;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,MAKEFIXED(0,0));

    lb.lColor = CLR_WHITE;
    lb.usType = LINETYPE_SOLID;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,fxSweepAngle);

    /*-------------------------------------------------------------------*\
                           Draw the sides
    \*-------------------------------------------------------------------*/
    /* Draw the right vertical side */
    lb.lColor = CLR_DARKGRAY;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR,0,(PBUNDLE)&lb);
    GpiMove(hPS,(PPOINTL)&lEndPt);
    lPt.x = lEndPt.x;
    lPt.y = pRectl->yBottom + ((iBoxHeight * 95) / 100);
    GpiLine (hPS,&lPt);

    /* Draw the left vertical side */
    lb.lColor = CLR_WHITE;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR,0,(PBUNDLE)&lb);
    lPt.x = lStartPt.x;
    GpiMove(hPS,(PPOINTL)&lPt);
    GpiLine (hPS,&lStartPt);

    /*-------------------------------------------------------------------*\
                           Draw the rounded top
    \*-------------------------------------------------------------------*/
    /* Reset the rectangle for drawing the rounded top */
    BulbRectl.xLeft   = lStartPt.x;
    BulbRectl.yBottom = pRectl->yBottom + (iBoxHeight * 90) / 100;
    BulbRectl.xRight  = lEndPt.x;
    BulbRectl.yTop    = pRectl->yTop;

    arcparms.lP = (BulbRectl.xRight - BulbRectl.xLeft) / 2L;
    arcparms.lQ = (BulbRectl.yTop - BulbRectl.yBottom) / 2L;
    GpiSetArcParams(hPS, (PARCPARAMS)&arcparms);

    /* Calculate the center point for the rounded top arc */
    lPtCenter.x = BulbRectl.xLeft + arcparms.lP;   
    lPtCenter.y = BulbRectl.yBottom + arcparms.lQ + 1;

    /* Draw an invisble arc from the current location to the start angle */
    fxStartAngle = MAKEFIXED(0,0);
    fxSweepAngle = MAKEFIXED(120,0);
    lb.usType = LINETYPE_INVISIBLE;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,MAKEFIXED(0,0));

    /* Draw the shaded side */
    lb.lColor = CLR_DARKGRAY;
    lb.usType = LINETYPE_SOLID;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,fxSweepAngle);

    /* Draw the highlighed side */
    fxStartAngle = MAKEFIXED(120,0);
    fxSweepAngle = MAKEFIXED(60,0);
    lb.lColor = CLR_WHITE;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,fxSweepAngle);

    /* Draw the reflection */
    arcparms.lP = (arcparms.lP * 3) / 4;
    arcparms.lQ = (arcparms.lQ * 3) / 4;
    GpiSetArcParams(hPS, (PARCPARAMS)&arcparms);
    fxStartAngle = MAKEFIXED(90,0);
    fxSweepAngle = MAKEFIXED(90,0);
    lb.usType = LINETYPE_INVISIBLE;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,MAKEFIXED(0,0));

    lb.lColor = CLR_WHITE;
    lb.usType = LINETYPE_SOLID;
    GpiSetAttrs (hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);
    GpiPartialArc (hPS,&lPtCenter,MAKEFIXED(1,0),fxStartAngle,fxSweepAngle);

    DrawTemperature(hPS,pRectl,lColor,lBackColor,lPercent);
    return (TRUE);
}


MRESULT EXPENTRY ThermometerWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    RECTL   Rectl;
    HPS     hps;
    ULONG   ulColors;

    switch (msg)
    {
        case WM_CREATE:
            /* Initialize the window extra bytes */
            WinSetWindowULong (hWnd, QWL_LOWERRANGE, 0L);
            WinSetWindowULong (hWnd, QWL_UPPERRANGE, 0L);
            WinSetWindowULong (hWnd, QWL_VALUE, 0L);
            WinSetWindowULong (hWnd, QWL_COLOR, MAKELONG(CLR_RED,CLR_PALEGRAY));
            break;

        case THM_SETRANGE:
            WinSetWindowULong (hWnd, QWL_LOWERRANGE, (ULONG)mp1);
            WinSetWindowULong (hWnd, QWL_UPPERRANGE, (ULONG)mp2);
            break;

        case THM_SETVALUE:
            /* Set the new value */
            WinSetWindowULong (hWnd, QWL_VALUE, (ULONG)mp1);

            /* Query colors and thermometer size */
            ulColors = WinQueryWindowULong (hWnd, QWL_COLOR);
            WinQueryWindowRect (hWnd, &Rectl);
            if (!(WinQueryWindowULong (hWnd, QWL_STYLE) & THS_NOPERCENT))
            {
                Rectl.xRight  = (Rectl.xRight * 6) / 10;
                Rectl.xLeft  += (Rectl.xRight / 5);
            }
            Rectl.yTop--;

            /* Update the thermometer temperature */
            hps = WinGetPS (hWnd);
            DrawTemperature (hps, &Rectl, (LONG)((SHORT)ulColors), 
                (LONG)(ulColors >> 16), CalculatePercentage(hWnd));
            WinReleasePS (hps);
            break;

        case THM_SETCOLOR:
            WinSetWindowULong (hWnd, QWL_COLOR, (ULONG)mp1);
            WinInvalidateRect (hWnd, NULL, FALSE);
            break;

        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            WinQueryWindowRect (hWnd, &Rectl);

            /* Fill window with background color */
            ulColors = WinQueryWindowULong (hWnd, QWL_COLOR);
            WinFillRect (hps, &Rectl, (LONG)(ulColors >> 16));

            if (!(WinQueryWindowULong (hWnd, QWL_STYLE) & THS_NOPERCENT))
            {
                DrawPercentages (hps, &Rectl);

                Rectl.xRight  = (Rectl.xRight * 6) / 10;
                Rectl.xLeft  += (Rectl.xRight / 5);
            }
            Rectl.yTop--;

            DrawThermometer (hps, &Rectl, 0L, (LONG)((SHORT)ulColors), 
                (LONG)(ulColors >> 16), CalculatePercentage(hWnd)); 

            WinEndPaint (hps);
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}


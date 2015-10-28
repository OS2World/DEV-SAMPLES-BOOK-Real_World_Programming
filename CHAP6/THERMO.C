/* --------------------------------------------------------------------
                       Thermometer Drawing sample
                              Chapter 6

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include "thermo.h"



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

    if (lStyle & TS_SHOW_PERCENT)
    {
    }

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

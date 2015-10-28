/* --------------------------------------------------------------------
                             LED Control
                              Chapter 14

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* LED Control
     Class Name  - LEDCLASS
     Messages    
        WM_ENABLE - MPFROMSHORT (enablestate)
*/

#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include "led.h"
#include "phone.h"

/* Window and Dialog Functions */
MRESULT EXPENTRY LEDWndProc      (HWND,ULONG,MPARAM,MPARAM);

/* Window extra bytes */
#define QWS_ENABLED         0
#define QWL_TEXT            QWS_ENABLED + sizeof(USHORT)
#define LEDEXTRA            QWL_TEXT    + sizeof(ULONG)
                                       
/* Local Functions */
VOID RedrawLED (HWND,HPS,USHORT);

/* Global Variables */
HBITMAP  hbmLED;               /* LED bitmap (contains 2 states) */
ULONG    ulLEDCount = 0;       /* Number of LED controls         */
ULONG    bmWidth;              /* Width of each bitmap state     */
ULONG    bmHeight;             /* Height of bitmap               */


/* ----------------------- Local Functions ---------------------- */

VOID RedrawLED (HWND hWnd, HPS hps, USHORT usEnabled)
{
    BOOL   bRelease = FALSE;
    ULONG  ulTxt;
    RECTL  Rectl;
    POINTL Ptl;

    if (!hps)
    {
        bRelease = TRUE;
        hps = WinGetPS (hWnd);
    }

    WinQueryWindowRect (hWnd, &Rectl);

    /* Draw text with bottom 1 pel above bitmap */
    Rectl.yBottom = bmHeight + 1;
    ulTxt = WinQueryWindowULong (hWnd, QWL_TEXT);
    WinDrawText (hps, -1, (PSZ)&ulTxt, &Rectl, 0L, 0L, 
       DT_CENTER | DT_BOTTOM | DT_TEXTATTRS);

    /* Draw bitmap state */
    Ptl.x = (Rectl.xRight - bmWidth) >> 1;
    Ptl.y = 0;
    Rectl.xLeft   = usEnabled ? 0 : bmWidth;
    Rectl.xRight  = Rectl.xLeft + bmWidth;
    Rectl.yBottom = 0L;
    Rectl.yTop    = bmHeight;
    WinDrawBitmap (hps, hbmLED, &Rectl, &Ptl, 0L, 0L, DBM_NORMAL);

    if (bRelease)
        WinReleasePS (hps);

    return;
}

/* ---------------------- External Function --------------------- */

VOID RegisterLEDClass (HAB hab)
{
   WinRegisterClass (hab, LEDCLASS, LEDWndProc, 0, LEDEXTRA);

   return;
}

/* ----------------------- Window Function ---------------------- */

MRESULT EXPENTRY LEDWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL             bHandled = TRUE;
    MRESULT          mReturn  = 0;
    RECTL            Rectl;
    HPS              hps;
    ULONG            ulText;
    PSZ              pszText;
    BITMAPINFOHEADER bi;

    switch (msg)
    {
        case WM_CREATE:
            /* Initial state is disabled */
            WinSetWindowUShort (hWnd, QWS_ENABLED, 0);

            /* Save 3 character LED text in window extra bytes as a long */
            pszText = ((PCREATESTRUCT)mp2)->pszText;
            ulText  = (pszText[2] << 16) | (pszText[1] << 8) | pszText[0];
            WinSetWindowULong (hWnd, QWL_TEXT, ulText);

            /* Set font in the control */
            WinSetPresParam (hWnd, PP_FONTNAMESIZE, FONTNAMELEN, HELV8FONT);

            /* Load the LED bitmap when first LED control is created */
            if (!ulLEDCount)
            {
                hps = WinGetPS (hWnd);
                hbmLED = GpiLoadBitmap (hps, 0, IDB_LED, 0L, 0L);
                GpiQueryBitmapParameters (hbmLED, &bi);
                bmWidth  = bi.cx >> 1; 
                bmHeight = bi.cy;
                WinReleasePS (hps);
            }
            ulLEDCount++;
            break;

        case WM_ENABLE:
            WinSetWindowUShort (hWnd, QWS_ENABLED, SHORT1FROMMP(mp1));
            RedrawLED (hWnd, 0, SHORT1FROMMP(mp1));
            break;

        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            WinQueryWindowRect (hWnd, &Rectl);
            WinFillRect (hps, &Rectl, CLR_PALEGRAY);
            RedrawLED (hWnd, hps, WinQueryWindowUShort (hWnd, QWS_ENABLED));
            WinEndPaint (hps);
            break;

        case WM_DESTROY:
            if (!(--ulLEDCount))
                GpiDeleteBitmap (hbmLED);
            break;
                          
        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}


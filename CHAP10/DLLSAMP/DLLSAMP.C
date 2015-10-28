/* --------------------------------------------------------------------
                           DLLSAMP Program
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include "dllsamp.h"

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

VOID EXPENTRY FillWindow (HWND hWnd, HPS hps, ULONG ulRGB)
{
    RECTL Rectl;
    BOOL  bRelease = FALSE;

    if (!hps)
    {
        hps = WinBeginPaint (hWnd, 0, 0);
        bRelease = TRUE;
    }
    WinQueryWindowRect (hWnd, &Rectl);
    /* Set color state in the hps to RGB mode */
    GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0L, 0L, NULL);
    WinFillRect (hps, &Rectl, ulRGB);
    if (bRelease)
        WinEndPaint (hps);
    return;
}


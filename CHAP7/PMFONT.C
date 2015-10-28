/* --------------------------------------------------------------------
                           PM Font Program
                              Chapter 7

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */


#define INCL_DOS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPILCIDS
#define INCL_DEV
#define INCL_WINERRORS

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <os2.h>
#include "pmfont.h"
#include "chap7.h"
#include "..\common\about.h"

BOOL APIENTRY GpiSetTextAlignment(HPS,LONG,LONG);

HAB  hab;
PVOID  pMem;
char   szBuff1[MAX_BUFF];
int    cxChar = 0;
int    cyChar = 0;
HWND   hWndMetrics = 0;
HWND   hWndShear = 0;
HWND   hMenu;
USHORT usDisplayMode  = IDM_METRICS;
USHORT usCharMode = IDM_CHMODE_ONE;
USHORT usSelection = 0;
USHORT usCurrentCP = 850;
USHORT usCurrentFontIndex = 0;
LONG   cxFontRes;
LONG   cyFontRes;
LONG   lHorzRes;
LONG   lVertRes;
double dRotSine;
double dRotCosine;
CHAR   szFontFace[] = "10.Helv";
CHAR   szDisplayStr[MAX_BUFF];
CHAR   szSpecialStr[MAX_STR];
CHAR   szUserStr[MAX_BUFF];
CHARBUNDLE CurrentCB;

GRADIENTL DefaultAngle = {0L,0L};
//GRADIENTL CurrentAngle = {0L,0L};
SIZEF     DefaultCharBox = {0,0};
FIXED     CharExtra = 0;
FIXED     BreakExtra = 0;

CHAR  *szWeights[] =
{
    "Ultra-light",
    "Extra-light",
    "Light",
    "Semi-light",
    "Medium",
    "Semi-bols",
    "Bold",
    "Extra-bols",
    "Ultra-bold"
};

CHAR  *szWidths[] =
{  
    "Ultra-condensed",
    "Extra-condensed",
    "Condensed",
    "Semi-condensed",
    "Medium",
    "Semi-expanded",
    "Expanded",
    "Extra-expanded",
    "Ultra-expanded"
};

CHAR  *szQuality[]=
{
    "Undefined",
    "DP Quality",
    "DP Draft",
    "Near Letter Quality",
    "Leter Quality"
};

CHAR  *szFormat[]=
{
    "Device",
    "GPI"
};

CHAR  *szType[]=
{
    " Bitmap",
    " Outline"
};

CHAR  *szSelection[]=
{
    "",
    "Bold",
    " Italic",
    " Strikeout",
    " UnderScore",
    " Outline"
};

CHAR  *szSpacing[]=
{
    "Proportional",
    "Fixed"
};

int iCharModes[] =
{
    CM_MODE1,CM_MODE2,CM_MODE3
};

#define BUFF_SIZE 80

HDC          hDisplayInfoDC = 0;
HPS          hDisplayPS = 0;
HWND         hWndFrame,hWndClient;
HWND         hWndDC;
HWND         hFontLBox;
PFONTMETRICS pScreenFontMetrics = 0;


MRESULT APIENTRY ClientWndProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT APIENTRY MetricsWndProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT APIENTRY ShearWndProc(HWND,ULONG,MPARAM,MPARAM);


int main ()
{
    HMQ    hmq;
    QMSG   qmsg;
    ULONG  flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                             FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                             FCF_ICON | FCF_MENU;
    CHAR   szClientClass[] = "CLIENT";
    RECTL  Rectl;
    int    i = 0;
    int    iClientHeight;

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    DosAllocMem((PPVOID)&pMem,0x1000,PAG_READ | PAG_WRITE);
    DosSubSet(pMem,DOSSUB_INIT | DOSSUB_SPARSE_OBJ,8192);

    WinRegisterClass (hab, szClientClass, ClientWndProc, CS_SIZEREDRAW | CS_CLIPCHILDREN, 0);
    WinLoadString (hab, 0, IDS_APPNAME, sizeof(szBuff1), szBuff1);

    WinQueryWindowRect (HWND_DESKTOP,&Rectl);
    iClientHeight = WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) -
                    WinQuerySysValue(HWND_DESKTOP,SV_CYICON);
    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
        &flFrameFlags, szClientClass, szBuff1, 0, 0, ID_APPNAME, &hWndClient);

    WinSetWindowPos (hWndFrame,HWND_TOP,0,
        WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) - iClientHeight,
        WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN),iClientHeight,
        SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER | SWP_SIZE | SWP_MOVE);

    hMenu  = WinWindowFromID (hWndFrame,FID_MENU);
    hWndDC = WinOpenWindowDC(hWndClient);

    WinEnableWindowUpdate(hWndClient,FALSE);
    if (create_controls(hWndClient))
    {
        WinEnableWindowUpdate(hWndClient,TRUE);
        WinShowWindow (hFontLBox,TRUE);
        WinSendMsg(hFontLBox,LM_SELECTITEM,MPFROMSHORT(0),MPFROMSHORT(TRUE));
        WinSetFocus(HWND_DESKTOP,hFontLBox);
        WinShowWindow (hWndMetrics,TRUE);
    }
    else
        return(0);

    while (WinGetMsg (hab,&qmsg,0,0,0))
        WinDispatchMsg (hab,&qmsg);

    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return 0;
}


VOID APIENTRY charbox_from_pointsize (FIXED fxPtSize,PSIZEF pSizef)
{
    pSizef->cx = MAKEFIXED( ((fxPtSize * cxFontRes) / 72),0);
    pSizef->cy = MAKEFIXED( ((fxPtSize * cyFontRes) / 72),0);
} 


void APIENTRY cleanup()
/*-----------------------------------------------------------------*\

   This function is called to free the memory allocated and destroy
   the application window.

\*-----------------------------------------------------------------*/
{
    if (pScreenFontMetrics)
        DosFreeMem(pScreenFontMetrics);
    DosFreeMem(pMem);
    if (hWndMetrics)
        WinDestroyWindow (hWndMetrics);
    if (hWndShear)
        WinDestroyWindow (hWndShear);
    WinDestroyWindow (hWndFrame);
}


void APIENTRY draw_grid(HPS hPS,PRECTL pRectl)
{
    POINTL   ptlGrid[4];

    ptlGrid[0].x = pRectl->xLeft;
    ptlGrid[0].y = pRectl->yTop / 2;
    ptlGrid[1].x = pRectl->xRight;
    ptlGrid[1].y = ptlGrid[0].y;
    ptlGrid[2].x = pRectl->xLeft + ((pRectl->xRight - pRectl->xLeft) / 2);
    ptlGrid[2].y = pRectl->yBottom;
    ptlGrid[3].x = ptlGrid[2].x;
    ptlGrid[3].y = pRectl->yTop;
    GpiMove (hPS,&ptlGrid[0]);
    GpiLine (hPS,&ptlGrid[1]);
    GpiMove (hPS,&ptlGrid[2]);
    GpiLine (hPS,&ptlGrid[3]);
}


BOOL APIENTRY create_controls(HWND hWndClient)
/*-----------------------------------------------------------------*\

   This function will create the font selection listbox and load
   the metrics and rotation dialogs. The fonts will be added to the
   listbox and the default display strings are loaded. All windows
   are left hidden.

\*-----------------------------------------------------------------*/
{
    RECTL  ClientRectl;
    RECTL  MetricsRectl;
    HWND   hWndEdit;
    HPS    hPS;

    WinLoadString (hab, 0, IDS_DISPLAY_TEXT,MAX_BUFF,szDisplayStr);
    WinLoadString (hab, 0, IDS_SPECIAL_TEXT,MAX_STR,szSpecialStr);

    WinQueryWindowRect(hWndClient,(PRECTL)&ClientRectl);

    /* Load the Metrics Window */
    if(hWndMetrics = WinLoadDlg(hWndClient,hWndClient,MetricsWndProc,0,METRICS_DLG,0))
    {
        WinQueryWindowRect(hWndMetrics,(PRECTL)&MetricsRectl);
        WinSetWindowPos (hWndMetrics,HWND_TOP,
            cxChar,ClientRectl.yTop - MetricsRectl.yTop - (cyChar * 5),0,0,
            SWP_ZORDER | SWP_MOVE);
    }
    else
       return(FALSE);

    /* Create the font selection list box */
    if (hFontLBox = WinCreateWindow (hWndClient,WC_LISTBOX,0,
            0L,
            cxChar,ClientRectl.yTop - (cyChar * 4),
            MetricsRectl.xRight,cyChar * 3,
            hWndClient,HWND_TOP,FONT_LBOX,0,0))
    {
        WinSetPresParam(hFontLBox,PP_FONTNAMESIZE,strlen(szFontFace)+1,(PVOID)szFontFace);
        hPS = WinGetPS (hWndClient);
        pScreenFontMetrics = init_font_LBox(hFontLBox,hPS);
        WinReleasePS(hPS);
    }
    else
       return(FALSE);

    /* Load the Shear Window and leave it hidden */
    if (hWndShear = WinLoadDlg(hWndClient,hWndClient,ShearWndProc,0,SHEAR_DLG,0))
    {
        WinQueryWindowRect(hWndShear,(PRECTL)&MetricsRectl);
        WinSetWindowPos (hWndShear,HWND_TOP,
            cxChar,ClientRectl.yTop - MetricsRectl.yTop - (cyChar * 5),0,0,
            SWP_ZORDER | SWP_MOVE);
        WinLoadString (hab,0,IDS_USER_TEXT,MAX_STR,szUserStr);
        hWndEdit = WinWindowFromID(hWndShear,DID_USER_TEXT);
        WinSendMsg(hWndEdit,EM_SETTEXTLIMIT,MPFROMSHORT(MAX_STR),0);
        WinSetWindowText(hWndEdit,szUserStr);
    }
    else
       return(FALSE);
    return (TRUE);
}


USHORT APIENTRY post_error(HWND hWnd,USHORT usErrorID)
/*-----------------------------------------------------------------*\

   This is a utility function which will post a message box with
   a string loaded from the resource file.

\*-----------------------------------------------------------------*/
{
    WinLoadString (hab,0,usErrorID,MAX_BUFF,szBuff1);
    return ((USHORT)WinMessageBox(HWND_DESKTOP,hWnd,(PSZ)szBuff1,
        (PSZ)"Error",0,MB_OK | MB_ICONHAND | MB_APPLMODAL));
}


void APIENTRY process_command(HWND hWnd,MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   All WM_COMMAND messages received by the client window will be
   processed here.  

\*-----------------------------------------------------------------*/
{
    USHORT usAction;
    USHORT usAttribute;

    switch(SHORT1FROMMP(mp1))
    {
        case IDM_METRICS:
            if (usDisplayMode != IDM_METRICS)
            {
                WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_ROTATION,TRUE),
                    MPFROM2SHORT(MIA_CHECKED,FALSE));
                WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_METRICS,TRUE),
                    MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
                WinShowWindow(hWndMetrics,TRUE);
                WinShowWindow(hWndShear,FALSE);
                usDisplayMode = IDM_METRICS;
                /* Force a new font to be created */
                WinSendMsg(hWndClient,WM_CONTROL,MPFROM2SHORT(FONT_LBOX,LN_SELECT),0);
                WinInvalidateRect(hWndClient,0,FALSE);
            }
            break;

        case IDM_ROTATION:
            if (usDisplayMode != IDM_ROTATION)
            {
                WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_METRICS,TRUE),
                    MPFROM2SHORT(MIA_CHECKED,FALSE));
                WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_ROTATION,TRUE),
                    MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
                WinShowWindow(hWndShear,TRUE);
                WinShowWindow(hWndMetrics,FALSE);
                usDisplayMode = IDM_ROTATION;
                WinInvalidateRect(hWndClient,0,FALSE);
            }
            break;

        case IDM_CHMODE_ONE:
        case IDM_CHMODE_TWO:
        case IDM_CHMODE_THREE:
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usCharMode,TRUE),
                MPFROM2SHORT(MIA_CHECKED,FALSE));
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(MPFROMSHORT(mp1),TRUE),
                MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
            usCharMode = SHORT1FROMMP(mp1);
            /* Force a new font to be created */
            WinSendMsg(hWndClient,WM_CONTROL,MPFROM2SHORT(FONT_LBOX,LN_SELECT),0);
            break;

        case IDM_ATTR_ITALIC:
        case IDM_ATTR_BOLD:
        case IDM_ATTR_UNDERSCORE:
        case IDM_ATTR_STRIKEOUT:
        case IDM_ATTR_OUTLINE:
            switch (SHORT1FROMMP(mp1))
            {
                case IDM_ATTR_ITALIC:
                    usAttribute = FATTR_SEL_ITALIC;
                    break;
                case IDM_ATTR_BOLD:
                    usAttribute = FATTR_SEL_BOLD;
                    break;
                case IDM_ATTR_UNDERSCORE:
                    usAttribute = FATTR_SEL_UNDERSCORE;
                    break;
                case IDM_ATTR_STRIKEOUT:
                    usAttribute = FATTR_SEL_STRIKEOUT;
                    break;
                case IDM_ATTR_OUTLINE:
                    usAttribute = FATTR_SEL_OUTLINE;
                    break;
            }

            if (usSelection & usAttribute)
            {
               usSelection &= ~usAttribute;
               usAction = 0;
            }
            else
            {
                usSelection |= usAttribute;
                usAction = MIA_CHECKED;
            }
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(SHORT1FROMMP(mp1),TRUE),
                MPFROM2SHORT(MIA_CHECKED,usAction));
            /* Force a new font to be created */
            WinSendMsg(hWndClient,WM_CONTROL,MPFROM2SHORT(FONT_LBOX,LN_SELECT),0);
            break;

        case IDM_CP437:
        case IDM_CP850:
        case IDM_CP852:
        case IDM_CP857: 
        case IDM_CP860: 
        case IDM_CP861: 
        case IDM_CP863: 
        case IDM_CP865: 
        case IDM_CP1004:
        case IDM_CP037:
        case IDM_CP273:
        case IDM_CP274:
        case IDM_CP277:
        case IDM_CP278:
        case IDM_CP280:
        case IDM_CP282:
        case IDM_CP284:
        case IDM_CP285:
            if (usCurrentCP != SHORT1FROMMP(mp1))
            {
                WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usCurrentCP,TRUE),
                    MPFROM2SHORT(MIA_CHECKED,FALSE));
                WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(SHORT1FROMMP(mp1),TRUE),
                    MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
                usCurrentCP = SHORT1FROMMP(mp1);
                /* Force a new font to be created */
                WinSendMsg(hWndClient,WM_CONTROL,MPFROM2SHORT(FONT_LBOX,LN_SELECT),0);
            }
            break;

        case IDM_ABOUT:
            WinLoadString (hab, 0, IDS_APPNAME, sizeof(szBuff1), szBuff1);
            DisplayAbout (hWnd, szBuff1);
            break;

    }
}

void APIENTRY process_control(HWND hWnd,HPS hPS,MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   All WM_CONTROL messages received by the client window will be
   processed here.  

\*-----------------------------------------------------------------*/
{

    if (LOUSHORT(mp1) == FONT_LBOX)
    {
        if (SHORT2FROMMP(mp1) == LN_SELECT)
        {
            usCurrentFontIndex = (LONG)WinSendMsg (hFontLBox,LM_QUERYSELECTION,
                MPFROMSHORT(LIT_FIRST),0);
            WinSendMsg(hWndMetrics,WM_UPDATE_METRICS,0,
                pScreenFontMetrics + usCurrentFontIndex);
            create_font (hPS,(int)usCurrentFontIndex,pScreenFontMetrics);
            WinInvalidateRect (hWndClient,0,FALSE);
        }
    }
}



void APIENTRY process_paint (HWND hWnd,HPS hPS)
/*-----------------------------------------------------------------*\

   The entire client area is erased and the right side is redraw based 
   on the current display mode. If the mode is IDM_METRICS two strings
   are displayed in the current font. If the mode is IDM_ROTATION a
   string is draw using the current rotation and shear angles set in
   the current CHARBUNDLE. 

\*-----------------------------------------------------------------*/
{
    int      i;
    HRGN     hClipRgn;
    HRGN     hPrevRgn;
    RECTL    Rectl;
    RECTL    ClientRectl;
    RECTL    PanelRectl;
    POINTL   Pointl;
    POINTL   TempPoint;
    POINTL   Ptl[TXTBOX_COUNT + 1];
    double   dSine;
    double   dCosine;
    int      iLength;
    int      xExtent;

    hPS = WinBeginPaint(hWnd,hPS,&Rectl);

    /* Redraw the background of the Window */
    WinQueryWindowRect (hWnd,(PRECTL)&ClientRectl);
    WinQueryWindowRect (hWndMetrics,(PRECTL)&PanelRectl);
    PanelRectl.yTop = ClientRectl.yTop;
    PanelRectl.xRight += cxChar * 2;
    WinFillRect(hPS,(PRECTL)&PanelRectl,CLR_PALEGRAY);
    ClientRectl.xLeft = PanelRectl.xRight;
    WinFillRect(hPS,(PRECTL)&ClientRectl,CLR_WHITE);

    /* Set up a clipping rectangle before drawing the text */
    hClipRgn = GpiCreateRegion(hPS,1,&ClientRectl);
    GpiSetClipRegion(hPS,hClipRgn,&hPrevRgn);

    /* Update the character mode */
    GpiSetCharMode (hPS,iCharModes[(usCharMode - IDM_CHMODE) - 1]);

    if (usDisplayMode == IDM_METRICS)
    {
        GpiSetCharAngle (hPS,&DefaultAngle);
        GpiSetCharSet   (hPS,DISPLAY_FONT);

        Pointl.x = PanelRectl.xRight + cxChar;
        Pointl.y = (ClientRectl.yTop / 3) * 2;
        GpiCharStringAt (hPS,&Pointl,strlen(szDisplayStr),(PSZ)szDisplayStr);

        Pointl.y = ClientRectl.yTop / 3;
        GpiCharStringAt (hPS,&Pointl,strlen(szSpecialStr),(PSZ)szSpecialStr);
    }
    else if(usDisplayMode == IDM_ROTATION)
    {
        draw_grid(hPS,(PRECTL)&ClientRectl);

        GpiSetCharExtra(hPS,CharExtra); 
        GpiSetCharBreakExtra(hPS,BreakExtra); 

        iLength = strlen(szUserStr);

        /* Set the character set and box but reset the angle of rotation */
        GpiSetAttrs(hPS,PRIM_CHAR,CBB_SET | CBB_BOX | CBB_ANGLE,
            CBB_ANGLE,&CurrentCB);

        /* Query the extent of the string before rotation or shearing */
        GpiQueryTextBox(hPS,iLength,(PCH)szUserStr,TXTBOX_COUNT,Ptl);
        Pointl.x = PanelRectl.xRight + ((ClientRectl.xRight - ClientRectl.xLeft) / 2);
        Pointl.y = ClientRectl.yTop / 2;

        if (Ptl[TXTBOX_BOTTOMLEFT].x < Ptl[TXTBOX_TOPRIGHT].x)
            xExtent = (Ptl[TXTBOX_BOTTOMLEFT].x - Ptl[TXTBOX_TOPRIGHT].x / 2);
        else
           xExtent = (Ptl[TXTBOX_TOPRIGHT].x - Ptl[TXTBOX_BOTTOMLEFT].x / 2);

        /* Calculate the starting point for the rotated string */
        dSine   = xExtent * dRotSine;
        dCosine = xExtent * dRotCosine;
        Pointl.x += ((LONG)dCosine * lHorzRes)/(LONG)lVertRes;
        Pointl.y += (LONG) dSine;

        /* Set the character angle and shear */
        GpiSetAttrs(hPS,PRIM_CHAR,CBB_ANGLE | CBB_SHEAR,0,&CurrentCB);

        /* Get the extents of the string with rotation and shearing */
        GpiQueryTextBox(hPS,iLength,(PCH)szUserStr,TXTBOX_COUNT,Ptl);

        for (i = 0; i < 5; i++)
        {
            Ptl[i].x += Pointl.x;
            Ptl[i].y += Pointl.y;
        }
        TempPoint = Ptl[2];
        Ptl[2] = Ptl[3];
        Ptl[3] = TempPoint;
        Ptl[4].x = Ptl[0].x;
        Ptl[4].y = Ptl[0].y;

        /* Draw a polygon which displays the text bounding box */
        GpiMove(hPS,(PPOINTL)&Ptl[TXTBOX_TOPLEFT]);
        GpiPolyLine(hPS,5L,Ptl);
        GpiCharStringPosAt (hPS,&Pointl,0L,CHS_LEAVEPOS,
            iLength,(PSZ)szUserStr,0);
    }
    GpiSetClipRegion(hPS,0,&hPrevRgn);
    WinEndPaint (hPS);
}



PFONTMETRICS APIENTRY init_font_LBox (HWND hLBox,HPS hPS)
/*-----------------------------------------------------------------*\

   This function will query metrics for supplied presentation space
   and then enumerate its public and private fonts.

\*-----------------------------------------------------------------*/
{            
    HDC          hDC;
    PSZ          pFace;
    ULONG        ulSize = 0;
    ULONG        ulCount = 0;
    int          i;
    PVOID        pData;
    PFONTMETRICS pFontMetrics;
    HATOMTBL     hAtomTable = WinQuerySystemAtomTable();

    hDC = GpiQueryDevice(hPS);
    DevQueryCaps (hDC,CAPS_HORIZONTAL_RESOLUTION,1L,(PLONG)&lHorzRes);
    DevQueryCaps (hDC,CAPS_VERTICAL_RESOLUTION,1L,(PLONG)&lVertRes);
    DevQueryCaps (hDC,CAPS_HORIZONTAL_FONT_RES,1,&cxFontRes);
    DevQueryCaps (hDC,CAPS_VERTICAL_FONT_RES,1,&cyFontRes);

    ulCount = GpiQueryFonts (hPS,QF_PUBLIC | QF_PRIVATE,0,
                             &ulCount,(LONG)sizeof(FONTMETRICS),0);
    if (ulCount)
    {
        DosAllocMem ((PPVOID)&pFontMetrics,sizeof(FONTMETRICS) * (USHORT)ulCount,
            OBJ_TILE | PAG_READ | PAG_WRITE | PAG_COMMIT);
        GpiQueryFonts (hPS,QF_PUBLIC | QF_PRIVATE,0,
            &ulCount,(LONG)sizeof(FONTMETRICS),pFontMetrics);
        WinSendMsg (hLBox,LM_DELETEALL,0,0);

        for (i = 0; i < (int)ulCount; i++)
        {
#ifdef TOOLKT21
            /* If the facename has been truncated query the atom text */
            if ( ((pFontMetrics+i)->fsType & FM_TYPE_FACETRUNC) &&
                 (ulSize = WinQueryAtomLength(hAtomTable,(pFontMetrics+i)->FaceNameAtom)) )
            {
                ulSize++;
                DosSubAlloc(pMem,(PPVOID)&pData,ulSize);
                if (WinQueryAtomName (hAtomTable,(pFontMetrics+i)->FaceNameAtom,pData,ulSize))
                    pFace = pData;
                else
                    pFace = (pFontMetrics+i)->szFacename;
            }
            else
#endif
                pFace = (pFontMetrics+i)->szFacename;

            WinSendMsg (hLBox,LM_INSERTITEM,(MPARAM)LIT_END,(MPARAM)pFace);

            if (ulSize)
                DosSubFree(pMem,pData,ulSize);
        }
    }
    return (pFontMetrics);
}


BOOL APIENTRY create_font(HPS hPS,int i,PFONTMETRICS pFontArray)
/*-----------------------------------------------------------------*\

   This function will create a new font based on the current selection
   in the font listbox and style and codepage attributes set via the
   menu. The PS is first reset to the default character set attribute 
   and character.

\*-----------------------------------------------------------------*/
{
    FATTRS fattr;
    SIZEF  sizef;

    /* Reset the character box to the default size */
    sizef.cx = MAKEFIXED((pFontArray+i)->lEmInc,0);
    sizef.cy = MAKEFIXED((pFontArray+i)->lEmHeight,0);
    GpiSetCharBox (hPS,&sizef);
    GpiSetCharSet (hPS,LCID_DEFAULT);
    GpiDeleteSetId (hPS,DISPLAY_FONT);

    strcpy(fattr.szFacename,(PSZ)((pFontArray+i)->szFacename));
    fattr.usRecordLength  = sizeof(FATTRS);
    fattr.fsSelection     = usSelection;
    fattr.lMatch          = (pFontArray+i)->lMatch;
    fattr.idRegistry      = (pFontArray+i)->idRegistry;
    fattr.usCodePage      = usCurrentCP;
    fattr.lMaxBaselineExt = (pFontArray+i)->lMaxBaselineExt;
    fattr.lAveCharWidth   = (pFontArray+i)->lAveCharWidth;
    fattr.fsType          = 0;
    fattr.fsFontUse       = 0;
    return (GpiCreateLogFont (hPS,0,DISPLAY_FONT,&fattr));
}



void APIENTRY size_controls(MPARAM lNewSize)
/*-----------------------------------------------------------------*\

   This function will reposition the font selection listbox and the
   metrics and shear panel windows based on the new client window size.

\*-----------------------------------------------------------------*/
{
    SHORT  sNewWidth  = SHORT1FROMMP(lNewSize);
    SHORT  sNewHeight = SHORT2FROMMP(lNewSize);
    RECTL  MetricsRectl;

    WinQueryWindowRect(hWndMetrics,(PRECTL)&MetricsRectl);
    WinSetWindowPos (hWndMetrics,HWND_TOP,
        cxChar,sNewHeight - MetricsRectl.yTop - (cyChar * 5),0,0,SWP_MOVE);

    WinQueryWindowRect(hWndShear,(PRECTL)&MetricsRectl);
    WinSetWindowPos (hWndShear,HWND_TOP,
        cxChar,sNewHeight - MetricsRectl.yTop - (cyChar * 5),0,0,SWP_MOVE);

    WinSetWindowPos (hFontLBox,HWND_TOP,
        cxChar,sNewHeight - (cyChar * 4),0,0,SWP_ZORDER | SWP_MOVE);
}




MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

   This is the application callback which handles the messages 
   necessary to maintain the client window.

\*-----------------------------------------------------------------*/
{
    MRESULT     mReturn  = 0;
    BOOL        bHandled = TRUE;
    FONTMETRICS FontMetrics;
    static HPS  hPS;
    SIZEL       sizel;

    switch(msg)
    {
        case WM_CREATE:
            /* Create a PS for the window */
            sizel.cx = sizel.cy = 0;
            hPS = GpiCreatePS(hab,WinOpenWindowDC(hWnd),
              (PSIZEL)&sizel,PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
            /* Query the height of the default font. */
            GpiQueryFontMetrics(hPS,sizeof(FONTMETRICS),(PFONTMETRICS)&FontMetrics);
            cyChar = FontMetrics.lMaxBaselineExt;
            cxChar = FontMetrics.lAveCharWidth;
            bHandled = FALSE;
            break;

        case WM_ERASEBACKGROUND:
            WinFillRect((HPS)LONGFROMMP(mp1),PVOIDFROMMP(mp2),CLR_WHITE);
            mReturn = MRFROMLONG(0L);
            break;

        case WM_SIZE:
            size_controls(mp2);
            break;

        case WM_COMMAND:
            process_command(hWnd,mp1,mp2);
            break;

        case WM_CONTROL:
            process_control (hWnd,hPS,mp1,mp2);
            break;

        case WM_PAINT:
            process_paint (hWnd,hPS);
            break;

        case WM_DESTROY:
            cleanup();
            break;

        default:
            bHandled = FALSE;
            break;
    }
    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}



MRESULT EXPENTRY MetricsWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

   This dialog panel displays the metrics of the currently selected
   font. When the private WM_UPDATE_METRICS message is received the
   text of the child windows is updated with the contents of the 
   fontmetrics pointed to by mp2.

\*-----------------------------------------------------------------*/
{
    MRESULT      mReturn  = 0;
    BOOL         bHandled = TRUE;
    PFONTMETRICS pFM;
    CHAR         szTemp[80];

    switch(msg)
    {
        case WM_UPDATE_METRICS:
            pFM = (PFONTMETRICS)mp2;
            WinSetWindowText(WinWindowFromID(hWnd,DID_FAMILYNAME),
                pFM->szFamilyname);
            WinSetWindowText(WinWindowFromID(hWnd,DID_FACENAME),
                pFM->szFacename);
            ltoa(pFM->lLowerCaseAscent,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_LOWERCASEASCENT),(PSZ)szTemp);

            ltoa(pFM->lXHeight,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_XHEIGHT),(PSZ)szTemp);

            ltoa(pFM->lMaxAscender,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_MAXASCENDER),(PSZ)szTemp);

            ltoa(pFM->lMaxDescender,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_MAXDECENDER),(PSZ)szTemp);

            ltoa(pFM->lLowerCaseDescent,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_LOWERCASEDECENT),(PSZ)szTemp);

            ltoa(pFM->lEmInc,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_EMINC),(PSZ)szTemp);

            ltoa(pFM->lInternalLeading,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_INTERNALLEADING),(PSZ)szTemp);

            ltoa(pFM->lExternalLeading,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_EXTERNALLEADING),(PSZ)szTemp);

            ltoa(pFM->lMaxBaselineExt,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_MAXBASELINEEXT),(PSZ)szTemp);

            ltoa(pFM->lEmHeight,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_EMHEIGHT),(PSZ)szTemp);

            ltoa(pFM->lAveCharWidth,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_AVECHARWIDTH),(PSZ)szTemp);

            ltoa(pFM->lMaxCharInc,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_MAXCHARINC),(PSZ)szTemp);

            ltoa((pFM->lMaxBaselineExt - pFM->lInternalLeading),(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_POINT_SIZE),(PSZ)szTemp);

            ltoa(pFM->usCodePage,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_CODEPAGE),(PSZ)szTemp);

            sprintf(szTemp,"%s%s%s%s%s",
                szSelection[(pFM->fsSelection & FM_SEL_BOLD)       ? 1 : 0],
                szSelection[(pFM->fsSelection & FM_SEL_ITALIC)     ? 2 : 0],
                szSelection[(pFM->fsSelection & FM_SEL_STRIKEOUT)  ? 3 : 0],
                szSelection[(pFM->fsSelection & FM_SEL_UNDERSCORE) ? 4 : 0],
                szSelection[(pFM->fsSelection & FM_SEL_OUTLINE)    ? 5 : 0]);
            WinSetWindowText(WinWindowFromID(hWnd,DID_SELECTION),(PSZ)szTemp);

            ltoa(pFM->sKerningPairs,(PSZ)szTemp,10);
            WinSetWindowText(WinWindowFromID(hWnd,DID_KERNPAIRS),(PSZ)szTemp);

            sprintf(szTemp,"%2i,%2i",LOBYTE(pFM->sFamilyClass),HIBYTE(pFM->sFamilyClass));
            WinSetWindowText(WinWindowFromID(hWnd,DID_FAMILYCLASS),(PSZ)szTemp);

            sprintf(szTemp,"%s",
                szSpacing[(pFM->fsType & FM_TYPE_FIXED) ? 1 : 0]);
            WinSetWindowText(WinWindowFromID(hWnd,DID_TYPE),(PSZ)szTemp);

            sprintf(szTemp,"%s%s",
                szFormat[(pFM->fsDefn & FM_DEFN_GENERIC) ? 1 : 0],
                szType[(pFM->fsDefn & FM_DEFN_OUTLINE) ? 1 : 0]);
            WinSetWindowText(WinWindowFromID(hWnd,DID_DEFINITION),(PSZ)szTemp);

            WinSetWindowText(WinWindowFromID(hWnd,DID_CAPABILITIES),
                (PSZ)szQuality[(pFM->fsCapabilities >> 5)]);

            WinSetWindowText(WinWindowFromID(hWnd,DID_WEIGHTCLASS),
                (PSZ)szWeights[pFM->usWeightClass - 1]);

            WinSetWindowText(WinWindowFromID(hWnd,DID_WIDTHCLASS),
                (PSZ)szWidths[pFM->usWidthClass - 1]);
            break;

        case WM_ERASEBACKGROUND:
            WinFillRect((HPS)LONGFROMMP(mp1),PVOIDFROMMP(mp2),CLR_PALEGRAY);
            mReturn = MRFROMLONG(0L);
            break;

        case WM_COMMAND:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    if (!bHandled)
        mReturn = WinDefDlgProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}



MRESULT EXPENTRY ShearWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

\*-----------------------------------------------------------------*/
{
    MRESULT  mReturn  = 0;
    BOOL     bHandled = TRUE;
    CHAR     szTemp[20];
    int      iAngle;
    double   dSine;
    double   dCosine;
    SHORT    sPos;

    switch(msg)
    {
        case WM_INITDLG:
            CurrentCB.usSet = DISPLAY_FONT;
            charbox_from_pointsize (MAKEFIXED(0,DEFAULT_SIZE),&CurrentCB.sizfxCell);
            DefaultCharBox = CurrentCB.sizfxCell;
            ltoa (HIUSHORT(CurrentCB.sizfxCell.cy),szTemp,10);
            WinSetDlgItemText(hWnd,DID_CHAR_HEIGHT,szTemp);
            ltoa (HIUSHORT(CurrentCB.sizfxCell.cx),szTemp,10);
            WinSetDlgItemText(hWnd,DID_CHAR_WIDTH,szTemp);
            init_sliders_n_spinners(hWnd);
            break;

        case WM_ERASEBACKGROUND:
            WinFillRect((HPS)LONGFROMMP(mp1),PVOIDFROMMP(mp2),CLR_PALEGRAY);
            mReturn = MRFROMLONG(0L);
            break;

        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1))
            {
                case DID_RESET_SIZE:
                    ltoa (HIUSHORT(DefaultCharBox.cy),szTemp,10);
                    WinSetDlgItemText(hWnd,DID_CHAR_HEIGHT,szTemp);
                    ltoa (HIUSHORT(DefaultCharBox.cx),szTemp,10);
                    WinSetDlgItemText(hWnd,DID_CHAR_WIDTH,szTemp);
                    WinSendMsg(hWnd,WM_COMMAND,MPFROM2SHORT(DID_APPLY,0),0);
                    break;
                case DID_RESET_ANGLE:
                    WinSendDlgItemMsg(hWnd,DID_ROT_ANGLE,SPBM_SETCURRENTVALUE,MPFROMLONG(0),0);
                    WinSendMsg(hWnd,WM_COMMAND,MPFROM2SHORT(DID_APPLY,0),0);
                    break;
                case DID_RESET_SHEAR:
                    WinSendDlgItemMsg(hWnd,DID_SHEAR_ANGLE,SPBM_SETCURRENTVALUE,MPFROMLONG(90),0);
                    WinSendMsg(hWnd,WM_COMMAND,MPFROM2SHORT(DID_APPLY,0),0);
                    break;
                case DID_APPLY:
                    WinQueryDlgItemText(hWnd,DID_CHAR_WIDTH,20,szTemp);
                    CurrentCB.sizfxCell.cx = MAKEFIXED(atol(szTemp),0);
                    WinQueryDlgItemText(hWnd,DID_CHAR_HEIGHT,20,szTemp);
                    CurrentCB.sizfxCell.cy = MAKEFIXED(atol(szTemp),0);
                    WinQueryDlgItemText(hWnd,DID_ANGLEX,20,szTemp);
                    CurrentCB.ptlAngle.x = atol(szTemp);
                    WinQueryDlgItemText(hWnd,DID_ANGLEY,20,szTemp);
                    CurrentCB.ptlAngle.y = atol(szTemp);
                    WinInvalidateRect (hWndClient,0,FALSE);
                    break;
            }
            break;

        case WM_CONTROL:
            switch (LOUSHORT(mp1))
            {
                case DID_USER_TEXT:
                    if (SHORT2FROMMP(mp1) == EN_CHANGE)
                    {
                        WinQueryWindowText(HWNDFROMMP(mp2),MAX_STR,szUserStr);
                        WinInvalidateRect (hWndClient,0,FALSE);
                    }
                    break;  
                case DID_CHAR_EXTRA:
                    sPos = (SHORT)WinSendDlgItemMsg(hWnd,DID_CHAR_EXTRA,
                       SLM_QUERYSLIDERINFO,
                       MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
                       0);
                    CharExtra = MAKEFIXED(sPos - 10,0);
                    WinInvalidateRect (hWndClient,0,FALSE);
                    WinUpdateWindow(hWndClient);
                    break;
                case DID_BREAK_EXTRA:
                    sPos = (SHORT)WinSendDlgItemMsg(hWnd,DID_BREAK_EXTRA,
                       SLM_QUERYSLIDERINFO,
                       MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
                       0);
                    BreakExtra = MAKEFIXED(sPos - 10,0);
                    WinInvalidateRect (hWndClient,0,FALSE);
                    WinUpdateWindow(hWndClient);
                    break;
                case DID_ROT_ANGLE:
                    if (SHORT2FROMMP(mp1) == SPBN_CHANGE)
                    {
                        if (WinSendMsg((HWND)mp2,SPBM_QUERYVALUE,&iAngle,
                            MPFROM2SHORT(0,SPBQ_UPDATEIFVALID)))
                        {
                            dRotSine = sin(TWO_PIE * iAngle / 360.0);
                            dSine   = 200.0 * dRotSine;
                            dRotCosine = cos(TWO_PIE * iAngle / 360.0);
                            dCosine = 200.0 * dRotCosine;
                            CurrentCB.ptlAngle.x = ((LONG)dCosine * lHorzRes)/(LONG)lVertRes;
                            CurrentCB.ptlAngle.y = (LONG) dSine;
                            ltoa (CurrentCB.ptlAngle.x,szTemp,10);
                            WinSetDlgItemText(hWnd,DID_ANGLEX,szTemp);
                            ltoa (CurrentCB.ptlAngle.y,szTemp,10);
                            WinSetDlgItemText(hWnd,DID_ANGLEY,szTemp);
                            WinInvalidateRect (hWndClient,0,FALSE);
                            WinUpdateWindow(hWndClient);
                        }
                    }
                    break;
                case DID_SHEAR_ANGLE:
                    if (SHORT2FROMMP(mp1) == SPBN_CHANGE)
                    {
                        if (WinSendMsg((HWND)mp2,SPBM_QUERYVALUE,&iAngle,
                            MPFROM2SHORT(0,SPBQ_UPDATEIFVALID)))
                        {
                            dSine   = 200.0 * sin(TWO_PIE * iAngle / 360.0);
                            dCosine = 200.0 * cos(TWO_PIE * iAngle / 360.0);
                            CurrentCB.ptlShear.x  = ((LONG)dCosine * lHorzRes)/(LONG)lVertRes;
                            CurrentCB.ptlShear.y  = (LONG) dSine;
                            ltoa (CurrentCB.ptlShear.x,szTemp,10);
                            WinSetDlgItemText(hWnd,DID_SHEARX,szTemp);
                            ltoa (CurrentCB.ptlShear.y,szTemp,10);
                            WinSetDlgItemText(hWnd,DID_SHEARY,szTemp);
                            WinInvalidateRect (hWndClient,0,FALSE);
                            WinUpdateWindow(hWndClient);
                        }
                    }
                    break;
            }
            break;

        default:
            bHandled = FALSE;
            break;
    }
    if (!bHandled)
        mReturn = WinDefDlgProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}



BOOL APIENTRY init_sliders_n_spinners(HWND hWnd)
/*-----------------------------------------------------------------*\

   This function will initalize the tick marks and text for the 
   slider controls which control character and break character 
   spacing. A label is placed ever two tick marks. The spinners for
   rotation and shear angle are intialized by setting the range from
   0 to 360 and the default angles are set 

\*-----------------------------------------------------------------*/
{
   SHORT  sIndex;
   CHAR   szTemp[4];
   HWND   hCharSliderWnd  = WinWindowFromID(hWnd,DID_CHAR_EXTRA);
   HWND   hBreakSliderWnd = WinWindowFromID(hWnd,DID_BREAK_EXTRA);
   HWND   hRotAngleWnd    = WinWindowFromID(hWnd,DID_ROT_ANGLE);
   HWND   hShearAngleWnd  = WinWindowFromID(hWnd,DID_SHEAR_ANGLE);

   for (sIndex = -10;sIndex <= 10 ;sIndex ++)
   {
      itoa(sIndex,szTemp,10);
      WinSendMsg(hCharSliderWnd, SLM_SETTICKSIZE,
               MPFROM2SHORT(sIndex + 10, 3), 0);
      if ((sIndex & 0x01) == 0)
          WinSendMsg(hCharSliderWnd, SLM_SETSCALETEXT,
                   MPFROMSHORT(sIndex + 10), MPFROMP(szTemp));

      WinSendMsg(hBreakSliderWnd, SLM_SETTICKSIZE,
               MPFROM2SHORT(sIndex + 10, 3), 0);

      if ((sIndex & 0x01) == 0)
          WinSendMsg(hBreakSliderWnd, SLM_SETSCALETEXT,
                    MPFROMSHORT(sIndex + 10), MPFROMP(szTemp));
   }       
   WinSendMsg (hCharSliderWnd,SLM_SETSLIDERINFO,
       MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
       MPFROMSHORT(10));
   WinSendMsg (hBreakSliderWnd,SLM_SETSLIDERINFO,
       MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
       MPFROMSHORT(10));

   WinSendMsg(hRotAngleWnd,SPBM_SETLIMITS,(MPARAM)360,(MPARAM)0);
   WinSendMsg(hRotAngleWnd,SPBM_SETTEXTLIMIT,MPFROMSHORT(3),0);
   WinSendMsg(hRotAngleWnd,SPBM_SETCURRENTVALUE,MPFROMLONG(0),0);

   WinSendMsg(hShearAngleWnd,SPBM_SETLIMITS,(MPARAM)360,(MPARAM)0);
   WinSendMsg(hShearAngleWnd,SPBM_SETTEXTLIMIT,MPFROMSHORT(3),0);
   WinSendMsg(hShearAngleWnd,SPBM_SETCURRENTVALUE,MPFROMLONG(90),0);

   return (TRUE);
}                                 



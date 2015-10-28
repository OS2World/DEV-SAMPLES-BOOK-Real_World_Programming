/* --------------------------------------------------------------------
                        Printing sample program
                              Chapter 11

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_WIN
#define INCL_GPI
#define INCL_DEV
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_WINERRORS
#define INCL_DOSPROCESS
#include <os2.h>
#include <string.h>
#include <stdio.h>
#include "prnt.h"
#include "dialog.h"
#include "..\common\about.h"

#define NERR_BuffTooSmall 2123

MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY PrintInfoDlgProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY PrintStatusProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY PrintDlgProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY SelectPrinterDlgProc (HWND,ULONG,MPARAM,MPARAM);

HAB    hab;
HWND   hWndFrame, 
       hWndClient;
HWND   hHintWnd;
HWND   hMenu;
CHAR   szTitle[64];
CHAR   szBuff1[MAX_BUFF];
CHAR   szFontFace[] = "8.Helv";
CHAR   szPM_Q_STD[] = "PM_Q_STD";
PVOID  pMem;
int    nSysFontHeight;
LONG   lColorWorkSpace;
USHORT usDraw = IDM_DISP_BLANK;
CHAR   szPriority[3][10] =
{ 
  "Low",
  "Standard",
  "High"
};

LONG lPriority[3] =
{
   1,50,99
};

/* Array of pointers to priority strings (for spinner control) */
PVOID pszPriorities[3] = 
{
    &szPriority[0],
    &szPriority[1],
    &szPriority[2]
};


DEVICEINFO CurrentDI;

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                            FCF_ICON | FCF_MENU;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, ClientWndProc, CS_SIZEREDRAW, 0);
    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    DosAllocMem((PPVOID)&pMem,0x1000,PAG_READ | PAG_WRITE);
    DosSubSet(pMem,DOSSUB_INIT | DOSSUB_SPARSE_OBJ,0x8000);

    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, WS_VISIBLE,
        &flFrameFlags, szClientClass, szTitle, 0, 0, ID_APPNAME, &hWndClient);

    hMenu  = WinWindowFromID (hWndFrame,FID_MENU);
    create_status_line(hWndClient);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    DosFreeMem(pMem);
    return (0);
}


VOID APIENTRY center_window (HWND hWnd)
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


BOOL APIENTRY create_info_DC(PPRQINFO3 pPrq3,PDEVICEINFO pDI)
/*-----------------------------------------------------------------*\

   This function will intialize the pDI DEVICEINFO structure based
   on the queue pointed to by pPrq3. If pDI contains an existing 
   entry its' info DC it is destroyed and any form info is freed.
   A new info DC is created and the get_page_metrics function is used
   to initialize size information in the stucture.

\*-----------------------------------------------------------------*/
{
    PDRIVDATA    pDD;
    PPRDINFO3    pPrd3;
    DEVOPENSTRUC DevOpenData;
    SIZEL        sizel;
    ULONG        ulNeeded = 0;
    int          i;
    CHAR         szDriverName[MAX_DEVICENAME];
    CHAR         szDeviceName[MAX_DEVICENAME];

    /* Free existing info DCs and PSs */
    if (pDI->hInfoPS)
    {
        GpiAssociate(pDI->hInfoPS,0);
        GpiDestroyPS(pDI->hInfoPS);
        pDI->hInfoPS = 0;
        DevCloseDC(pDI->hInfoDC);
        pDI->hInfoDC = 0;
    }  
    if (pDI->pHCInfo)
    {
        DosSubFree(pMem,pDI->pHCInfo,(pDI->lSizeHC * sizeof(HCINFO)));
        pDI->pHCInfo = 0;
        pDI->lSizeHC = 0;
    }

    if ((i = strcspn(pPrq3->pszDriverName,".")) != 0)
    {
        strncpy((PSZ)szDriverName,(PSZ)pPrq3->pszDriverName,i);
        szDriverName[i] = '\0';
        strcpy((PSZ)szDeviceName,(PSZ)&pPrq3->pszDriverName[i + 1]);
    }

    if (SplQueryDevice(0,pPrq3->pszPrinters,3,0,0,&ulNeeded) != NERR_BuffTooSmall)
        return(FALSE);

    if (DosSubAlloc(pMem,(PPVOID)&pPrd3,ulNeeded))
        return (FALSE);
    if (SplQueryDevice(0,pPrq3->pszPrinters,3,(PVOID)pPrd3,ulNeeded,
            &ulNeeded) == 0)
    {
        if (pDI->pDrivData)
            pDD = pDI->pDrivData;
        else
            pDD = pPrq3->pDriverData;

        strcpy(pDI->szQueueDesc,pPrq3->pszComment);
        strcpy(pDI->szDriverName,pPrq3->pszDriverName);
        strcpy(pDI->szQueueName,pPrq3->pszName);

        DevOpenData.pszLogAddress = pPrd3->pszLogAddr;
        DevOpenData.pszDriverName = szDriverName;
        DevOpenData.pdriv         = pDD;
        DevOpenData.pszDataType   = (PSZ)szPM_Q_STD;


        if ((pDI->hInfoDC = DevOpenDC (hab,OD_INFO,"*",4,
                                (PVOID)&DevOpenData,0)) != 0)
        {
            sizel.cx = 0;
            sizel.cy = 0;
            if(!(pDI->hInfoPS = GpiCreatePS (hab,pDI->hInfoDC,&sizel,GPIA_ASSOC | GPIT_MICRO | PU_LOMETRIC)))
            {
                DevCloseDC(pDI->hInfoDC);
                pDI->hInfoDC = 0;
            }  
            else
            {
                get_page_metrics(pDI);
            }
        }
    }
    else
       post_error(hWndClient,ID_ERR_FAIL_DEV_QUERY);

    DosSubFree(pMem,pPrd3,ulNeeded);
    return((BOOL)pDI->hInfoPS);
}


BOOL APIENTRY create_print_DC(PDEVICEINFO pDI)
/*-----------------------------------------------------------------*\

   This function will create an OD_QUEUED device context based on the
   contents of the pDI parameter. If successful a presentation space
   is created and the DEVESC_STARTDOC escape is issued. The presentation
   space is then reset from indexed to RGB mode.

\*-----------------------------------------------------------------*/
{
    DEVOPENSTRUC  DevOpenStr;
    CHAR          szQueueProcParams[9];
    CHAR          szSpoolerParams[9];
    CHAR          szDriverName[MAX_DEVICENAME];
    int           i;

    memset((PVOID)&DevOpenStr, 0, sizeof(DevOpenStr));

    /* Set priority in spooler params */
    sprintf((PSZ)szSpoolerParams,(PSZ)"PRTY=%d",pDI->lPriority);
    /* Set number of copies in the queue processor params */
    sprintf((PSZ)szQueueProcParams,(PSZ)"COP=%d",pDI->lCopies);

    WinLoadString(hab,0,IDS_JOB_TITLE,sizeof(szBuff1),szBuff1);

    if ((i = strcspn(pDI->szDriverName,".")) != 0)
    {
        strncpy((PSZ)szDriverName,(PSZ)&pDI->szDriverName,i);
        szDriverName[i] = '\0';
    }

    DevOpenStr.pszLogAddress      = pDI->szQueueName;
    DevOpenStr.pszDriverName      = szDriverName;
    DevOpenStr.pdriv              = pDI->pDrivData;
    DevOpenStr.pszDataType        = (PSZ)szPM_Q_STD;
    DevOpenStr.pszComment         = (PSZ)szBuff1;
    DevOpenStr.pszQueueProcParams = (PSZ)szQueueProcParams;
    DevOpenStr.pszSpoolerParams   = (PSZ)szSpoolerParams;

    if ((pDI->hPrintDC = DevOpenDC(hab,OD_QUEUED,"*",9,
                             (PVOID)&DevOpenStr,(HDC)0)) != 0)
    {
        if (!(pDI->hPrintPS = GpiCreatePS(hab,pDI->hPrintDC,(PSIZEL)&pDI->PageSizel,
                                         PU_LOMETRIC | GPIA_ASSOC )))
        {
            DevCloseDC(pDI->hPrintDC);
            pDI->hPrintDC = 0;
        }  
        else
        {
            DevEscape(pDI->hPrintDC,DEVESC_STARTDOC,strlen(szBuff1),szBuff1,0,0);
            GpiCreateLogColorTable (CurrentDI.hPrintPS,LCOL_RESET,LCOLF_RGB,0,0,NULL);
        }
    }
    else
        WinGetLastError(hab);
    return((BOOL)pDI->hPrintPS);
}


void APIENTRY create_status_line(HWND hWndClient)
/*-----------------------------------------------------------------*\

   This function creates a static window positioned across the bottom
   of the client window which will display hint text when menus are 
   browsed.

\*-----------------------------------------------------------------*/
{
    FONTMETRICS fontMetrics;
    HPS    hPS  = WinGetPS(hWndClient);
    RECTL  Rectl;
    LONG   lColor;

    WinQueryWindowRect(hWndClient,(PRECTL)&Rectl);
    GpiQueryFontMetrics(hPS,sizeof(FONTMETRICS),(PFONTMETRICS)&fontMetrics);
    nSysFontHeight = fontMetrics.lMaxBaselineExt + 4;
    WinSendMsg(hWndClient,WM_SIZE,0,MPFROM2SHORT(Rectl.xRight,Rectl.yTop));

    hHintWnd = WinCreateWindow (hWndClient,WC_STATIC,"",
            WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER,
            0,0,Rectl.xRight,nSysFontHeight,
            hWndClient,HWND_TOP,1,NULL,NULL);
    lColor = CLR_PALEGRAY;
    WinSetPresParam(hHintWnd,PP_BACKGROUNDCOLORINDEX,sizeof(PLONG),&lColor);
    lColor = CLR_BLACK;
    WinSetPresParam(hHintWnd,PP_FOREGROUNDCOLORINDEX,sizeof(PLONG),&lColor);
    WinSetPresParam(hHintWnd,PP_FONTNAMESIZE,strlen(szFontFace)+1,(PVOID)szFontFace);

    WinReleasePS(hPS);
}


void APIENTRY display_hint(SHORT sMenuID)
/*-----------------------------------------------------------------*\

   This function will load a menu hint string based on the supplied
   id and display that string in the status window.

\*-----------------------------------------------------------------*/
{
    CHAR  szBuff2[MAX_BUFF];

    if (sMenuID == -1)
        szBuff2[0] = '\0';
    else
    {
        WinLoadString(hab,0,sMenuID,sizeof(szBuff2),szBuff2);
        if ( (sMenuID == IDM_PRINT) || (sMenuID == IDM_PRINT_THREAD) )
            strcat(szBuff2,CurrentDI.szQueueDesc);
    }
    WinSetWindowText(hHintWnd,szBuff2);
}

void APIENTRY draw_page(HPS hPS)
/*-----------------------------------------------------------------*\
\*-----------------------------------------------------------------*/
{
    AREABUNDLE  ab;
    LINEBUNDLE  lb;
    ARCPARAMS   arcparms; 
    POINTL      ptl;

    switch (usDraw)
    {
        case IDM_DISP_BLANK:
            break;
        case IDM_DISP_BOX:
            ab.lColor = RGB_RED;
            GpiSetAttrs (hPS,PRIM_AREA,ABB_COLOR,0,&ab);
            lb.lColor = RGB_GREEN;
            lb.usType = LINETYPE_SOLID;
            GpiSetAttrs(hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);

            ptl.x = ptl.y = 0;
            GpiMove(hPS,&ptl);
            ptl.x = CurrentDI.PageSizel.cx;
            ptl.y = CurrentDI.PageSizel.cy;
            GpiBox (hPS,DRO_OUTLINEFILL,&ptl,0,0);
            break;
        case IDM_DISP_ELLIPSE:
            ab.lColor = RGB_GREEN;
            GpiSetAttrs (hPS,PRIM_AREA,ABB_COLOR,0,&ab);
            lb.lColor = RGB_BLACK;
            lb.usType = LINETYPE_SOLID;
            GpiSetAttrs(hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);

            /* define the bounding box for the arc */
            arcparms.lP = CurrentDI.PageSizel.cx / 2L;
            arcparms.lQ = CurrentDI.PageSizel.cy / 2L;
            arcparms.lR = 0L;
            arcparms.lS = 0L;
            GpiSetArcParams(hPS, (PARCPARAMS)&arcparms);

            /* Calculate the center point of bulb */
            ptl.x = arcparms.lP;   
            ptl.y = arcparms.lQ + 1;
            GpiMove (hPS,&ptl);
            GpiFullArc(hPS,DRO_OUTLINEFILL,MAKEFIXED(1,0));
            break;
        default:
            break;
    }
}


void APIENTRY draw_page_background(HPS hPS)
/*-----------------------------------------------------------------*\

   This function draws the "print page" in the client area. The
   physical page is drawn as a white rectangle with a black border
   and the printable area is drawn as a blue dashed outline.

\*-----------------------------------------------------------------*/
{
    AREABUNDLE  ab;
    LINEBUNDLE  lb;
    POINTL      ptl;

    /* Draw the physical page white with a black border */
    ab.lColor = RGB_WHITE;
    GpiSetAttrs(hPS,PRIM_AREA,ABB_COLOR,0,(PBUNDLE)&ab);
    lb.lColor = RGB_BLACK;
    lb.usType = LINETYPE_SOLID;
    GpiSetAttrs(hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);

    ptl.x = PAGE_OFFSET;
    ptl.y = (CurrentDI.cy - nSysFontHeight) - PAGE_OFFSET;
    GpiConvert(hPS,CVTC_DEVICE,CVTC_WORLD,1,(PPOINTL)&ptl);
    GpiMove(hPS,(PPOINTL)&ptl);
    ptl.x = CurrentDI.PhyPageSizel.cx + ptl.x;
    ptl.y = ptl.y - CurrentDI.PhyPageSizel.cy;
    GpiBox(hPS,DRO_OUTLINEFILL,(PPOINTL)&ptl,0,0);

    /* Outline the addressable portion of the page with a blue dashed line */
    lb.lColor = RGB_BLUE;
    lb.usType = LINETYPE_SHORTDASH;
    GpiSetAttrs(hPS,PRIM_LINE,LBB_COLOR | LBB_TYPE,0,(PBUNDLE)&lb);

    ptl.x = PAGE_OFFSET;
    ptl.y = (CurrentDI.cy - nSysFontHeight) - PAGE_OFFSET;
    GpiConvert(CurrentDI.hClientPS,CVTC_DEVICE,CVTC_WORLD,1,(PPOINTL)&ptl);
    ptl.x += CurrentDI.PrintOffsetSizel.cx;
    ptl.y -= CurrentDI.PrintOffsetSizel.cy;
    GpiMove(hPS,(PPOINTL)&ptl);
    ptl.x = CurrentDI.PageSizel.cx + ptl.x; 
    ptl.y = ptl.y - CurrentDI.PageSizel.cy; 
    GpiBox(hPS,DRO_OUTLINE,(PPOINTL)&ptl,0,0);
}


PPRQINFO3 APIENTRY enum_print_queues(HWND hWnd,HWND hWndLBox,PULONG pulSize,
                                     PULONG pulCount,PULONG pulCurrent)
/*-----------------------------------------------------------------*\

   This function will enumerate the installed print queues and add
   their description to hWndLBox.  The function returns a pointer
   to an array of PPRQINFO3 structures if successful. The number of
   entries in the array and the total size of the allocated memory 
   is returned in the pulCount and pulSize parameters respectfully.
   The index of the current application default queue is returned in
   the pulCurrent parameter.

\*-----------------------------------------------------------------*/
{
    ULONG     cReturned, cTotal, ulSize;
    int       i;
    PSZ       pName;
    PPRQINFO3 pQueueInfo = 0;

    ulSize = 0;
    SplEnumQueue(0,3,0,0L,(PULONG)&cReturned,(PULONG)&cTotal,
        (PULONG)&ulSize,0);    

    if (pulCount)
        *pulCount = cTotal;

    if (!cTotal)
       post_error(hWnd,ID_NOQUEUE_ERROR);
    else if (!DosSubAlloc(pMem,(PPVOID)&pQueueInfo,ulSize))
    {
        /* Set the size in the return parameter */
        *pulSize = ulSize;

        SplEnumQueue(0,3,pQueueInfo,ulSize,(PULONG)&cReturned,(PULONG)&cTotal,
            (PULONG)&ulSize,0);    

        WinEnableWindowUpdate(hWndLBox,FALSE);
        WinSendMsg (hWndLBox,LM_DELETEALL,0,0);
        for (i = 0;i < (int)cTotal; i++)
        {
            /* If the queue description is a NULL string then use the
               the queue name so the dialog won't have a blank entry */
            if (*pQueueInfo[i].pszComment)
                pName = pQueueInfo[i].pszComment;
            else
                pName = pQueueInfo[i].pszName;
            WinSendMsg (hWndLBox,LM_INSERTITEM,(MPARAM)LIT_END,(MPARAM)pName);
            if (strcmp(CurrentDI.szQueueDesc,pName) == 0)
                /* This is the current application default */
                *pulCurrent = i;
        }
        WinEnableWindowUpdate(hWndLBox,TRUE);
    }
    return(pQueueInfo);
}




VOID APIENTRY get_job_properties(PPRQINFO3 pQueueInfo,PDDINFO pDDInfo,
                                 ULONG ulOption)
/*-----------------------------------------------------------------*\

   This function will query or post job properties for the queue
   identified by the pQueueInfo parameter. ulOptioh should be set
   to DPDM_QUERYJOBPROP or DPDM_POSTJOBPROP.
   
\*-----------------------------------------------------------------*/
{
   int   i;
   PSZ   pszTemp;
   CHAR  szDriverName[MAX_DEVICENAME];
   CHAR  szDeviceName[MAX_DEVICENAME];

   if ((i = strcspn(pQueueInfo->pszDriverName,".")) != 0)
   {
      strncpy((PSZ)szDriverName,(PSZ)pQueueInfo->pszDriverName,i);
      szDriverName[i] = '\0';
      strcpy((PSZ)szDeviceName,(PSZ)&pQueueInfo->pszDriverName[i + 1]);
   }
   else
   {
      strcpy((PSZ)szDriverName, pQueueInfo->pszDriverName);
      *szDeviceName = '\0';
   }

   pszTemp = (PSZ)strchr(pQueueInfo->pszPrinters, ',');
   if ( pszTemp )
   {
      *pszTemp = '\0' ;
   }

   if (pDDInfo->pDrivData == 0)
   {
       /* Allocate memory for the driver data */
       pDDInfo->lSizeDD = DevPostDeviceModes(hab,0,szDriverName,szDeviceName,
           pQueueInfo->pszPrinters,ulOption);
       DosSubAlloc(pMem,(PPVOID)&pDDInfo->pDrivData,pDDInfo->lSizeDD);
       pDDInfo->pDrivData->cb = sizeof(DRIVDATA);
       pDDInfo->pDrivData->lVersion = 0;
       strcpy(pDDInfo->pDrivData->szDeviceName,szDeviceName);
   }
   DevPostDeviceModes(hab,pDDInfo->pDrivData,szDriverName,szDeviceName,
       pQueueInfo->pszPrinters,ulOption);
}  



VOID APIENTRY get_page_metrics(PDEVICEINFO pDI)
/*-----------------------------------------------------------------*\

   This function will initialize the physical and printable page
   sizes for the information DC in the pDI parameter.

\*-----------------------------------------------------------------*/
{
    POINTL   PtlRes;
    PHCINFO  pHCInfo;
    int      i;

    GpiQueryPS(pDI->hInfoPS,(PSIZEL)&pDI->PageSizel);
    pDI->lSizeHC = DevQueryHardcopyCaps(pDI->hInfoDC,0,0,0);
    if ( (pDI->lSizeHC > 0) &&
         (DosSubAlloc(pMem,(PPVOID)&pDI->pHCInfo,pDI->lSizeHC * sizeof(HCINFO)) == 0) )
    { 
        DevQueryHardcopyCaps(pDI->hInfoDC,0,pDI->lSizeHC,pDI->pHCInfo);
        pHCInfo = pDI->pHCInfo;
        for (i = 0; i < pDI->lSizeHC; i++)
        {
            if (pHCInfo[i].flAttributes & HCAPS_CURRENT)
            {
                /* Calculate the physical page size */
                DevQueryCaps (pDI->hInfoDC,CAPS_HORIZONTAL_RESOLUTION,1L,
                     (PLONG) &PtlRes.x);
                DevQueryCaps (pDI->hInfoDC,CAPS_VERTICAL_RESOLUTION,1L,
                    (PLONG) &PtlRes.y);
                /* Convert from millimeters to pixels */
                pDI->PhyPageSizel.cx = (PtlRes.x * pHCInfo[i].cx) / 1000;
                pDI->PhyPageSizel.cy = (PtlRes.y * pHCInfo[i].cy) / 1000;
                /* Convert from device to world coordinate space */
                GpiConvert(pDI->hInfoPS,CVTC_DEVICE,CVTC_WORLD,1,(PPOINTL)&pDI->PhyPageSizel);

                pDI->PrintOffsetSizel.cx = (PtlRes.x * pHCInfo[i].xLeftClip) / 1000;
                pDI->PrintOffsetSizel.cy = (PtlRes.y * pHCInfo[i].yBottomClip) / 1000;
                /* Convert from device to world coordinate space */
                GpiConvert(pDI->hInfoPS,CVTC_DEVICE,CVTC_WORLD,1,(PPOINTL)&pDI->PrintOffsetSizel);
                break;
            }
        }
    }
    else
    {
        pDI->pHCInfo = 0;    
        pDI->lSizeHC = 0;
    }
}


HPS APIENTRY init_app(HWND hWnd)
/*-----------------------------------------------------------------*\

   This function is called on program startup to intialize the CurrentDI
   variable based on the system default queue. A PS is created for the
   client window, the default viewing matrix is modified, and the PS
   is changed from indexed to RGB mode.

\*-----------------------------------------------------------------*/
{
    PPRQINFO3 pPrq3;
    ULONG     ulSize;
    char      szTemp[MAX_BUFF];
    PSZ       pStr;
    ULONG     ulNeeded;
    MATRIXLF  matlfDefView; 

    memset(&CurrentDI,0,sizeof(DEVICEINFO));
    CurrentDI.lCopies   = 1;
    CurrentDI.lPriority = 50;

    /* Query the default printer */
    ulSize = PrfQueryProfileString(HINI_PROFILE,"PM_SPOOLER","QUEUE",
        0,szTemp,MAX_BUFF);

    if (ulSize)
    {
       pStr = (PSZ)strchr(szTemp,';');
       *pStr = 0;
       SplQueryQueue(0,szTemp,3,0,0,&ulNeeded);
       if (ulNeeded && !(DosSubAlloc(pMem,(PPVOID)&pPrq3,ulNeeded)))
       {
           SplQueryQueue(0,szTemp,3,pPrq3,ulNeeded,&ulNeeded);
           get_job_properties(pPrq3,(PDDINFO)&CurrentDI.lSizeDD,DPDM_QUERYJOBPROP); 
           create_info_DC(pPrq3,&CurrentDI);
           DosSubFree(pMem,pPrq3,ulNeeded);

           CurrentDI.hClientDC = WinOpenWindowDC(hWnd);
           CurrentDI.hClientPS = GpiCreatePS(hab,CurrentDI.hClientDC,
               (PSIZEL)&CurrentDI.ClientSizel,
               PU_LOMETRIC | GPIF_LONG | GPIT_NORMAL | GPIA_ASSOC);

           /* Scale the page to 50% */
           GpiQueryDefaultViewMatrix(CurrentDI.hClientPS,9,&matlfDefView );
           matlfDefView.fxM11 = MAKEFIXED(0,32768);
           matlfDefView.fxM22 = MAKEFIXED(0,32768);
           matlfDefView.lM31  = PAGE_OFFSET;
           matlfDefView.lM32  = 0;
           GpiSetDefaultViewMatrix(CurrentDI.hClientPS,9,&matlfDefView,TRANSFORM_REPLACE);

           /* Create an RGB color table */
           GpiCreateLogColorTable (CurrentDI.hClientPS,LCOL_RESET,LCOLF_RGB,0,0,NULL);
           lColorWorkSpace = WinQuerySysColor(HWND_DESKTOP,SYSCLR_APPWORKSPACE,0);
       }
    }

    if (!CurrentDI.hInfoDC)
    {
         /* Disable print menus */
         HWND hMenu = WinWindowFromID(WinQueryWindow(hWnd,QW_PARENT),FID_MENU);
         WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_CHOOSE_PRINTER,TRUE),
             MPFROM2SHORT(MIA_DISABLED,FALSE));
         WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_PRINTER_INFO,TRUE),
             MPFROM2SHORT(MIA_DISABLED,FALSE));
         WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_PRINT,TRUE),
             MPFROM2SHORT(MIA_DISABLED,FALSE));
         /* Message box */
         post_error(hWnd,ID_ERR_NODEF_PRINTERS);
    }
    return (CurrentDI.hInfoDC);
}


USHORT APIENTRY post_error(HWND hWnd,USHORT usErrorID)
/*-----------------------------------------------------------------*\

   This is a utility function which will post a message box with
   a string loaded from the resource file.

\*-----------------------------------------------------------------*/
{
    WinLoadString (hab,0,usErrorID,MAX_BUFF,szBuff1);
    return (WinMessageBox(HWND_DESKTOP,hWnd,(PSZ)szBuff1,
        (PSZ)"Error",0,MB_OK | MB_ICONHAND | MB_APPLMODAL));
}


void APIENTRY process_command(HWND hWnd,MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   All WM_COMMAND messages received by the client window will be
   processed here.  

\*-----------------------------------------------------------------*/
{
    switch(SHORT1FROMMP(mp1))
    {
        case IDM_DISP_BLANK:
        case IDM_DISP_BOX:
        case IDM_DISP_ELLIPSE:
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usDraw,TRUE),
                MPFROM2SHORT(MIA_CHECKED,FALSE));
            usDraw = SHORT1FROMMP(mp1);
            WinSendMsg (hMenu,MM_SETITEMATTR,MPFROM2SHORT(usDraw,TRUE),
                MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
            WinInvalidateRect(hWndClient,0,FALSE);
            break;
        case IDM_ABOUT:
            DisplayAbout (hWnd, szTitle);
            break;
        case IDM_CHOOSE_PRINTER:
            /* Display the queue selection dialog */
            WinDlgBox(HWND_DESKTOP,hWndFrame,(PFNWP)SelectPrinterDlgProc,0,SELECT_PRINTER_DLG,(PVOID)0);
            break;
        case IDM_PRINTER_INFO:
            /* Display the queue information dialog */
            WinDlgBox(HWND_DESKTOP,hWndFrame,(PFNWP)PrintInfoDlgProc,0,QUEUE_INFO_DLG,(PVOID)0);
            break;
        case IDM_PRINT:
            /* Confirm the print operation and print */
            if (WinDlgBox(HWND_DESKTOP,hWndClient,PrintDlgProc,0,PRINT_DLG,(PVOID)0) == DID_OK)
            {
                WinDlgBox(HWND_DESKTOP,hWndClient,PrintStatusProc,0,PRINT_STATUS_DLG,
                    (PVOID)&mp1);
            }
            break;
        case IDM_PRINT_THREAD:
            /* Confirm the print operation and print in a thread */
            if (WinDlgBox(HWND_DESKTOP,hWndClient,PrintDlgProc,0,PRINT_DLG,(PVOID)0) == DID_OK)
            {
                WinLoadDlg(HWND_DESKTOP,hWndClient,PrintStatusProc,0,PRINT_STATUS_DLG,
                    (PVOID)&mp1);
            }
            break;
        default:
            break;
    }
    return;
    mp2;
    hWnd;
}

#ifdef __IBMC__
#pragma linkage(process_print, system)
#endif

void APIENTRY process_print(HWND hPrintStatusDlg)
/*-----------------------------------------------------------------*\

   This function perfoms the actual printing. A copy of the current
   device settings are copied to an allocated memory block and a
   print DC is created. If successful the printing is done and the
   job is completed. The print status dialog is updated via posted
   WM_PRINT_STATUS messages.

\*-----------------------------------------------------------------*/
{
    PDEVICEINFO pDI;

    if (DosSubAlloc(pMem,(PPVOID)&pDI,sizeof(DEVICEINFO)) == 0) 
    {
       *pDI = CurrentDI;

       if (create_print_DC(pDI))
       {
           WinPostMsg(hPrintStatusDlg,WM_PRINT_STATUS,(MPARAM)JOB_STARTED,0);
           draw_page(pDI->hPrintPS);
           DevEscape(pDI->hPrintPS,DEVESC_ENDDOC,0,0,0,0);
           WinPostMsg(hPrintStatusDlg,WM_PRINT_STATUS,(MPARAM)JOB_FINISHED,0);
     
           GpiAssociate(pDI->hPrintPS,0);
           GpiDestroyPS(pDI->hPrintPS);
           DevCloseDC(pDI->hPrintDC);
       }
       else
           WinPostMsg(hPrintStatusDlg,WM_PRINT_STATUS,(MPARAM)JOB_FAILED,0);
       DosSubFree(pMem,pDI,sizeof(DEVICEINFO));
    }
}    


void APIENTRY process_size(MPARAM mp1,MPARAM mp2)
{
    POINTL   ptl;
    MATRIXLF matlfDefView; 

    CurrentDI.cx = SHORT1FROMMP(mp2);
    CurrentDI.cy = SHORT2FROMMP(mp2);
    WinSetWindowPos (hHintWnd,HWND_TOP,0,0,SHORT1FROMMP(mp2),
        nSysFontHeight,SWP_SIZE | SWP_MOVE);

    /* Reset the default view matrix before converting client size */
    ptl.x = PAGE_OFFSET;
    ptl.y = (CurrentDI.cy - nSysFontHeight) - PAGE_OFFSET + 1;
    GpiQueryDefaultViewMatrix(CurrentDI.hClientPS,9,&matlfDefView );
    matlfDefView.lM31  = 0;
    matlfDefView.lM32  = 0;
    GpiSetDefaultViewMatrix(CurrentDI.hClientPS,9,&matlfDefView,
         TRANSFORM_REPLACE);
    GpiConvert(CurrentDI.hClientPS,CVTC_DEVICE,CVTC_WORLD,1,(PPOINTL)&ptl);
    ptl.y = (ptl.y - CurrentDI.PhyPageSizel.cy) + 
        CurrentDI.PrintOffsetSizel.cy;
    ptl.x = ptl.x + CurrentDI.PrintOffsetSizel.cx;
    matlfDefView.lM31 = (LONG)((float)ptl.x * 0.5);
    matlfDefView.lM32 = (LONG)((float)ptl.y * 0.5);
    GpiSetDefaultViewMatrix(CurrentDI.hClientPS,9,&matlfDefView,
        TRANSFORM_REPLACE);
    return;
    mp1;
}

MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

   This is the application callback which handles the messages 
   necessary to maintain the client window.

\*-----------------------------------------------------------------*/
{
    HPS      hPS;
    RECTL    Rectl;
    TID      tid;
    BOOL     bHandled = TRUE;
    MRESULT  mReturn  = 0;

    switch (msg)
    {
        case WM_CREATE:
            hPS = init_app(hWnd);
            break;
        case WM_COMMAND:
            process_command(hWnd,mp1,mp2);
            break;
        case WM_PAINT:
            hPS = WinBeginPaint (hWnd,CurrentDI.hClientPS,(PRECTL)&Rectl);
            WinFillRect(hPS,(PRECTL)&Rectl,lColorWorkSpace);
            draw_page_background(hPS);
            draw_page(hPS);
            WinEndPaint (hPS);
            break;
        case WM_ERASEBACKGROUND:
            mReturn = MRFROMLONG(1L);
            break;
        case WM_MENUSELECT:
            display_hint(SHORT1FROMMP(mp1));
            bHandled = FALSE;
            break;
        case WM_SIZE:
            process_size(mp1,mp2);
            break;
        case WM_START_PRINT:
            if (SHORT1FROMMP(mp2) == IDM_PRINT)
                process_print((HWND)mp1);
            else
                DosCreateThread(&tid,(PFNTHREAD)process_print,(ULONG)mp1,
                   0,0x2000);
            break;
        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}


MRESULT EXPENTRY PrintDlgProc(HWND hWndDlg,ULONG ulMessage,MPARAM mp1,
                              MPARAM mp2)
/*-----------------------------------------------------------------*\

   This function will display the print confirmation dialog box. It
   allows the user to verify the print destination and set the job
   priority and print copies.

\*-----------------------------------------------------------------*/
{
    HWND  hWnd;
    ULONG ulSelect;

    switch (ulMessage) 
    {
        case WM_INITDLG:
            center_window(hWndDlg);
            hWnd = WinWindowFromID(hWndDlg,DID_PRINTER);
            WinSetWindowText(hWnd,CurrentDI.szQueueDesc);
            WinSendMsg(hWnd,EM_SETREADONLY,(MPARAM)TRUE,0);

            hWnd = WinWindowFromID(hWndDlg,DID_DOCUMENT);
            WinLoadString(hab,0,IDS_JOB_TITLE,sizeof(szBuff1),szBuff1);
            WinSetWindowText(hWnd,szBuff1);
            WinSendMsg(hWnd,EM_SETREADONLY,(MPARAM)TRUE,0);

            /* Set the limits to the valid range for a queue processor */
            hWnd = WinWindowFromID(hWndDlg,DID_COPIES);
            WinSendMsg(hWnd,SPBM_SETLIMITS,(MPARAM)999,(MPARAM)1);
            WinSendMsg(hWnd,SPBM_SETCURRENTVALUE,MPFROMLONG(CurrentDI.lCopies),0);
            WinSetFocus(HWND_DESKTOP,hWnd);

            /* Initialize the priority spinner control */
            hWnd = WinWindowFromID(hWndDlg,DID_PRIORITY);
            WinSendMsg (hWnd,SPBM_SETTEXTLIMIT, (MPARAM)9L,(MPARAM)0L);
            WinSendMsg (hWnd,SPBM_SETARRAY, (MPARAM)pszPriorities,(MPARAM)3);
            WinSendMsg (hWnd,SPBM_SETCURRENTVALUE,(MPARAM)1,(MPARAM)0L);
            return ((MRESULT)TRUE);
        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1)) 
            {
                case DID_OK:
                    hWnd = WinWindowFromID(hWndDlg,DID_COPIES);
                    WinSendDlgItemMsg(hWndDlg,DID_COPIES,SPBM_QUERYVALUE,
                        &CurrentDI.lCopies,MPFROM2SHORT(0,SPBQ_UPDATEIFVALID));
                    /* Query selection index from spinner */
                    if(WinSendDlgItemMsg (hWndDlg, DID_PRIORITY,
                        SPBM_QUERYVALUE, (MPARAM)&ulSelect, 0L))
                        CurrentDI.lPriority = lPriority[ulSelect];
                    else
                        CurrentDI.lPriority = 50;
                    WinDismissDlg(hWndDlg,DID_OK);
                    return 0;
            }
            break;
        case WM_CONTROL:
            switch (SHORT1FROMMP(mp1))
            {
                case DID_PRIORITY:
                    if (HIUSHORT(mp1) == SPBN_ENDSPIN)
                    {
                        /* Query selection index from spinner */
                        if(WinSendDlgItemMsg (hWndDlg, DID_PRIORITY,
                            SPBM_QUERYVALUE, (MPARAM)&ulSelect, 0L))
                            CurrentDI.lPriority = lPriority[ulSelect];
                        else
                            CurrentDI.lPriority = 50;
                    }
                    break;
            }
            break;
    }
    return (WinDefDlgProc(hWndDlg,ulMessage,mp1,mp2));
}



MRESULT EXPENTRY PrintStatusProc(HWND hWndDlg,ULONG ulMessage,
                                 MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   This function is the callback for the print job status dialog.
   When the WM_INITDLG message is received a WM_START_PRINT message
   is posted to the client window requesting the print job to be started.
   When the WM_PRINT_STATUS message is received the text in the status
   window is updated accordingly.

\*-----------------------------------------------------------------*/
{
    HWND  hWnd;
    LONG  lColor;

    switch (ulMessage) 
    {
        case WM_INITDLG:
            center_window(hWndDlg);
            hWnd = WinWindowFromID(hWndDlg,DID_OK);
            WinShowWindow(hWnd,FALSE);

            hWnd = WinWindowFromID(hWndDlg,DID_JOBNAME);
            WinLoadString(hab,0,IDS_JOB_TITLE,sizeof(szBuff1),szBuff1);
            WinSetWindowText(hWnd,szBuff1);

            hWnd = WinWindowFromID(hWndDlg,DID_PRINTER);
            WinSetWindowText(hWnd,CurrentDI.szQueueDesc);

            hWnd = WinWindowFromID(hWndDlg,DID_STATUS);
            WinSetWindowText(hWnd,"Initializing Printer");

            WinPostMsg(hWndClient,WM_START_PRINT,(MPARAM)hWndDlg,
               (MPARAM)*(PULONG)mp2);
            break;
        case WM_PRINT_STATUS:
            hWnd = WinWindowFromID(hWndDlg,DID_STATUS);
            switch(SHORT1FROMMP(mp1))
            {
                case JOB_STARTED:
                    WinSetWindowText(hWnd,"Starting Print Job");
                    break;
                case JOB_CANCELED:
                    WinSetWindowText(hWnd,"Print Job Cancelled");
                    WinPostMsg(hWndDlg,WM_COMMAND,(MPARAM)DID_OK,0);
                    break;
                case JOB_FAILED:
                    lColor = CLR_RED;
                    WinSetPresParam(hWnd,PP_FOREGROUNDCOLORINDEX,sizeof(PLONG),&lColor);
                    WinSetWindowText(hWnd,"Print Job Failed");
                    WinAlarm(HWND_DESKTOP,WA_ERROR);
                    /* Switch the cancel button to an ok button */
                    hWnd = WinWindowFromID(hWndDlg,DID_OK);
                    WinShowWindow(hWnd,TRUE);
                    hWnd = WinWindowFromID(hWndDlg,DID_CANCEL);
                    WinShowWindow(hWnd,FALSE);
                    break;
                case JOB_FINISHED:
                    WinSetWindowText(hWnd,"Print Job Finished");
                    WinPostMsg(hWndDlg,WM_COMMAND,(MPARAM)DID_OK,0);
                    break;
            }
            break;
        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1)) 
            {
                case DID_CANCEL:
                    hWnd = WinWindowFromID(hWndDlg,DID_STATUS);
                    WinSetWindowText(hWnd,"Canceling Print Job");
                    WinPostMsg(hWndClient,WM_ABORT_PRINT,0,0);
                    return 0;
                case DID_OK:
                    WinDismissDlg(hWndDlg,DID_OK);
                    return 0;
            }
            break;
    }
    return (WinDefDlgProc(hWndDlg,ulMessage,mp1,mp2));
}


MRESULT EXPENTRY SelectPrinterDlgProc(HWND hWndDlg,ULONG ulMessage,
                                      MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   This function is the callback for the printer selection dialog.
   It will fill a list box with installed queue entries and post
   job properties when requested. If OK is selected the current
   queue selection along with it's last job properties are set in
   the CurrentDI variable.

\*-----------------------------------------------------------------*/
{
    static  PPRQINFO3 pQueueInfo;
    static  ULONG     ulSize = 0;
    static  HWND      hWndLBox;
    static  BOOL      bRedraw = FALSE;
    static  PDDINFO   pDDInfo;
    static  ULONG     ulCount;
    RECTL   Rectl;
    int     i;
    PDDINFO pDDI;
    SHORT   sSelection;
    ULONG   ulCurrent;

    switch (ulMessage) 
    {
        case WM_INITDLG:
            center_window(hWndDlg);
            hWndLBox = WinWindowFromID(hWndDlg,DID_QUEUE_NAMES);
            if (!(pQueueInfo = enum_print_queues(hWndDlg,hWndLBox,
                (PULONG)&ulSize,(PULONG)&ulCount,(PULONG)&ulCurrent)))
               WinDismissDlg(hWndDlg,DID_OK);
            else if (!DosSubAlloc(pMem,(PPVOID)&pDDInfo,ulCount * sizeof(DDINFO)))
            {
               memset(pDDInfo,0,ulCount * sizeof(DDINFO));

               /* Preload the entry for the current printer with the 
                  saved values from CurrentDI */
               (pDDInfo + ulCurrent)->pDrivData = CurrentDI.pDrivData;
               (pDDInfo + ulCurrent)->lSizeDD = CurrentDI.lSizeDD;

               /* Select the current entry in the list box */
               WinSendMsg (hWndLBox,LM_SELECTITEM,MPFROMSHORT(ulCurrent),MPFROMSHORT(TRUE));
            }
            break;
        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1)) 
            {
                case DID_OK:
                    sSelection = (USHORT)WinSendMsg (hWndLBox,LM_QUERYSELECTION,
                       MPFROMSHORT(LIT_FIRST),0);
                    if (!(pDDInfo + sSelection)->pDrivData)
                    {
                        get_job_properties(&pQueueInfo[sSelection],
                            &pDDInfo[sSelection],DPDM_QUERYJOBPROP); 
                        bRedraw = TRUE;
                    }
                    if ((pDDInfo + sSelection)->pDrivData)
                    {
                        /* If we are changing application default printers
                           free up current driver data and replace it with
                           the driver data for the selected printer. */
                        if ( CurrentDI.pDrivData && 
                            (CurrentDI.pDrivData != (pDDInfo + sSelection)->pDrivData) )
                        {
                            DosSubFree(pMem,CurrentDI.pDrivData,CurrentDI.lSizeDD);
                            CurrentDI.pDrivData = (pDDInfo + sSelection)->pDrivData;
                            CurrentDI.lSizeDD = (pDDInfo + sSelection)->lSizeDD;
                        }
                        (pDDInfo + sSelection)->pDrivData = 0;
                        (pDDInfo + sSelection)->lSizeDD = 0;
                    }
                    create_info_DC(&pQueueInfo[sSelection],(PDEVICEINFO)&CurrentDI); 
                    WinDismissDlg(hWndDlg,DID_OK);

                    pDDI = pDDInfo;
                    for (i = 0; i < (int)ulCount;i++)
                    {
                        if ((pDDI + i)->pDrivData)
                            DosSubFree(pMem,(pDDI + i)->pDrivData,
                                (pDDI + i)->lSizeDD);
                    }
                    DosSubFree(pMem,pDDInfo,ulCount * sizeof(DDINFO));
                    DosSubFree(pMem,pQueueInfo,ulSize);
                    if (bRedraw)
                    {
                        WinQueryWindowRect(hWndClient,(PRECTL)&Rectl);
                        WinSendMsg(hWndClient,WM_SIZE,0,
                            MPFROM2SHORT(Rectl.xRight,Rectl.yTop));
                        WinInvalidateRect(hWndClient,0,TRUE);
                    }
                    break;
                case DID_CANCEL:
                    WinDismissDlg(hWndDlg,DID_CANCEL);
                    pDDI = pDDInfo;
                    for (i = 0; i < (int)ulCount;i++)
                    {
                        /* Free up any allocated driver data except the
                           current application default  */
                        if ( (pDDI + i)->pDrivData &&
                             (CurrentDI.pDrivData != (pDDI + i)->pDrivData) )
                            DosSubFree(pMem,(pDDI + i)->pDrivData,
                                (pDDI + i)->lSizeDD);
                    }
                    DosSubFree(pMem,pDDInfo,ulCount * sizeof(DDINFO));
                    DosSubFree(pMem,pQueueInfo,ulSize);
                    break;
                case DID_JOBPROPS:
                    sSelection = (USHORT)WinSendMsg (hWndLBox,LM_QUERYSELECTION,
                       MPFROMSHORT(LIT_FIRST),0);
                    if (sSelection != LIT_NONE)
                    {
                        get_job_properties(&pQueueInfo[sSelection],
                            &pDDInfo[sSelection],DPDM_POSTJOBPROP); 
                        bRedraw = TRUE;
                    }
                    return 0;
            }
            break;
        default:
            break;
    }
    return (WinDefDlgProc(hWndDlg,ulMessage,mp1,mp2));
}

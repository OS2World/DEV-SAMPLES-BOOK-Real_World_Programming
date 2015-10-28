/* -----------------------------------------------------------------
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
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "prnt.h"
#include "dialog.h"


extern HAB   hab;
extern CHAR  szTemp[];
extern PVOID pMem;
CHAR   szYes[] = "Yes";
CHAR   szNo[]  = "No";


void APIENTRY free_info_DCs(PDEVICEINFO pDI,ULONG ulCount)
/*-----------------------------------------------------------------*\

   This function will walk the array of DEVICEINFO structures detroying
   nformation device contexts and freeing memory allocated for form
   arrays.

\*-----------------------------------------------------------------*/
{
    int i;
    for (i = 0; i < (int)ulCount;i++)
    {
        if ((pDI + i)->hInfoPS)
        {
            GpiAssociate((pDI + i)->hInfoPS,0);
            GpiDestroyPS((pDI + i)->hInfoPS);
            pDI->hInfoPS = 0;
            DevCloseDC((pDI + i)->hInfoDC);
            (pDI + i)->hInfoDC = 0;
        }  
        if ((pDI + i)->pHCInfo)
        {
            DosSubFree(pMem,(pDI + i)->pHCInfo,((pDI + i)->lSizeHC * 
                sizeof(HCINFO)));
            (pDI + i)->pHCInfo = 0;
            (pDI + i)->lSizeHC = 0;
        }
    }
}


void APIENTRY query_device_info(HWND hWndDlg,PPRQINFO3 pPrq3,
                                PDEVICEINFO pDI)
/*-----------------------------------------------------------------*\

   This function will update fields of the queue info dialog based
   on the supplied pPrq3 and pDI structures.

\*-----------------------------------------------------------------*/
{
    HWND    hWnd;
    int     index;
    PHCINFO pHCI;
    LONG    lDevCaps[CAPS_RASTER_CAPS + 1];
    CHAR    szTemp[MAX_BUFF];

    /* Update the device and default printer info */
    hWnd = WinWindowFromID(hWndDlg,DID_QUEUE_NAME);
    WinSetWindowText(hWnd,pPrq3->pszName);

    hWnd = WinWindowFromID(hWndDlg,DID_DEFAULT_DEVICE);
    WinSetWindowText(hWnd,pPrq3->pszDriverName);

    hWnd = WinWindowFromID(hWndDlg,DID_PRINTERS);
    WinSetWindowText(hWnd,pPrq3->pszPrinters);

    /* Update the queue priority */
    hWnd = WinWindowFromID(hWndDlg,DID_PRIORITY);
    index = ((pPrq3->uPriority > 7) ? ID_PRIORITY_HIGH :
             (pPrq3->uPriority > 3) ? ID_PRIORITY_DEF : 
             ID_PRIORITY_MIN);
    WinLoadString (hab,0,index,MAX_BUFF,szTemp);
    WinSetWindowText(hWnd,szTemp);
            
    /* Update the queue status */
    hWnd = WinWindowFromID(hWndDlg,DID_STATUS);
    index = ((pPrq3->fsStatus == PRQ3_PAUSED) ? ID_STATUS_HELD :
             (pPrq3->fsStatus == PRQ_PENDING) ? ID_STATUS_PEND : 
              ID_STATUS_NORM);
    WinLoadString (hab,0,index,MAX_BUFF,szTemp);
    WinSetWindowText(hWnd,szTemp);

    /* Update the number of jobs in the queue */
    hWnd = WinWindowFromID(hWndDlg,DID_NUMBER_JOBS);
    itoa(pPrq3->cJobs,szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    /* Update the queue start and end time */
    hWnd = WinWindowFromID(hWndDlg,DID_START_TIME);
    index = (pPrq3->uStartTime > 779) ? pPrq3->uStartTime - 720 : 
        pPrq3->uStartTime;
    sprintf(szTemp,"%2i:%02i %s",index / 60,
        pPrq3->uStartTime % 60,(pPrq3->uStartTime > 779) ? "PM" : 
        "AM");
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_END_TIME);
    index = (pPrq3->uUntilTime > 779) ? pPrq3->uUntilTime - 720 : 
       pPrq3->uUntilTime;
    sprintf(szTemp,"%2i:%02i %s",index / 60,
       pPrq3->uUntilTime % 60,(pPrq3->uUntilTime > 779) ? "PM" : 
       "AM" );
    WinSetWindowText(hWnd,szTemp);

    /* Query the forms available */
    if ((!pDI->pHCInfo) &&
        (pDI->lSizeHC = DevQueryHardcopyCaps(pDI->hInfoDC,0,0,0)) > 0)
    { 
        /* Only query the forms the first time */
        if (DosSubAlloc(pMem,(PPVOID)&pDI->pHCInfo,(pDI->lSizeHC * 
               sizeof(HCINFO))))
            pDI->pHCInfo = 0;
        else
            DevQueryHardcopyCaps(pDI->hInfoDC,0,pDI->lSizeHC,
                pDI->pHCInfo);
    }
    if (pDI->pHCInfo)
    {
        hWnd = WinWindowFromID(hWndDlg,DID_FORMS);
        WinEnableWindowUpdate(hWnd,FALSE);
        pHCI = pDI->pHCInfo;
        WinSendMsg (hWnd,LM_DELETEALL,0,0);
        for (index = 0; index < pDI->lSizeHC; index++)
        {
            WinSendMsg (hWnd,LM_INSERTITEM,(MPARAM)LIT_END,
                (pHCI + index)->szFormname);
        }
        WinEnableWindowUpdate(hWnd,TRUE);
        WinSendMsg (hWnd,LM_SELECTITEM,(MPARAM)0,MPFROMSHORT(TRUE));
    }
    /* Query all of the desired metrics */
    DevQueryCaps (pDI->hInfoDC,CAPS_FAMILY,CAPS_RASTER_CAPS,
        (PLONG)&lDevCaps);

    /* Update the device technology */
    hWnd = WinWindowFromID(hWndDlg,DID_TECHNOLOGY);
    WinLoadString (hab,0,lDevCaps[CAPS_TECHNOLOGY] + ID_TECH_BASE,
       MAX_BUFF,szTemp);
    WinSetWindowText(hWnd,szTemp);

    /* Update the device resolution */
    hWnd = WinWindowFromID(hWndDlg,DID_CAPS_WIDTH);
    itoa(lDevCaps[CAPS_WIDTH],szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_CAPS_HEIGHT);
    itoa(lDevCaps[CAPS_HEIGHT],szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_HORZ_RES);
    itoa(lDevCaps[CAPS_HORIZONTAL_RESOLUTION],szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_VERT_RES);
    itoa(lDevCaps[CAPS_VERTICAL_RESOLUTION],szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_BITBLT);
    WinSetWindowText(hWnd,
       (lDevCaps[CAPS_RASTER_CAPS] & CAPS_RASTER_BITBLT) ?
       szYes : szNo);

    hWnd = WinWindowFromID(hWndDlg,DID_BANDING);
    WinSetWindowText(hWnd,
       (lDevCaps[CAPS_RASTER_CAPS] & CAPS_RASTER_BANDING) ?
       szYes : szNo);

    hWnd = WinWindowFromID(hWndDlg,DID_SETPEL);
    WinSetWindowText(hWnd,
       (lDevCaps[CAPS_RASTER_CAPS] & CAPS_RASTER_SET_PEL) ?
       szYes : szNo);

    hWnd = WinWindowFromID(hWndDlg,DID_RASTER_FONT);
    WinSetWindowText(hWnd,
       (lDevCaps[CAPS_RASTER_CAPS] & CAPS_RASTER_FONTS) ?
       szYes : szNo);

    hWnd = WinWindowFromID(hWndDlg,DID_KERNING);
    WinSetWindowText(hWnd,
       (lDevCaps[CAPS_ADDITIONAL_GRAPHICS] & CAPS_GRAPHICS_KERNING_SUPPORT) ?
       szYes : szNo);

    hWnd = WinWindowFromID(hWndDlg,DID_PALETTE);
    WinSetWindowText(hWnd,
       (lDevCaps[CAPS_ADDITIONAL_GRAPHICS] & CAPS_PALETTE_MANAGER) ?
       szYes : szNo);
}


void APIENTRY query_form_info(HWND hWndDlg,PHCINFO pHCInfo)
/*-----------------------------------------------------------------*\

   This function will update the form fields in the queue info dialog
   based on the form pointed to by pHCInfo.

\*-----------------------------------------------------------------*/
{
    HWND    hWnd;
    int     i;
    CHAR    szTemp[MAX_BUFF];


    hWnd = WinWindowFromID(hWndDlg,DID_FORM_CX);
    itoa(pHCInfo->cx,szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_FORM_CY);
    itoa(pHCInfo->cy,szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_LEFT_CLIP);
    itoa(pHCInfo->xLeftClip,szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_BOTTOM_CLIP);
    itoa(pHCInfo->yBottomClip,szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_RIGHT_CLIP);
    itoa(pHCInfo->xRightClip,szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_TOP_CLIP);
    itoa(pHCInfo->yTopClip,szTemp,10);
    WinSetWindowText(hWnd,szTemp);

    hWnd = WinWindowFromID(hWndDlg,DID_FORM_ATTR);
    if (pHCInfo->flAttributes & HCAPS_CURRENT)
    {
        WinLoadString (hab,0,ID_CAPS_CURRENT,MAX_BUFF,szTemp);
        if (pHCInfo->flAttributes & HCAPS_SELECTABLE)
        {
            strcat(szTemp,", ");
            i = strlen (szTemp); 
            WinLoadString (hab,0,ID_CAPS_SELECTABLE,MAX_BUFF - i,&szTemp[i]);
        }
    }
    else if (pHCInfo->flAttributes & HCAPS_SELECTABLE)
        WinLoadString (hab,0,ID_CAPS_SELECTABLE,MAX_BUFF,szTemp);
    else
        WinLoadString (hab,0,ID_CAPS_NONE,MAX_BUFF,szTemp);
    WinSetWindowText(hWnd,szTemp);
}


MRESULT EXPENTRY PrintInfoDlgProc(HWND hWndDlg,ULONG ulMessage,
                                  MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   This function is the callback for the queue and printer information
   dialog. It will fill a list box with installed queue entries and 
   post job properties when requested. In addition the forms available 
   on the selected queue are queried and added to a second listbox.
   Details from the PRQINFO3, HCINFO, and various device capablilities 
   of the selected queue are displayed in the dialog.
   
\*-----------------------------------------------------------------*/
{
   static    PPRQINFO3   pQueueInfo;
   static    PDEVICEINFO pDI = 0;
   static    ULONG       ulSize   = 0;
   static    ULONG       ulDISize = 0;
   static    ULONG       ulCount  = 0;
   static    DDINFO      DDInfo;
   SHORT     sQueueSelect;
   SHORT     sFormSelect;
   HWND      hWndLBox;
   ULONG     ulCurrent;
   PHCINFO   pHCI;

   switch (ulMessage) 
   {
      case WM_INITDLG:
         center_window(hWndDlg);
         hWndLBox = WinWindowFromID(hWndDlg,DID_QUEUE_NAMES);
         if (!(pQueueInfo = enum_print_queues(hWndDlg,hWndLBox,
               (PULONG)&ulSize,(PULONG)&ulCount,(PULONG)&ulCurrent)))
             WinDismissDlg(hWndDlg,DID_OK);
         else
         {
             DDInfo.lSizeDD = 0;
             DDInfo.pDrivData = 0;
             ulDISize = ulCount * sizeof(DEVICEINFO);
             DosSubAlloc(pMem,(PPVOID)&pDI,ulDISize);
             memset(pDI,0,ulDISize);
             WinSendMsg (hWndLBox,LM_SELECTITEM,MPFROMSHORT(ulCurrent),
                 MPFROMSHORT(TRUE));
         }
         break;
      case WM_COMMAND:
         switch (SHORT1FROMMP(mp1)) 
         {
            case DID_CANCEL:
            case DID_OK:
               free_info_DCs(pDI,ulCount);
               DosSubFree(pMem,pQueueInfo,ulSize);
               DosSubFree(pMem,pDI,ulDISize);
               WinDismissDlg(hWndDlg,DID_OK);
               return 0;
            case DID_JOBPROPS:
            case DID_UPDATE:
               hWndLBox = WinWindowFromID(hWndDlg,DID_QUEUE_NAMES);
               sQueueSelect = (USHORT)WinSendMsg (hWndLBox,
                  LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST),0);
               if (sQueueSelect != LIT_NONE)
               {
                   if (SHORT1FROMMP(mp1) == DID_JOBPROPS)
                   {
                       get_job_properties(&pQueueInfo[sQueueSelect],
                           (PDDINFO)&DDInfo,DPDM_POSTJOBPROP); 
                       if (DDInfo.pDrivData)
                       {  
                           DosSubFree(pMem,DDInfo.pDrivData,
                               DDInfo.lSizeDD);
                           DDInfo.lSizeDD = 0;
                           DDInfo.pDrivData = 0;
                       }
                   }
                   free_info_DCs(pDI,ulCount);
                   DosSubFree(pMem,pQueueInfo,ulSize);
                   DosSubFree(pMem,pDI,ulDISize);
                   pDI = 0;
                   hWndLBox = WinWindowFromID(hWndDlg,DID_QUEUE_NAMES);
                   if (!(pQueueInfo = enum_print_queues(hWndDlg,
                         hWndLBox,(PULONG)&ulSize,(PULONG)&ulCount,
                         (PULONG)&ulCurrent)))
                       WinDismissDlg(hWndDlg,DID_OK);
                   else
                   {
                       ulDISize = ulCount * sizeof(DEVICEINFO);
                       DosSubAlloc(pMem,(PPVOID)&pDI,ulDISize);
                       memset(pDI,0,ulDISize);
                       WinSendMsg (hWndLBox,LM_SELECTITEM,
                          MPFROMSHORT(sQueueSelect),MPFROMSHORT(TRUE));
                   }
               }
               return 0;
            default:
               break;
         }
         break;

      case WM_CONTROL:
         if (SHORT2FROMMP(mp1) == LN_SELECT)
         {
             switch (LOUSHORT(mp1))
             {
                case DID_QUEUE_NAMES:
                   hWndLBox = WinWindowFromID(hWndDlg,DID_QUEUE_NAMES);
                   sQueueSelect = (USHORT)WinSendMsg (hWndLBox,
                      LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST),0);
                   if (sQueueSelect != LIT_NONE)
                   {
                       if (!(pDI + sQueueSelect)->hInfoDC)
                           create_info_DC(
                               (PPRQINFO3)&pQueueInfo[sQueueSelect],
                               (PDEVICEINFO)&pDI[sQueueSelect]);
                       query_device_info(hWndDlg,
                               (PPRQINFO3)&pQueueInfo[sQueueSelect],
                               (PDEVICEINFO)&pDI[sQueueSelect]);
                   }
                   break;
                case DID_FORMS:
                   hWndLBox = WinWindowFromID(hWndDlg,DID_QUEUE_NAMES);
                   sQueueSelect = (USHORT)WinSendMsg (hWndLBox,
                      LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST),0);

                   if ((pHCI = pDI[sQueueSelect].pHCInfo) != 0)
                   {
                       hWndLBox = WinWindowFromID(hWndDlg,DID_FORMS);
                       sFormSelect = (USHORT)WinSendMsg (hWndLBox,
                          LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST),0);
                       if (sFormSelect != LIT_NONE)
                           query_form_info(hWndDlg,&(pHCI[sFormSelect]));
                   }
                   break;
             }
         }
         break;
   }
   return (WinDefDlgProc(hWndDlg,ulMessage,mp1,mp2));
}

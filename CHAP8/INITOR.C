/* --------------------------------------------------------------------
                            Initor Program
                              Chapter 8

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */


#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "initor.h"
#include "..\common\about.h"

MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY DataWndProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY AddDlgProc(HWND,ULONG,MPARAM,MPARAM);

HAB    hab;
HWND   hWndFrame, 
       hWndClient,
       hAppLBox,
       hKeyLBox,
       hAppTitle,
       hKeyTitle,
       hDataTitle,
       hDataFrame,
       hDataWnd,
       hVScroll;
PVOID  pMem;
PVOID  pKeyData  = NULL;
ULONG  ulKeySize = 0;
HWND   hButton[NUM_BUTTONS];
USHORT usSelectItem[NUM_BUTTONS] =
{
    IDM_ADD_KEY,
    IDM_DISPLAY_USERSYS,
    IDM_DELETE_KEY,
    IDM_SAVE_CHANGES,
    0
};
HWND hPopupMenu [NUM_BUTTONS] =
{
    0,
    0,
    0,
    0,
    0
};
HINI hCurrentIni = 0;

CHAR   szTitle[MAX_TITLE];
CHAR   szKey[MAX_TITLE];
CHAR   szBuff1[MAX_BUFF];
CHAR   szBuff2[MAX_BUFF];
CHAR   szFontFace[]    = "10.System Monospaced";
CHAR   szDataClass[]   = "DATA";
CHAR   szClientClass[] = "CLIENT";
int    nFixedFontWidth  = 0;
int    nFixedFontHeight = 0;
int    nSysFontHeight;
int    iButtonWidth;
int    iButtonHeight;
int    iButtonY;


int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    RECTL Rectl;
    ULONG ulSize = sizeof(RECTL);
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                            FCF_NOBYTEALIGN;  
    int        iClientHeight;
    FRAMECDATA FrameData;

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    DosAllocMem(&pMem,8192,PAG_READ | PAG_WRITE);
    DosSubSet(pMem,DOSSUB_INIT | DOSSUB_SPARSE_OBJ,8192);

    WinRegisterClass (hab, szClientClass, ClientWndProc,CS_SIZEREDRAW, 0);
    FrameData.cb = sizeof(FRAMECDATA);
    FrameData.flCreateFlags = flFrameFlags;
    FrameData.hmodResources = 0;
    FrameData.idResources   = 0;
    WinLoadString (hab,0,ID_APPNAME,MAX_TITLE,szTitle);

    hWndFrame = WinCreateWindow (HWND_DESKTOP,WC_FRAME,szTitle,0L,
        0,0,0,0,HWND_DESKTOP,HWND_TOP,10,&FrameData,NULL);
    
    WinSendMsg (hWndFrame,WM_SETICON,
        (MPARAM)WinLoadPointer(HWND_DESKTOP,0,ID_APPNAME),0);
    hWndClient = WinCreateWindow (hWndFrame,szClientClass,szClientClass,
                               WS_VISIBLE | WS_CLIPCHILDREN,0,0,0,0,
                               hWndFrame,HWND_TOP,FID_CLIENT,NULL,NULL);

    iClientHeight = WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) -
                    WinQuerySysValue(HWND_DESKTOP,SV_CYICON);

    WinLoadString (hab,0,ID_INITOR,MAX_TITLE,szTitle);
    WinLoadString (hab,0,ID_LOCATION,MAX_TITLE,szKey);

    if (PrfQueryProfileData(HINI_USERPROFILE,szTitle,szKey,(PVOID)&Rectl,(PULONG)&ulSize))
        WinSetWindowPos (hWndFrame,HWND_TOP,
            Rectl.xLeft,Rectl.yBottom,Rectl.xRight,Rectl.yTop,
            SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER | SWP_SIZE | SWP_MOVE);
    else
        WinSetWindowPos (hWndFrame,HWND_TOP,0,
            WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) - iClientHeight,
            WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN),iClientHeight,
            SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER | SWP_SIZE | SWP_MOVE);

    create_controls(hWndClient);
    WinShowWindow (hAppLBox,TRUE);
    WinShowWindow (hKeyLBox,TRUE);
    enum_app_names(HINI_PROFILE,ID_HINIBOTH);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}

void APIENTRY change_ini_file(USHORT usCommand)
/*-----------------------------------------------------------------*\

   This function changes the currently viewed INI file base on the 
   usCommand value. If a system INI file is to be displayed the
   current INI file handle is set to one of the three system handles.
   If a private INI file is to be displayed the file open dialog is 
   posted allowing the user to select a file. If OK is selected and 
   the file can be opened as an INI file it will become the current
   INI file viewed. The enum_app_names function is called to update
   the display.

\*-----------------------------------------------------------------*/
{
    FILEDLG FileDlg;
    USHORT  usStringID = 0;
    HINI    hTempIni;
    HINI    hPrevIni = hCurrentIni;

    usSelectItem[1] = usCommand;
    switch(usCommand)
    {
        case IDM_DISPLAY_USER:
            hCurrentIni = HINI_USERPROFILE;
            usStringID  = ID_HINIUSER;
            break;
        case IDM_DISPLAY_SYS:
            hCurrentIni = HINI_SYSTEMPROFILE;
            usStringID  = ID_HINISYS;
            break;
        case IDM_DISPLAY_USERSYS:
            hCurrentIni = HINI_PROFILE;
            usStringID  = ID_HINIBOTH;
            break;
        case IDM_DISPLAY_PRIVATE:
            memset(&FileDlg,0,sizeof(FILEDLG));
            FileDlg.cbSize   = sizeof(FILEDLG);
            FileDlg.fl       = FDS_CENTER | FDS_OPEN_DIALOG;
            FileDlg.pszTitle = "Select Private INI File";
            strcpy (FileDlg.szFullFile,"*.INI");
            WinFileDlg(HWND_DESKTOP,hWndFrame,(PFILEDLG)&FileDlg);
            if ( (FileDlg.lReturn == DID_OK) &&
                 ((hTempIni = PrfOpenProfile(hab,FileDlg.szFullFile)) != 0) )
            {
                if ( (hCurrentIni != HINI_SYSTEMPROFILE) &&
                     (hCurrentIni != HINI_USERPROFILE)   &&
                     (hCurrentIni != HINI_PROFILE) )
                    PrfCloseProfile(hCurrentIni);
                hCurrentIni = hTempIni;
                WinSetWindowText(hWndFrame,(PSZ)FileDlg.szFullFile);
            }
            break;
    }
    if (hPrevIni != hCurrentIni)
        enum_app_names(hCurrentIni,usStringID);
}


void APIENTRY cleanup()
/*-----------------------------------------------------------------*\

   This function is called to free the memory allocated, popup menus
   and windows created for the application

\*-----------------------------------------------------------------*/
{
    int i;

    for (i = 0; i < NUM_MENUS; i++)
        WinDestroyWindow (hPopupMenu[i]);
     if (pKeyData)
     {
         DosSubFree(pMem,pKeyData,ulKeySize);
         pKeyData  = NULL;
         ulKeySize = 0;
     }
    DosFreeMem(pMem);
    WinDestroyWindow (hWndClient);
    WinDestroyWindow (hWndFrame);
}


void APIENTRY create_controls(HWND hWndClient)
/*-----------------------------------------------------------------*\

   This function will create the control windows which will display
   the application names, key names, and key data for INI files. The
   initial size is based on the size of their parent hWndClient. The
   popup menus used by INITOR are also loaded.

\*-----------------------------------------------------------------*/
{
    RECTL       Rectl;
    int         iButtonX;
    int         iColumn;
    int         i;
    FRAMECDATA  FrameData;
    FONTMETRICS fontMetrics;
    POINTL      Ptl[TXTBOX_COUNT + 1];
    LONG        lColor = CLR_PALEGRAY;
    HPS         hPS    = WinGetPS(hWndClient);
    ULONG       flFrameFlags = FCF_BORDER | FCF_NOBYTEALIGN | FCF_VERTSCROLL;  

    WinQueryWindowRect(hWndClient,(PRECTL)&Rectl);
    GpiQueryFontMetrics(hPS,sizeof(FONTMETRICS),(PFONTMETRICS)&fontMetrics);
    nSysFontHeight = fontMetrics.lMaxBaselineExt + 2;

    WinLoadString (hab,0,ID_APPLICATION,MAX_TITLE,szTitle);
    hAppTitle = WinCreateWindow (hWndClient,WC_STATIC,szTitle,
            WS_VISIBLE | SS_TEXT | DT_CENTER,
            0,Rectl.yTop - nSysFontHeight,
            Rectl.xRight / 2 - 1,nSysFontHeight,
            hWndClient,HWND_TOP,1,NULL,NULL);
    WinSetPresParam(hAppTitle,PP_BACKGROUNDCOLORINDEX,sizeof(PLONG),&lColor);

    WinLoadString (hab,0,ID_KEY,MAX_TITLE,szTitle);
    hKeyTitle = WinCreateWindow (hWndClient,WC_STATIC,szTitle,
            WS_VISIBLE | SS_TEXT | DT_CENTER,
            Rectl.xRight / 2,Rectl.yTop - nSysFontHeight,
            Rectl.xRight / 2 - 1,nSysFontHeight,
            hWndClient,HWND_TOP,1,NULL,NULL);
    WinSetPresParam(hKeyTitle,PP_BACKGROUNDCOLORINDEX,sizeof(PLONG),&lColor);

    WinLoadString (hab,0,ID_DATA,MAX_TITLE,szTitle);
    hDataTitle = WinCreateWindow (hWndClient,WC_STATIC,szTitle,
            SS_TEXT | DT_LEFT,
            0,Rectl.yTop / 2,
            Rectl.xRight,nSysFontHeight,
            hWndClient,HWND_TOP,1,NULL,NULL);

    WinSetPresParam(hDataTitle,PP_BACKGROUNDCOLORINDEX,sizeof(PLONG),&lColor);
    WinSetPresParam(hDataTitle,PP_FONTNAMESIZE,strlen(szFontFace)+1,(PVOID)szFontFace);

    hAppLBox = WinCreateWindow (hWndClient,WC_LISTBOX,NULL,
            LS_NOADJUSTPOS,
            1,Rectl.yTop / 2 + nSysFontHeight,
            Rectl.xRight / 2 - 2,Rectl.yTop / 2 - (nSysFontHeight * 2 + 1),
            hWndClient,HWND_TOP,APP_LBOX,NULL,NULL);

    hKeyLBox = WinCreateWindow (hWndClient,WC_LISTBOX,NULL,
            LS_NOADJUSTPOS,
            (Rectl.xRight / 2) + 1,Rectl.yTop / 2 + nSysFontHeight,
            Rectl.xRight / 2 - 2,Rectl.yTop / 2 - (nSysFontHeight  * 2 + 1),
            hWndClient,HWND_TOP,KEY_LBOX,NULL,NULL);

    // Load the longest string to calculate button width
    WinLoadString (hab,0,ID_DISPLAY,MAX_TITLE,szTitle);
    GpiQueryTextBox(hPS,strlen(szTitle),szTitle,TXTBOX_COUNT,Ptl);
    iButtonWidth  = Ptl[TXTBOX_CONCAT].x + (fontMetrics.lAveCharWidth * 2);
    iButtonHeight = nSysFontHeight + (nSysFontHeight / 3);
    iColumn       = Rectl.xRight / NUM_BUTTONS;
    iButtonX      = (iColumn - iButtonWidth) / 2;
    iButtonY      = nSysFontHeight / 2;
           
    WinRegisterClass (hab,szDataClass,DataWndProc,0,0);
    FrameData.cb = sizeof(FRAMECDATA);
    FrameData.flCreateFlags = flFrameFlags;
    FrameData.hmodResources = 0;
    FrameData.idResources   = 0;
    hDataFrame = WinCreateWindow (hWndClient,WC_FRAME,"",0L,
        0,0,0,0,hWndClient,HWND_TOP,11,&FrameData,NULL);

    hDataWnd = WinCreateWindow (hDataFrame,szDataClass,"",
            WS_VISIBLE,0,0,0,0,
            hDataFrame,HWND_TOP,FID_CLIENT,NULL,NULL);

    WinSetWindowPos (hDataFrame,HWND_TOP,
            1,iButtonHeight * 2,Rectl.xRight - 2,
            Rectl.yTop / 2 - 2 - (iButtonHeight * 2),
            SWP_SHOW | SWP_ZORDER | SWP_SIZE | SWP_MOVE);

    for (i = 0; i < NUM_BUTTONS; i++)
    {
        WinLoadString (hab,0,ID_FIRST_BUTTON + i,MAX_TITLE,szTitle);
        hButton[i] = WinCreateWindow (hWndClient,WC_BUTTON,szTitle,
            WS_VISIBLE | BS_PUSHBUTTON,
            (iColumn * i) + iButtonX,iButtonY,
            iButtonWidth,iButtonHeight,
            hWndClient,HWND_TOP,ID_FIRST_BUTTON + i,NULL,NULL);
    }
    WinReleasePS(hPS);

    // Update the title
    WinSendMsg(hDataWnd,WM_UPDATE_TITLE,0L,0L);
    WinShowWindow(hDataTitle,TRUE);

    for (i = 0; i < NUM_MENUS; i++)
        hPopupMenu[i] = WinLoadMenu(HWND_OBJECT,0,FIRST_MENU + i);

}


void APIENTRY enum_app_names(HINI hini,USHORT usStringID)
/*-----------------------------------------------------------------*\

   This function will query the size required to hold all of the
   application names for the current INI file. If the file contains
   entries a temporary block of memory is allocated and the application
   names are queried from PM. The strings are then added to the 
   application name listbox and the memory if freed. The first entry 
   in the listbox is selected causing it's owner, the client, to be 
   notifed. The client window will then fill the key name listbox
   by calling enum_key_name function.

\*-----------------------------------------------------------------*/
{
     PVOID pData;
     PBYTE pCurrent;
     ULONG ulSize = 0L;

     if (PrfQueryProfileSize(hini,NULL,NULL,(PULONG)&ulSize) && ulSize)
     {
         DosSubAlloc(pMem,(PPVOID)&pData,ulSize);
         if(PrfQueryProfileString(hini,NULL,NULL,"No Entries",pData,ulSize))
         {
             pCurrent = pData;
             WinEnableWindowUpdate(hAppLBox,FALSE);
             WinSendMsg (hAppLBox,LM_DELETEALL,NULL,NULL);
             while (*pCurrent)
             {
                 WinSendMsg (hAppLBox,LM_INSERTITEM,(MPARAM)LIT_SORTASCENDING,(MPARAM)pCurrent);
                 while(*pCurrent)
                     pCurrent++;
                 pCurrent++;
             }
             WinSendMsg (hAppLBox,LM_SELECTITEM,MPFROMSHORT(0),
                 MPFROMSHORT(TRUE));
             WinEnableWindowUpdate(hAppLBox,TRUE);
             if (usStringID)
             {
                 WinLoadString (hab,0,usStringID,MAX_TITLE,szTitle);
                 WinSetWindowText(hWndFrame,(PSZ)szTitle);
             }
         }
         DosSubFree(pMem,pData,ulSize);
     }
     else
     {
         WinAlarm(HWND_DESKTOP,WA_WARNING);
         WinEnableWindowUpdate(hAppLBox,FALSE);
         WinSendMsg (hAppLBox,LM_DELETEALL,NULL,NULL);
         WinSendMsg (hAppLBox,LM_INSERTITEM,(MPARAM)LIT_SORTASCENDING,"No Entries");
         WinSendMsg (hAppLBox,LM_SELECTITEM,MPFROMSHORT(0),MPFROMSHORT(TRUE));
         WinEnableWindowUpdate(hAppLBox,TRUE);
         // No entries exist force the data window to free up it's 
         // data buffer and repaint.
         get_key_data("","");
         WinInvalidateRect(hDataWnd,NULL,FALSE);
     }
}


void APIENTRY get_key_data(PSZ pAppName,PSZ pKeyName)
/*-----------------------------------------------------------------*\

   This function will attempt to query the keydata for the current
   application / key pair. If keydata currently exists it is freed
   before the new entry is queried.  If successful a message is sent
   to the client to update the keydata windows title with the size
   of the new data and the data window is invalidated.

\*-----------------------------------------------------------------*/
{
     if (pKeyData)
     {
         DosSubFree(pMem,pKeyData,ulKeySize);
         pKeyData  = NULL;
         ulKeySize = 0;
     }
     if (PrfQueryProfileSize(hCurrentIni,pAppName,pKeyName,(PULONG)&ulKeySize) && ulKeySize)
     {
         DosSubAlloc(pMem,(PPVOID)&pKeyData,ulKeySize);
         if(PrfQueryProfileData(hCurrentIni,pAppName,pKeyName,pKeyData,(PULONG)&ulKeySize))
         {
             // Update the title
             WinSendMsg(hDataWnd,WM_UPDATE_TITLE,0L,0L);
             // Force the data window to repaint
             WinInvalidateRect(hDataWnd,NULL,FALSE);
         }
     }
}


void APIENTRY enum_key_name(PSZ pAppName)
/*-----------------------------------------------------------------*\

   This function will query the size required to hold all of the
   key names for the current application name int the current INI file. 
   If the application name contains key strings a temporary block of 
   memory is allocated and the key names are queried from PM. The strings 
   are then added to the application name listbox and the memory if freed.
   The first entry in the listbox is selected causing it's owner, the
   client, to be notifed. The client window will then fill the data window
   by calling get_key_data.

\*-----------------------------------------------------------------*/
{
     PVOID pData;
     PBYTE pCurrent;
     ULONG ulSize = 0L;

     if (PrfQueryProfileSize(hCurrentIni,pAppName,NULL,(PULONG)&ulSize) && ulSize)
     {
         DosSubAlloc(pMem,(PPVOID)&pData,ulSize);
         if(PrfQueryProfileString(hCurrentIni,pAppName,NULL,"No Entries",pData,ulSize))
         {
             pCurrent = pData;
             WinEnableWindowUpdate(hKeyLBox,FALSE);
             WinSendMsg (hKeyLBox,LM_DELETEALL,NULL,NULL);
             while (*pCurrent)
             {
                 WinSendMsg (hKeyLBox,LM_INSERTITEM,(MPARAM)LIT_SORTASCENDING,(MPARAM)pCurrent);
                 while(*pCurrent)
                     pCurrent++;
                 pCurrent++;
             }
             WinSendMsg (hKeyLBox,LM_SELECTITEM,MPFROMSHORT(0),
                 MPFROMSHORT(TRUE));
             WinEnableWindowUpdate(hKeyLBox,TRUE);
         }
         DosSubFree(pMem,pData,ulSize);
     }
     else
     {
         WinEnableWindowUpdate(hKeyLBox,FALSE);
         WinSendMsg (hKeyLBox,LM_DELETEALL,NULL,NULL);
         WinEnableWindowUpdate(hKeyLBox,TRUE);
         // No entries exist force the data window to free up it's 
         // data buffer and repaint.
         get_key_data("","");
         WinInvalidateRect(hDataWnd,NULL,FALSE);
     }
}


void APIENTRY size_controls(MPARAM lNewSize)
/*-----------------------------------------------------------------*\

   Since the children are scaled to the client window size this function
   is called when the client window receives a WM_SIZE message. All
   child windows are resized to fit the new client size. The buttons
   will resized but not overlap.

\*-----------------------------------------------------------------*/
{
    SHORT sNewWidth  = SHORT1FROMMP(lNewSize);
    SHORT sNewHeight = SHORT2FROMMP(lNewSize);
    int   iButtonX;
    int   iColumn;
    int   i;

    WinSetWindowPos (hAppTitle,HWND_TOP,
        0,sNewHeight - nSysFontHeight,sNewWidth / 2 - 1,nSysFontHeight,
        SWP_SIZE | SWP_MOVE);
    WinSetWindowPos (hKeyTitle,HWND_TOP,
        sNewWidth / 2,sNewHeight - nSysFontHeight,sNewWidth / 2 - 1,nSysFontHeight,
        SWP_SIZE | SWP_MOVE);
    WinSetWindowPos (hDataTitle,HWND_TOP,
        0,sNewHeight / 2,sNewWidth,nSysFontHeight,
        SWP_SIZE | SWP_MOVE);
    WinSetWindowPos (hAppLBox,HWND_TOP,
        1,sNewHeight / 2 + nSysFontHeight,
        sNewWidth / 2 - 2,sNewHeight / 2 - (nSysFontHeight * 2 + 1),
        SWP_SIZE | SWP_MOVE);
    WinSetWindowPos (hKeyLBox,HWND_TOP,
        sNewWidth / 2 + 1,sNewHeight / 2 + nSysFontHeight,
        sNewWidth / 2 - 2,sNewHeight / 2 - (nSysFontHeight * 2 + 1),
        SWP_SIZE | SWP_MOVE);

    iColumn  = (max (sNewWidth,iButtonWidth * NUM_BUTTONS)) / NUM_BUTTONS;
    iButtonX = (iColumn - iButtonWidth) / 2;

    for (i = 0; i < NUM_BUTTONS; i++)
        WinSetWindowPos (hButton[i],HWND_TOP,
            (iColumn * i) + iButtonX,iButtonY,0,0,SWP_MOVE);

    WinSetWindowPos (hDataFrame,HWND_TOP,
            1,iButtonHeight * 2,sNewWidth - 2,
            sNewHeight / 2 - 2 - (iButtonHeight * 2),
            SWP_SHOW | SWP_ZORDER | SWP_SIZE | SWP_MOVE);
}


USHORT APIENTRY post_error(HWND hWnd,USHORT usErrorID)
/*-----------------------------------------------------------------*\

   This is a utility function which will post a message box with
   a string loaded from the resource file.

\*-----------------------------------------------------------------*/
{
    WinLoadString (hab,0,usErrorID,MAX_BUFF,szBuff1);
    return (WinMessageBox(HWND_DESKTOP,hWnd,szBuff1,
        (PSZ)"Error",0,MB_OK | MB_ICONHAND | MB_APPLMODAL));
}


void APIENTRY process_command(HWND hWnd,MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   All WM_COMMAND messages received by the client window will be
   processed here.  For each button command a popup menu is displayed.

\*-----------------------------------------------------------------*/
{
    PVOID   pKey;
    RECTL   Rectl;
    USHORT  usSelection;
    USHORT  usAddCmd    = IDM_ADD_KEY;
    USHORT  usCommand   = SHORT1FROMMP(mp1);
    USHORT  usButtonNum = (USHORT)(usCommand - ID_FIRST_BUTTON);

    switch(usCommand)
    {
        case ID_DELETE:
        case ID_DISPLAY:
        case ID_ADD:
        case ID_SAVE:
            WinQueryWindowRect (hButton[usButtonNum],(PRECTL)&Rectl);
            WinMapWindowPoints(hButton[usButtonNum],hWndFrame,
                (PPOINTL)&Rectl,2);
            WinPopupMenu(hWndFrame,hWndClient,hPopupMenu[usCommand - ID_FIRST_BUTTON],
                Rectl.xLeft + 1,Rectl.yTop + 1,
                usSelectItem[usButtonNum],
                PU_HCONSTRAIN | PU_VCONSTRAIN | PU_KEYBOARD |
                PU_MOUSEBUTTON1 | PU_SELECTITEM);
            break;

        case ID_EXIT:
            WinPostMsg(hWndFrame,WM_QUIT,0,0);
            break;

        case IDM_ADD_APP:
            usAddCmd = IDM_ADD_APP;
        case IDM_ADD_KEY:
            usSelectItem[0] = usCommand;
            WinDlgBox(HWND_DESKTOP,hWndFrame,(PFNWP)AddDlgProc,0,ADD_DLG,(PVOID)&usAddCmd);
            break;

        case IDM_ABOUT:
            WinLoadString (hab,0,ID_APPNAME,MAX_TITLE,szTitle);
            DisplayAbout (hWnd, szTitle);
            break;

        case IDM_REFRESH:
            refresh_display();
            break;
        case IDM_DISPLAY_USER:
        case IDM_DISPLAY_SYS:
        case IDM_DISPLAY_USERSYS:
        case IDM_DISPLAY_PRIVATE:
            if ( (usSelectItem[1] != usCommand) ||
                 (usCommand == IDM_DISPLAY_PRIVATE) )
            {
                change_ini_file(usCommand);
            }
            break;

        case IDM_DELETE_APP:
        case IDM_DELETE_KEY:
            usSelectItem[2] = usCommand;
            usSelection = (USHORT)WinSendMsg (hAppLBox,LM_QUERYSELECTION,
                MPFROMSHORT(LIT_FIRST),NULL);
            if (usSelection != LIT_NONE)
            {
                WinSendMsg(hAppLBox,LM_QUERYITEMTEXT,
                    MPFROM2SHORT(usSelection,MAX_TITLE),MPFROMP(szTitle));
                if (usCommand == IDM_DELETE_KEY)
                {
                    usSelection = (USHORT)WinSendMsg (hKeyLBox,LM_QUERYSELECTION,
                        MPFROMSHORT(LIT_FIRST),NULL);
                    WinSendMsg(hKeyLBox,LM_QUERYITEMTEXT,
                        MPFROM2SHORT(usSelection,MAX_TITLE),MPFROMP(szKey));
                    pKey = szKey;
                }
                else
                    pKey = NULL;
                if (!(PrfWriteProfileString(hCurrentIni,szTitle,pKey,NULL)))
                {
                    post_error(hWnd,ID_DELETE_ERROR);
                }
                refresh_display();
            }
            else
                WinAlarm(HWND_DESKTOP,WA_NOTE);
            break;

        case IDM_SAVE_CHANGES:
            usSelectItem[3] = usCommand;
            break;
        case IDM_SAVE_LOCATION:
            usSelectItem[3] = usCommand;
            WinQueryWindowRect(hWndFrame,(PRECTL)&Rectl);
            WinMapWindowPoints(hWndFrame,HWND_DESKTOP,(PPOINTL)&Rectl,1);
            WinLoadString (hab,0,ID_INITOR,MAX_TITLE,szTitle);
            WinLoadString (hab,0,ID_LOCATION,MAX_TITLE,szKey);
            if (!(PrfWriteProfileData(HINI_USERPROFILE,szTitle,szKey,
                (PVOID)&Rectl,sizeof(RECTL))))
                post_error(hWnd,ID_SAVEPOS_ERROR);
            refresh_display();
            break;
    }
}

void APIENTRY process_control(HWND hWnd,MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   This function will handle the control notifications passed to
   the client window in the WM_CONTROL message. When an item is
   selected in the application or key name list boxes the remaining
   listboxes and controls are updated.

\*-----------------------------------------------------------------*/
{
    USHORT usSelection;

    if (SHORT2FROMMP(mp1) == LN_SELECT)
    {
        usSelection = (USHORT)WinSendMsg (hAppLBox,LM_QUERYSELECTION,
            MPFROMSHORT(LIT_FIRST),NULL);
        WinSendMsg(hAppLBox,LM_QUERYITEMTEXT,
            MPFROM2SHORT(usSelection,MAX_TITLE),
            MPFROMP(szTitle));
        switch (LOUSHORT(mp1))
        {
            case APP_LBOX:
                enum_key_name(szTitle);
                break;
            case KEY_LBOX:
                usSelection = (USHORT)WinSendMsg (hKeyLBox,LM_QUERYSELECTION,
                    MPFROMSHORT(LIT_FIRST),NULL);
                WinSendMsg(hKeyLBox,LM_QUERYITEMTEXT,
                    MPFROM2SHORT(usSelection,MAX_TITLE),
                    MPFROMP(szKey));
                get_key_data(szTitle,szKey);
                break;
        }
    }
}

void APIENTRY refresh_display()
/*-----------------------------------------------------------------*\

   This is a utility which causes the current INI file to be reread
   and updated on the display. The current selection is saved an 
   restored after the update. This prevents the listbox from scrolling
   to the top after updateing an entry.

\*-----------------------------------------------------------------*/
{
    USHORT usCurrApp = (USHORT)WinSendMsg(hAppLBox,LM_QUERYSELECTION,0,0);
    USHORT usCurrKey = (USHORT)WinSendMsg(hKeyLBox,LM_QUERYSELECTION,0,0);

    enum_app_names(hCurrentIni,0);

    WinSendMsg (hAppLBox,LM_SELECTITEM,MPFROMSHORT(usCurrApp),MPFROMSHORT(TRUE));
    WinSendMsg (hKeyLBox,LM_SELECTITEM,MPFROMSHORT(usCurrKey),MPFROMSHORT(TRUE));
}


MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

   This is the application callback which handles the messages 
   necessary to INITOR.

\*-----------------------------------------------------------------*/
{
    HPS     hps;
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;

    switch (msg)
    {
        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            WinEndPaint (hps);
            break;

        case WM_ERASEBACKGROUND:
            WinFillRect((HPS)LONGFROMMP(mp1),PVOIDFROMMP(mp2),CLR_PALEGRAY);
            mReturn = MRFROMLONG(0L);
            break;

        case WM_COMMAND:
            process_command(hWnd,mp1,mp2);
            break;

        case WM_CONTROL:
            process_control(hWnd,mp1,mp2);
            break;

        case WM_SIZE:
            size_controls(mp2);
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


MRESULT EXPENTRY AddDlgProc(HWND hWndDlg,ULONG ulMessage,MPARAM mp1,MPARAM mp2)
/*-----------------------------------------------------------------*\

   This is the callback for the dialog which allows you to add items
   to the current INI file. It will disable the OK button until all
   three edit fields contain text.

\*-----------------------------------------------------------------*/
{
    static HWND hAppWnd;
    static HWND hKeyWnd;
    static HWND hDataWnd;
    USHORT usSelection;
    USHORT usAddCmd;
    ULONG  ulLength;
    PVOID  pData = NULL;
    
    switch (ulMessage) 
    {
        case WM_INITDLG:
            hAppWnd  = WinWindowFromID(hWndDlg,DID_APP_NAME);
            hKeyWnd  = WinWindowFromID(hWndDlg,DID_KEY_NAME);
            hDataWnd = WinWindowFromID(hWndDlg,DID_KEY_DATA);
            usAddCmd = *((PUSHORT)mp2);

            WinSendMsg(hAppWnd,EM_SETTEXTLIMIT,MPFROMSHORT(MAX_TITLE),0);
            WinSendMsg(hKeyWnd,EM_SETTEXTLIMIT,MPFROMSHORT(MAX_TITLE),0);
            usSelection = (USHORT)WinSendMsg (hAppLBox,LM_QUERYSELECTION,
                MPFROMSHORT(LIT_FIRST),NULL);
            if ((usSelection != LIT_NONE) && (usAddCmd == IDM_ADD_KEY))
            {
                WinSendMsg(hAppLBox,LM_QUERYITEMTEXT,
                    MPFROM2SHORT(usSelection,MAX_TITLE),MPFROMP(szTitle));
                WinSetWindowText(WinWindowFromID(hWndDlg,DID_APP_NAME),szTitle);
                WinSendMsg(hAppWnd,EM_SETREADONLY,MPFROMSHORT(TRUE),0);
                WinSetFocus(HWND_DESKTOP,hKeyWnd);
                return ((MPARAM)1);
            }
            break;

        case WM_CONTROL:
              if ( (SHORT2FROMMP(mp1) == EN_CHANGE) ||
                   (SHORT2FROMMP(mp1) == MLN_CHANGE) )
              {
                  if ( (WinQueryWindowTextLength(hAppWnd)) &&
                       (WinQueryWindowTextLength(hKeyWnd)) &&
                       (WinQueryWindowTextLength(hDataWnd)) )
                      WinEnableWindow(WinWindowFromID(hWndDlg,DID_OK),TRUE);
                  else
                      WinEnableWindow(WinWindowFromID(hWndDlg,DID_OK),FALSE);
              }
            break;

        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1)) 
            {
                case DID_OK:
                    WinEnableWindow(WinWindowFromID(hWndDlg,DID_OK),FALSE);
                    WinQueryWindowText(hAppWnd,MAX_TITLE,szTitle);
                    WinQueryWindowText(hKeyWnd,MAX_TITLE,szKey);
                    ulLength = WinQueryWindowTextLength(hDataWnd);
                    DosSubAlloc(pMem,(PPVOID)&pData,ulLength);

                    if (pData && WinQueryWindowText(hDataWnd,ulLength,pData))
                    {
                        if (!(PrfWriteProfileString(hCurrentIni,szTitle,szKey,pData)))
                        {
                            post_error(hWndDlg,ID_WRITE_ERROR);
                        }
                        DosSubFree(pMem,pData,ulLength);
                        refresh_display();
                    }
                    else
                        post_error(hWndDlg,ID_QTEXT_ERROR);
                    WinDismissDlg(hWndDlg,DID_OK);
                    return 0;
            }
            break;
    }
    return (WinDefDlgProc(hWndDlg,ulMessage,mp1,mp2));
}


MRESULT EXPENTRY DataWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

   This is the callback for INITOR's private data display window.
   It will display data in both HEX and ASCII and allows scrolling.

\*-----------------------------------------------------------------*/
{
    static HPS  hPS;           
    static int  iWndHeight;
    static int  iNumLines;
    static int  iTopLine = 1;
    BOOL        bHandled = TRUE;
    MRESULT     mReturn  = 0;

    switch (msg)
    {
        case WM_CREATE:
            hPS = init_data_window(hWnd);
            hVScroll = WinWindowFromID(hDataFrame,FID_VERTSCROLL);
            break;

        case WM_PAINT:
            iNumLines = paint_data_window(hWnd,hPS,iTopLine);
            WinSendMsg(hVScroll,SBM_SETSCROLLBAR,MPFROMSHORT(iTopLine),
                MPFROM2SHORT(1,iNumLines));
            break;

        case WM_VSCROLL:
            switch (SHORT2FROMMP(mp2))
            {
                case SB_LINEDOWN:
                   if (iTopLine < iNumLines)
                   {
                       iTopLine++;
                       WinInvalidateRect(hDataWnd,NULL,FALSE);
                   }
                   break;
                case SB_LINEUP:
                   if (iTopLine > 1)
                   {
                       iTopLine--;
                       WinInvalidateRect(hDataWnd,NULL,FALSE);
                   }
                   break;
                case SB_SLIDERTRACK:
                   if ((SHORT1FROMMP(mp2) >= 1) || (SHORT1FROMMP(mp2)) <= iNumLines)
                   {
                       iTopLine = SHORT1FROMMP(mp2);
                       WinInvalidateRect(hDataWnd,NULL,FALSE);
                   }
                   else
                       WinAlarm(HWND_DESKTOP,WA_NOTE);
                   break;
            }
            break;

        case WM_ERASEBACKGROUND:
            WinFillRect((HPS)LONGFROMMP(mp1),PVOIDFROMMP(mp2),CLR_WHITE);
            mReturn = MRFROMLONG(0L);
            break;

        case WM_SIZE:
            iWndHeight = SHORT2FROMMP(mp2);
            break;

        case WM_UPDATE_TITLE:
            WinLoadString (hab,0,ID_DATA,MAX_BUFF,(PSZ)szBuff1);
            sprintf(szBuff2,szBuff1,ulKeySize,ulKeySize);
            WinSetWindowText(hDataTitle,(PSZ)szBuff2);
            break;

        case WM_DESTROY:
            GpiDeleteSetId(hPS,FIXED_FONT_LCID);
            GpiAssociate(hPS,0);
            GpiDestroyPS(hPS);
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}


HPS APIENTRY init_data_window(HWND hWnd)
/*-----------------------------------------------------------------*\

   This function will intialize the data window by creating a PS
   and create a Monospaced system font. The default attributes and
   the font are set into the PS.
   
\*-----------------------------------------------------------------*/
{
    HPS          hPS = 0;
    SIZEL        sizel;
    FATTRS       fattr;
    FONTMETRICS  FontMetrics;

    sizel.cx = sizel.cy = 0;
    if (hPS = GpiCreatePS(hab,WinOpenWindowDC(hWnd),
        (PSIZEL)&sizel,PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC))
    {
        fattr.usRecordLength  = sizeof(FATTRS);
        fattr.fsSelection     = 0;
        fattr.lMatch          = 0;
        fattr.idRegistry      = 0;
        fattr.usCodePage      = 0;
        fattr.lMaxBaselineExt = 16;
        fattr.lAveCharWidth   = 8;
        fattr.fsType          = 0;
        fattr.fsFontUse       = 0;
        strcpy(fattr.szFacename,"System Monospaced");
        GpiCreateLogFont (hPS,NULL,FIXED_FONT_LCID,&fattr);
        GpiSetCharSet(hPS,FIXED_FONT_LCID);
        GpiQueryFontMetrics(hPS,(long)sizeof(FONTMETRICS),&FontMetrics);
        nFixedFontWidth  = FontMetrics.lAveCharWidth;
        nFixedFontHeight = FontMetrics.lMaxBaselineExt;
        GpiSetBackMix(hPS,BM_OVERPAINT);
    }
    return (hPS);
}


int APIENTRY format_line(PSZ pRetBuff,PSZ pData,int iLineNum,int iLineSize)
/*-----------------------------------------------------------------*\

   This function will take the data in pData for and format iLineSize
   bytes of the data as HEX and ASCII in pRetBuff.

\*-----------------------------------------------------------------*/
{
    PSZ  pCurRet;
    int  i;
    int  iLength;

    // Format the offset from the start of data
    i = sprintf(pRetBuff,"%04X: ",(iLineNum - 1) * 16);

    // Set the remaining bytes to spaces
    pCurRet = pRetBuff + i;
    memset(pCurRet,' ',80);

    // Format the data as HEX
    pCurRet = pRetBuff + HEX_START;
    for (i = 0; i < iLineSize; i++)
    {
        if (*(pData + i) < 16)
        {
            *pCurRet++ = '0';
            ltoa((LONG)*(pData + i),pCurRet,16);
            pCurRet += 1;
        }
        else
        {
            ltoa((LONG)*(pData + i),pCurRet,16);
            pCurRet += 2;
        }
        if (i == 7)
            *pCurRet++ = '-';
        else
            *pCurRet++ = ' ';
    }

    // Format the data as an ASCII string
    pCurRet = pRetBuff + ASCII_START;
    *pCurRet++ = '>';
    for (i = 0; i < iLineSize; i++)
    {
        if ((*(pData + i) >= 32) && (*(pData + i) <= 128))
            *(pCurRet + i) = *(pData + i);
        else
            *(pCurRet + i) = '.';
    }
    *(pCurRet + iLineSize)     = '<';
    *(pCurRet + iLineSize + 1) = '\0';

    iLength = strlen(pRetBuff);
    return (iLength);
}


int APIENTRY paint_data_window(HWND hWnd,HPS hPS,int iStartLine)
/*-----------------------------------------------------------------*\

   This function will handle the painting of the data window. The
   data is output after being formated by format_line. A maximum
   of 16 bytes are displayed per line.  The first line displayed
   is determined by iStartLine.

\*-----------------------------------------------------------------*/
{
    RECTL   Rectl;
    PSZ     pData;
    POINTL  ptl;
    int     i,j;
    int     iSize;
    int     iPartialLine;
    int     iLineLength = MAX_LINE;
    int     iNumLines = ulKeySize / MAX_LINE;

    hPS = WinBeginPaint (hWnd,hPS,&Rectl);
    WinFillRect(hPS,&Rectl,CLR_WHITE);

    WinQueryWindowRect(hWnd,&Rectl);
    if ((ulKeySize) && (pKeyData))
    {
        if ((iPartialLine = ulKeySize % MAX_LINE) != 0)
            iNumLines++;

        ptl.x = Rectl.xLeft + nFixedFontWidth;
        pData = (PSZ)pKeyData + ((iStartLine - 1) * 16);
        for (j = 1,i = iStartLine; i <= iNumLines; i++,j++)
        {
            ptl.y = (Rectl.yTop - nSysFontHeight * j);
            if ((i == iNumLines) && (iPartialLine))
                iLineLength = iPartialLine;

            iSize = format_line(szBuff1,pData,i,iLineLength);
            GpiCharStringAt (hPS,&ptl,iSize,szBuff1);
            pData += iLineLength;
        }
    }
    WinEndPaint (hPS);
    return (iNumLines);
}

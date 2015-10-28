/* --------------------------------------------------------------------
                        Common Dialogs Program
                              Chapter 4

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */


#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include "commdlg.h"
#include "..\common\about.h"


#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED     3
#define DRIVE_REMOTE    4

MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY FileOpenDlgProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY FontDlgProc(HWND,ULONG,MPARAM,MPARAM);

HAB    hab;
HWND   hWndFrame, 
       hWndClient,
       hFontDlg = 0;
PVOID  pMem;
PCHAR  pFileText    = 0;
PAPSZ  pDrivesList  = 0;
ULONG  ulTextSize   = 0;
ULONG  ulDrivesSize = 0;
int    cyHeight;
char   szBuff1[MAX_BUFF];
char   szFilePath[CCHMAXPATH];
char   szFamilyname[FACESIZE];
ATOM   atSavePath;
HATOMTBL    hAtomTable = 0;
FATTRS      Fattrs;
FONTDLG     FontDlg2;


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

    hAtomTable = WinQuerySystemAtomTable();

    DosAllocMem((PPVOID)&pMem,0x1000,PAG_READ | PAG_WRITE);
    DosSubSet(pMem,DOSSUB_INIT | DOSSUB_SPARSE_OBJ,8192);
    get_fixed_drives();

    WinRegisterClass (hab, szClientClass, ClientWndProc, CS_SIZEREDRAW, 0);
    WinLoadString (hab, 0, IDS_APPNAME, sizeof(szBuff1), szBuff1);

    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, WS_VISIBLE,
        &flFrameFlags, szClientClass, szBuff1, 0, 0, ID_APPNAME, &hWndClient);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}


HWND APIENTRY add_checkbox(HWND hWndDlg)
/*-----------------------------------------------------------------*\

   This function will create a checkbox button in the lower right
   hand corner of hWndDlg. The dialog is expected to be the standard
   font selection dialog. The button is vertically aligned to the 
   center of OK pushbutton and horizonatally aligned so that all of 
   the text is visable in the dialog.

\*-----------------------------------------------------------------*/
{
   HWND    hCheckBox;
   HPS     hPS;
   RECTL   DlgRectl;
   RECTL   ButtonRectl;
   int     iLength;
   PVOID   pData;
   FONTMETRICS FontMetrics;
   POINTL  Ptl[TXTBOX_COUNT + 1];

   // Get the sizes of the dialog and the cancel pushbutton
   WinQueryWindowRect(hWndDlg,&DlgRectl);
   WinQueryWindowRect(WinWindowFromID(hWndDlg,DID_CANCEL),&ButtonRectl);
   WinMapWindowPoints(WinWindowFromID(hWndDlg,DID_CANCEL),hWndDlg,
     (PPOINTL)&ButtonRectl,2);

   // Load the checkbox text string
   DosSubAlloc(pMem,(PPVOID)&pData,MAX_BUFF);
   iLength = WinLoadString (hab,0,IDS_UPDATE,MAX_BUFF,(PSZ)pData);

   // Get the string extent and the average character width.
   hPS = WinGetPS(hWndDlg);
   GpiQueryTextBox(hPS,iLength,(PSZ)pData,TXTBOX_COUNT,Ptl);
   GpiQueryFontMetrics(hPS,sizeof(FONTMETRICS),(PFONTMETRICS)&FontMetrics);
   WinReleasePS(hPS);

   // Increase the height of the window by 50%
   Ptl[TXTBOX_TOPRIGHT].y += Ptl[TXTBOX_TOPRIGHT].y / 2;

   // Create the checkbox
   hCheckBox = WinCreateWindow (hWndDlg,WC_BUTTON,(PSZ)pData,
       WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
       ButtonRectl.xRight + FontMetrics.lAveCharWidth,
       ButtonRectl.yBottom + 
       ((ButtonRectl.yTop - ButtonRectl.yBottom) / 2) -
       (Ptl[TXTBOX_TOPRIGHT].y / 2),
       DlgRectl.xRight - ButtonRectl.xRight - (FontMetrics.lAveCharWidth + 1),
       Ptl[TXTBOX_TOPRIGHT].y,
       hWndDlg,HWND_TOP,DID_UPDATE_FONT,0,0);
    DosSubFree(pMem,pData,MAX_BUFF);
    return (hCheckBox);
}


void APIENTRY cleanup()
/*-----------------------------------------------------------------*\

   This function is called to free the memory allocated and destroy
   the application window.

\*-----------------------------------------------------------------*/
{
    if (pFileText)
        DosSubFree(pMem,pFileText,ulTextSize);
    if (pDrivesList)
        DosSubFree(pMem,pDrivesList,ulDrivesSize);
    DosFreeMem(pMem);
    WinDestroyWindow (hWndFrame);
}



void APIENTRY get_fixed_drives()
/*-----------------------------------------------------------------*\

   This function will enumerate the drives available to this device
   and create papszDriveList parameter of the FILEDLG structure. Only
   non removable drives are placed in the list.

\*-----------------------------------------------------------------*/
{
    APIRET      RetCode;
    ULONG       ulBytes;
    PFSQBUFFER2 pfsq;
    HFILE       hDrive;
    ULONG       ulAction;
    int         nDrive;
    char        szDrive[3] = " :";
    PUSHORT     pDrives = 0;
    PULONG      pStringArray;
    PSZ         pDriveLetter;

    // Disable the exception handling when the drive door is open 
    DosError (0);

    // Allocate the memory for the pDrives array
    DosSubAlloc(pMem,(PPVOID)&pDrives,sizeof(USHORT) * MAX_DRIVES);
    memset(pDrives,0,sizeof(USHORT) * MAX_DRIVES);
    DosSubAlloc(pMem,(PPVOID)&pfsq,1024);

    // Allocate memory for pDrivesList 
    ulDrivesSize = ((MAX_DRIVES + 1) * sizeof(PLONG)) + (3 * MAX_DRIVES);
    DosSubAlloc(pMem,(PPVOID)&pDrivesList,ulDrivesSize);
    pStringArray = (PULONG)pDrivesList;
    memset((PSZ)pStringArray,0,ulDrivesSize);
    pDriveLetter = (PSZ)pStringArray + ((MAX_DRIVES + 1) * sizeof(PLONG));

    for (nDrive = 0; nDrive < MAX_DRIVES; nDrive++)
    {
        szDrive[0] = (CHAR)(nDrive + 'A');
        ulBytes    = 1024;

        // Query the file system to determine if it is a remote drive 
        RetCode = DosQueryFSAttach ((PSZ)szDrive,0L,FSAIL_QUERYNAME,
            (PFSQBUFFER2)pfsq,&ulBytes);
        if (((RetCode == 0) || (RetCode == ERROR_BUFFER_OVERFLOW)) &&
            (pfsq->szName[0] == (UCHAR)szDrive[0]))
        {
            if (pfsq->iType == FSAT_REMOTEDRV)
                pDrives[nDrive] = DRIVE_REMOTE;
            else
                pDrives[nDrive] = DRIVE_FIXED;
        }
        else if (RetCode == ERROR_NOT_READY)
            // If drive not ready then assume removable 
            pDrives[nDrive] = DRIVE_REMOVABLE;

        // If drive is not remote, determine if it is removeable 
        if (pDrives[nDrive] == DRIVE_FIXED)
        {
            // Open the drive and issue a DSK_BLOCKREMOVABLE request 
            if (!DosOpen ((PSZ)szDrive,(PHFILE)&hDrive,&ulAction,
                0L,0L,FILE_OPEN,OPEN_FLAGS_DASD | OPEN_SHARE_DENYNONE,0L))
            {
                USHORT usType    = 0;
                BYTE   bCmd      = 0;
                ULONG  cbParmLen = 1L;
                ULONG  cbDataLen = 2L;

                if (!DosDevIOCtl (hDrive,8L,32L,(PVOID)&bCmd,1L,&cbParmLen,
                    (PVOID)&usType,2L,&cbDataLen))
                    pDrives[nDrive] = (USHORT)((usType == 0) ? DRIVE_REMOVABLE : DRIVE_FIXED);
                else
                    pDrives[nDrive] = 0;
            }
            DosClose (hDrive);
        }

        if (pDrives[nDrive] == DRIVE_FIXED)
        {
            // Only add fixed drives to the drives list
            *pStringArray = (ULONG)pDriveLetter;
            pStringArray++;
            *pDriveLetter++ = (nDrive + 'A');
            *pDriveLetter++ = ':';
            pDriveLetter++;
        }
    }

    // Reenable exception handling 
    DosError (1);

    // Free Temp memory
    DosSubFree(pMem,pDrives,sizeof(USHORT) * MAX_DRIVES);
    DosSubFree(pMem,(PVOID)pfsq,1024);
}


void APIENTRY load_file(PSZ pFileName)
/*-----------------------------------------------------------------*\

   This function is called to open pFileName, allocate memory, read
   the file into memory, and close the file. If an error occurs a
   message box is posted.

\*-----------------------------------------------------------------*/
{
    ULONG      i;
    FILESTATUS FileStat;
    HFILE      hFile;
    ULONG      ulAction;
    
    if (!DosOpen(pFileName,(PHFILE)&hFile,(PULONG)&ulAction,0, 
         FILE_NORMAL,FILE_OPEN,OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE,0) )
    {
        if (pFileText) 
        {
            DosFreeMem(pFileText);
            pFileText  = 0; 
            ulTextSize = 0;
        }

        if (!DosQueryFileInfo(hFile,1,(PVOID)&FileStat,sizeof(FILESTATUS)))
        {
            ulTextSize = FileStat.cbFileAlloc;
            if (DosAllocMem((PPVOID)&pFileText,ulTextSize,fALLOC))
            {
                ulTextSize = 0;
                pFileText  = 0;
                post_error(hWndClient,IDS_MEMORY_ERROR);
                DosClose(hFile);
                return;
            }    
        }

        if (DosRead(hFile,(PVOID)pFileText,ulTextSize,(PULONG)&ulAction))
        {
           post_error(hWndClient,IDS_READ_ERROR);
           WinInvalidateRect(hWndClient,0,FALSE);
        }
        else
        {
           for (i = 0; i < ulTextSize;i++)
           {
               // Remove NULL characters and carrige returns.
               if (*(pFileText + i) == 0)
                   *(pFileText + i) = ' ';
               if (*(pFileText + i) == 0xD)
                   *(pFileText + i) = ' ';
           }
           strcpy((PSZ)szFilePath,pFileName);
           set_font ((PSZ)Fattrs.szFacename);
        }
        DosClose(hFile);
    }
    else
        post_error(hWndClient,IDS_OPEN_ERROR);
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
    FILEDLG FileDlg;
    FONTDLG FontDlg;

    switch(SHORT1FROMMP(mp1))
    {
        case IDM_OPEN1:
            // Create the default file open dialog.
            memset(&FileDlg,0,sizeof(FILEDLG));
            FileDlg.cbSize   = sizeof(FILEDLG);
            FileDlg.fl       = FDS_CENTER | FDS_OPEN_DIALOG;
            WinFileDlg(HWND_DESKTOP,hWndClient,(PFILEDLG)&FileDlg);
            if (FileDlg.lReturn == DID_OK) 
                WinSetWindowText(hWndFrame,(PSZ)FileDlg.szFullFile);
            break;

        case IDM_OPEN2:
            memset(&FileDlg,0,sizeof(FILEDLG));
            set_title(szBuff1,IDS_OPEN);
            FileDlg.pszTitle   = szBuff1;
            FileDlg.cbSize     = sizeof(FILEDLG);
            FileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
            FileDlg.papszIDriveList = pDrivesList;
            WinLoadString (hab,0,IDS_PATH,CCHMAXPATH,(PSZ)&FileDlg.szFullFile);
            WinFileDlg(HWND_DESKTOP,hWndClient,(PFILEDLG)&FileDlg);
            if (FileDlg.lReturn == DID_OK)
                load_file((PSZ)&FileDlg.szFullFile);
            break;

        case IDM_OPEN3:
            // Create a dialog with a custom dialog template and callback.
            memset(&FileDlg,0,sizeof(FILEDLG));
            FileDlg.cbSize     = sizeof(FILEDLG);
            FileDlg.pszTitle   = szBuff1;
            FileDlg.fl         = FDS_OPEN_DIALOG | FDS_CUSTOM | FDS_CENTER;
            FileDlg.hMod       = 0;
            FileDlg.usDlgId    = OPEN_DIALOG;
            FileDlg.ulUser     = atSavePath;
            FileDlg.pfnDlgProc = (PFNWP)FileOpenDlgProc;
            set_title(szBuff1,IDS_OPEN);
            set_path((PSZ)FileDlg.szFullFile,FDS_OPEN_DIALOG);
            WinFileDlg(HWND_DESKTOP,hWndClient,(PFILEDLG)&FileDlg);
            if (FileDlg.lReturn == DID_OK)
            {
                if (atSavePath)
                    atSavePath = WinDeleteAtom(hAtomTable,atSavePath);
                if (FileDlg.ulUser == DID_SAVE_DIR) 
                    atSavePath = WinAddAtom(hAtomTable,(PSZ)FileDlg.szFullFile);
                load_file((PSZ)&FileDlg.szFullFile);
            }
            break;

        case IDM_SAVEAS1:
            memset(&FileDlg,0,sizeof(FILEDLG));
            FileDlg.cbSize   = sizeof(FILEDLG);
            FileDlg.pszTitle = szBuff1;
            FileDlg.fl       = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB;
            set_title(szBuff1,IDS_SAVE);
            strcat((PSZ)FileDlg.szFullFile,(PSZ)szFilePath);
            WinFileDlg(HWND_DESKTOP,hWndFrame,(PFILEDLG)&FileDlg);
            if (FileDlg.lReturn == DID_OK) 
            {
                WinSetWindowText(hWndFrame,(PSZ)FileDlg.szFullFile);
                // Code to perform file save operation would go here.
            }
            break;

        case IDM_FONT1:
            szFamilyname[0] = 0; 
            memset((PCH)&FontDlg,0,sizeof(FONTDLG));
            FontDlg.cbSize         = sizeof(FONTDLG); 
            FontDlg.pszFamilyname  = szFamilyname;
            FontDlg.usFamilyBufLen = FACESIZE; 
            FontDlg.fl             = FNTS_CENTER;
            FontDlg.clrFore        = CLR_BLACK;  
            FontDlg.clrBack        = CLR_WHITE; 
            WinFontDlg(HWND_DESKTOP,hWndClient,(PFONTDLG)&FontDlg);
            if (FontDlg.lReturn == DID_OK)
                WinSetWindowText(hWndFrame,(PSZ)FontDlg.pszFamilyname);
            break;

        case IDM_FONT2:
            szFamilyname[0] = 0; 
            memset((PCH)&FontDlg,0,sizeof(FONTDLG));
            FontDlg.cbSize         = sizeof(FONTDLG); 
            FontDlg.hpsScreen      = WinGetPS(hWnd);   
            FontDlg.pszFamilyname  = szFamilyname;
            FontDlg.usFamilyBufLen = FACESIZE; 
            FontDlg.pszPreview     = "Sample Text";
            FontDlg.fl             = FNTS_RESETBUTTON | FNTS_CENTER | 
                                     FNTS_INITFROMFATTRS | FNTS_BITMAPONLY;
            FontDlg.flStyle        = FATTR_SEL_ITALIC;
            FontDlg.clrFore        = CLR_BLACK;  
            FontDlg.clrBack        = CLR_PALEGRAY; 
            FontDlg.fAttrs         = Fattrs;
            FontDlg.pszTitle       = szBuff1;
            set_title(szBuff1,IDS_FONT);
            WinFontDlg(HWND_DESKTOP,hWndClient,(PFONTDLG)&FontDlg);
            if (FontDlg.lReturn == DID_OK)
            {
                set_font((PSZ)FontDlg.pszFamilyname);
                Fattrs = FontDlg.fAttrs;
            }
            WinReleasePS(FontDlg.hpsScreen);
            break;

        case IDM_FONT3:
            szFamilyname[0] = 0; 
            memset((PCH)&FontDlg2,0,sizeof(FONTDLG));
            FontDlg2.hpsScreen      = WinGetPS(hWnd);   
            FontDlg2.cbSize         = sizeof(FONTDLG); 
            FontDlg2.pszFamilyname  = szFamilyname;
            FontDlg2.usFamilyBufLen = FACESIZE; 
            FontDlg2.pszPreview     = "Sample Text";
            FontDlg2.pfnDlgProc     = FontDlgProc;
            FontDlg2.clrFore        = CLR_BLACK;  
            FontDlg2.clrBack        = CLR_PALEGRAY; 
            FontDlg2.fl             = FNTS_CENTER | 
                                      FNTS_INITFROMFATTRS | FNTS_BITMAPONLY |
                                      FNTS_APPLYBUTTON |
                                      FNTS_MODELESS;
            FontDlg2.fAttrs         = Fattrs;
            FontDlg2.pszTitle       = szBuff1;
            set_title(szBuff1,IDS_FONT);
            hFontDlg = WinFontDlg(HWND_DESKTOP,hWndClient,(PFONTDLG)&FontDlg2);
            if (!hFontDlg)
                post_error(hWnd,IDS_FONTDLG_ERROR);
            break;

        case IDM_ABOUT:
            WinLoadString (hab, 0, IDS_APPNAME, sizeof(szBuff1), szBuff1);
            DisplayAbout (hWnd, szBuff1);
            break;

        default:
            break;
    }
    return;
    mp2;
}


void APIENTRY set_font(PSZ pFacename)
/*-----------------------------------------------------------------*\

   This function will set a new title bar text string. The string is
   comprised of the filename of the displayed file followed by the
   facename of the font that file is displayed in. The client window
   is then invalidated. This function should be called after the font
   is changed.

\*-----------------------------------------------------------------*/
{
    ULONG  ulSize;
    PVOID  pData;

    if (szFilePath[0])
    {
        ulSize  = strlen(szFilePath);
        ulSize += strlen(pFacename);
        DosSubAlloc(pMem,(PPVOID)&pData,ulSize);
        strcpy(pData,szFilePath);
        WinLoadString (hab,0,IDS_DASH,MAX_BUFF,szBuff1);
        strcat(pData,szBuff1);
        strcat(pData,pFacename);
        WinSetWindowText(hWndFrame,(PSZ)pData);
        DosSubFree(pMem,pData,ulSize);
    }
    else
        WinSetWindowText(hWndFrame,pFacename);
    WinInvalidateRect(hWndClient,0,FALSE);
}


void APIENTRY set_path(PSZ pFileSpec,USHORT usType)
/*-----------------------------------------------------------------*\

   This function will initialze pFileSpec with a path and file spec
   based on the contents of atSavePath atom. If it is NULL the default
   file spec is loaded otherwise the atom text is retrieved. 

\*-----------------------------------------------------------------*/
{
    char   szBuff[MAX_BUFF];
    ULONG  ulSize;
    PVOID  pData;
    PCHAR  pChar;

    WinLoadString (hab,0,IDS_PATH,MAX_BUFF,szBuff);
    if (atSavePath && 
        (ulSize = WinQueryAtomLength(hAtomTable,atSavePath) + 1))
    {
        DosSubAlloc(pMem,(PPVOID)&pData,ulSize);
        if(WinQueryAtomName (hAtomTable,atSavePath,pData,ulSize))
        {
            if (usType == FDS_OPEN_DIALOG)
            {
                // Find the beginning of the filename.
                pChar = (PCHAR)pData + ulSize - 1;
                while ((*pChar != '\\') && (pChar != pData))
                    pChar--;
                if (*pChar == '\\')
                {
                   pChar++;
                   *pChar = '\0';
                }
                strcat(pData,szBuff);
            }
            strcpy (pFileSpec,(PSZ)pData);
        }
        DosSubFree(pMem,pData,ulSize);
    }
    else
        strcat(pFileSpec,szBuff);
}


void APIENTRY set_title(PSZ pTitle,USHORT usType)
/*-----------------------------------------------------------------*\

   This function will build a dialog title string in the memory pointed 
   to by pTitle. The string is comprised of the application name followed
   by a hyphen followed by the name of the dialog. The usType parameter
   is the string id of the dialog name. All resources are loaded from
   the string table in the resource file.

\*-----------------------------------------------------------------*/
{
    char   szBuff[MAX_BUFF];

    WinLoadString (hab,0,IDS_APPNAME,MAX_BUFF,pTitle);
    WinLoadString (hab,0,IDS_DASH,MAX_BUFF,szBuff);
    strcat(pTitle,szBuff);
    WinLoadString (hab,0,usType,MAX_BUFF,szBuff);
    strcat(pTitle,szBuff);
}


MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
/*-----------------------------------------------------------------*\

   This is the application callback which handles the messages 
   necessary to maintain the client window.

\*-----------------------------------------------------------------*/
{
    static  RECTL Rectl;
    HPS     hPS;
    int     iDrawn;
    int     iTotalDrawn;
    LONG    lHeight;
    BOOL    bHandled = TRUE;
    FONTMETRICS FontMetrics;
    MRESULT     mReturn  = 0;

    switch (msg)
    {
        case WM_CREATE:
            // Query the height of the default font.
            hPS = WinGetPS(hWnd);
            memset(&Fattrs,0,sizeof(FATTRS));
            GpiQueryFontMetrics(hPS,sizeof(FONTMETRICS),(PFONTMETRICS)&FontMetrics);
            Fattrs.lMaxBaselineExt = FontMetrics.lMaxBaselineExt;
            Fattrs.usRecordLength   = sizeof(FATTRS);
            strcpy((PSZ)Fattrs.szFacename,(PSZ)FontMetrics.szFacename);
            WinReleasePS(hWnd);
            szFilePath[0] = '\0';
            break;

        case WM_PAINT:
            hPS = WinBeginPaint (hWnd,0,0);
            WinFillRect(hPS,&Rectl,CLR_WHITE);
            if (pFileText)
            {
                GpiCreateLogFont(hPS,0,CLIENT_FONT,&Fattrs);
                GpiSetCharSet(hPS,CLIENT_FONT);
                GpiQueryFontMetrics(hPS,sizeof(FONTMETRICS),(PFONTMETRICS)&FontMetrics);
                cyHeight = FontMetrics.lMaxBaselineExt + FontMetrics.lExternalLeading;
                lHeight = Rectl.yTop;
                for ( iTotalDrawn = 0;
                      (iTotalDrawn < ulTextSize) && (Rectl.yTop > 0); 
                      Rectl.yTop -= cyHeight) 
                {
                    iDrawn = WinDrawText(hPS,
                        min (0x1000,(ulTextSize - iTotalDrawn)),
                        pFileText + iTotalDrawn,
                        (PRECTL)&Rectl,CLR_BLACK,CLR_WHITE,
                        DT_LEFT | DT_TOP | DT_WORDBREAK);
                    if (iDrawn)
                        iTotalDrawn += iDrawn;
                    else
                        iTotalDrawn++;
                }
                Rectl.yTop = lHeight;
                GpiDeleteSetId(hPS,CLIENT_FONT);  
            }
            WinEndPaint (hPS);
            break;

        case WM_SIZE:
            Rectl.xLeft  = Rectl.yBottom = 0;
            Rectl.xRight = SHORT1FROMMP(mp2);
            Rectl.yTop   = SHORT2FROMMP(mp2);
            break;

        case WM_ERASEBACKGROUND:
            WinFillRect((HPS)LONGFROMMP(mp1),PVOIDFROMMP(mp2),CLR_WHITE);
            mReturn = MRFROMLONG(0L);
            break;

        case WM_COMMAND:
            process_command(hWnd,mp1,mp2);
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


MRESULT EXPENTRY FileOpenDlgProc(HWND hWndDlg,ULONG ulMessage,MPARAM mp1,MPARAM mp2 )
/*-----------------------------------------------------------------*\

   This dialog call back will handle the save directory option added
   to a standard or custom file open dialog.  All other processing is
   handled by WinDefFildDlgProc.

\*-----------------------------------------------------------------*/
{
   PFILEDLG   pFileDlg;

   switch (ulMessage)
   {
       case WM_INITDLG:
           // Determine if the save directory button should be checked.
           pFileDlg = (PFILEDLG)WinQueryWindowULong(hWndDlg,QWL_USER);
           if (pFileDlg->ulUser)
               WinCheckButton(hWndDlg,DID_SAVE_DIR,1);
           break;

       case WM_COMMAND:
           switch (SHORT1FROMMP(mp1)) 
           {
               case DID_OK:
               case DID_CANCEL:
                   if(WinQueryButtonCheckstate(hWndDlg,DID_SAVE_DIR))
                   {
                       // Do the default processing
                       WinDefFileDlgProc(hWndDlg,ulMessage,mp1,mp2);

                       // Get the pointer to the FILEDLG structure
                       pFileDlg = (PFILEDLG)WinQueryWindowULong(hWndDlg,QWL_USER);
                       pFileDlg->ulUser = DID_SAVE_DIR;
                       return (0);
                   }
                   break;
           }
           break;
   }
   return (WinDefFileDlgProc(hWndDlg,ulMessage,mp1,mp2));
}  




MRESULT EXPENTRY FontDlgProc(HWND hWndDlg,ULONG ulMessage,MPARAM mp1,MPARAM mp2 )
/*-----------------------------------------------------------------*\

   This dialog call back will handle the apply button and update checkbox
   which is added during the WM_INITDLG message. All other processing is
   handled by WinDefFontDlgProc.

\*-----------------------------------------------------------------*/
{
   static  PFONTDLG  pFontDlg;
   static  HWND      hWndUpdate;
   MRESULT mReturn;

   switch (ulMessage)
   {
       case WM_INITDLG:
           pFontDlg = (PFONTDLG)WinQueryWindowULong(hWndDlg,QWL_USER);
           hWndUpdate = add_checkbox(hWndDlg);
           break;

       case FNTM_UPDATEPREVIEW:
           if(WinSendMsg(hWndUpdate,BM_QUERYCHECK,0,0))
           {
               // Update the display window.
               mReturn = WinDefFontDlgProc(hWndDlg,ulMessage,mp1,mp2);
               set_font(pFontDlg->pszFamilyname);
               Fattrs = pFontDlg->fAttrs;
               return (mReturn);
           }
           break;

       case WM_COMMAND:
           switch (SHORT1FROMMP(mp1)) 
           {
               case DID_APPLY_BUTTON:
               case DID_OK_BUTTON:
                   WinDefFontDlgProc(hWndDlg,ulMessage,mp1,mp2);
                   set_font(pFontDlg->pszFamilyname);
                   Fattrs = pFontDlg->fAttrs;
                   return (0);
                   break;
           }
           break;
   }
   return (WinDefFontDlgProc(hWndDlg,ulMessage,mp1,mp2));
}  

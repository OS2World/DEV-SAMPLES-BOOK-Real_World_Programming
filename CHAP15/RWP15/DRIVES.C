/* --------------------------------------------------------------------
                                Drives 
                              Chapter 15

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_DOS
#define INCL_DOSDEVIOCTL
#define INCL_WIN
#define INCL_ERRORS

#include <os2.h>
#include <stdio.h>
#include <string.h>
#include "drives.h"

/* Local Functions    */
VOID DisplayDriveInfo (HWND,ULONG);
VOID FormatNumberField (HWND,USHORT,ULONG);
VOID QueryDiskSpace (ULONG,PULONG,PULONG,PULONG);
VOID QueryVolumeLabel (ULONG,PSZ);

/* External variables */
extern HAB      hab;                    /* Handle to anchor block       */

/* Global variables   */
DRIVEINFO DriveInfo[26];                /* Array of drive information   */
ULONG     ulNumDrives = 0L;             /* Number of valid drives       */

char szDriveDescription[11][16] =
    {
        "LD Floppy",
        "HD Floppy",
        "3.5"" Floppy",
        "8"" SD Floppy",
        "8"" HD Floppy",
        "Fixed Disk",
        "Tape Drive",
        "Unknown",
        "R/W Optical",
        "3.5"" 4MB Floppy",
        "Not Ready",
    };

/* ----------------------- Local Functions ----------------------- */

VOID DisplayDriveInfo (HWND hWnd, ULONG ulInx)
{
    CHAR  szVolumeLabel[12];
    ULONG ulTotalSpace,
          ulAllocated,
          ulAvailable;

    WinSetDlgItemText (hWnd, IDC_LOCATION, DriveInfo[ulInx].szLocation);
    WinSetDlgItemText (hWnd, IDC_REMOVABLE, DriveInfo[ulInx].szRemovable);
    WinSetDlgItemText (hWnd, IDC_FILESYSTEM, DriveInfo[ulInx].szFileSystem);
    WinSetDlgItemText (hWnd, IDC_DESCRIPTION, 
        szDriveDescription[DriveInfo[ulInx].ulDescriptionIndex]);

    WinSetPointer (HWND_DESKTOP, 
        WinQuerySysPointer (HWND_DESKTOP,SPTR_WAIT,FALSE));

    QueryVolumeLabel (DriveInfo[ulInx].szDrive[0] - 'A' + 1, szVolumeLabel);
    WinSetDlgItemText (hWnd, IDC_VOLUMELABEL, szVolumeLabel);

    QueryDiskSpace (DriveInfo[ulInx].szDrive[0] - 'A' + 1, &ulTotalSpace,
        &ulAllocated, &ulAvailable);

    FormatNumberField (hWnd, IDC_TOTALSPACE, ulTotalSpace);
    FormatNumberField (hWnd, IDC_ALLOCATED,  ulAllocated);
    FormatNumberField (hWnd, IDC_AVAILABLE,  ulAvailable);

    WinSetPointer (HWND_DESKTOP, 
        WinQuerySysPointer (HWND_DESKTOP,SPTR_ARROW,FALSE));

    return;
}

VOID FormatNumberField (HWND hWnd, USHORT usCtlID, ULONG ulNumber)
{
    CHAR  szNumber[13];

    sprintf (szNumber, "%12lu", ulNumber);

    WinSetDlgItemText (hWnd, usCtlID, szNumber);

    return;
}

VOID QueryDiskSpace (ULONG ulDriveNumber, PULONG pulTotalSpace,
                     PULONG pulAllocated, PULONG pulAvailable)
{
    FSALLOCATE fsAllocate;

    DosError (FERR_DISABLEHARDERR);

    if (!DosQueryFSInfo (ulDriveNumber, FSIL_ALLOC, &fsAllocate, sizeof(FSALLOCATE)))
    {
        *pulTotalSpace =
            fsAllocate.cSectorUnit * fsAllocate.cUnit * fsAllocate.cbSector;
        *pulAvailable  =
            fsAllocate.cSectorUnit * fsAllocate.cUnitAvail * fsAllocate.cbSector;
        *pulAllocated  = *pulTotalSpace - *pulAvailable;
    }
    else
        *pulTotalSpace = *pulAllocated = *pulAvailable  = 0L;

    DosError (FERR_ENABLEHARDERR);

    return;
}

VOID QueryVolumeLabel (ULONG ulDriveNumber, PSZ pszVolumeLabel)
{
    FSINFO fsInfo;

    DosError (FERR_DISABLEHARDERR);

    if (!DosQueryFSInfo (ulDriveNumber, FSIL_VOLSER, &fsInfo, sizeof(FSINFO)))
        strcpy (pszVolumeLabel, fsInfo.vol.szVolLabel);
    else
        pszVolumeLabel[0] = '\0';

    DosError (FERR_ENABLEHARDERR);

    return;
}

VOID QueryDrives (HWND hWnd)
{
    PFSQBUFFER2        pfsq2;
    ULONG              ulLen,
                       ulInx,
                       ulAction,
                       ulParmLen,
                       ulDataLen;
    APIRET             RetCode;
    HFILE              hFile;
    BIOSPARAMETERBLOCK bpb;
    CHAR               szDrive[3] = " :";
    BYTE               cBlock     = 0;

    WinSetPointer (HWND_DESKTOP, 
        WinQuerySysPointer (HWND_DESKTOP,SPTR_WAIT,FALSE));

    /* Allocate buffer */
    DosAllocMem ((PPVOID)&pfsq2, 1024L, fALLOC);

    DosError (FERR_DISABLEHARDERR);

    ulNumDrives = 0L;

    for (ulInx = 0; ulInx < 26; ulInx++)
    {
        szDrive[0] = (CHAR)('A' + ulInx);

        ulLen = 1024L;
        RetCode = DosQueryFSAttach (szDrive, 0L, FSAIL_QUERYNAME, pfsq2, &ulLen);

        DriveInfo[ulNumDrives].szDrive[0]         = szDrive[0];
        DriveInfo[ulNumDrives].szDrive[1]         = '\0';

        if (RetCode == ERROR_NOT_READY)
        {
            /* Assume local, removable, and FAT file system */
            strcpy (DriveInfo[ulNumDrives].szLocation,   "Local");
            strcpy (DriveInfo[ulNumDrives].szRemovable,  "Yes");
            strcpy (DriveInfo[ulNumDrives].szFileSystem, "FAT");
            DriveInfo[ulNumDrives].ulDescriptionIndex = 10L;
            ulNumDrives++;
        }
        else if (RetCode != ERROR_INVALID_DRIVE)
        {
            bpb.fsDeviceAttr = 0;

            /* Attempt to open the device */
            if (!DosOpen (szDrive, &hFile, &ulAction, 0L,
                FILE_NORMAL, FILE_OPEN, OPEN_FLAGS_DASD | OPEN_SHARE_DENYNONE, 0L))
            {
                ulParmLen = sizeof(BYTE);
                ulDataLen = sizeof(BIOSPARAMETERBLOCK);
                DosDevIOCtl (hFile, IOCTL_DISK, DSK_GETDEVICEPARAMS,
                    (PVOID)&cBlock, ulParmLen, &ulParmLen,
                    (PVOID)&bpb, ulDataLen, &ulDataLen);
                DosClose (hFile);
            }
            else
            {
                /* Remote drives may not allow themselves to be opened
                   with the OPEN_FLAGS_DASD access flag.  Default to
                   not removable and description of unknown. */
                bpb.fsDeviceAttr = 0x0001;
                bpb.bDeviceType  = 7;
            }

            /* Is the drive remote?      */
            if (pfsq2->iType == FSAT_REMOTEDRV)
                strcpy (DriveInfo[ulNumDrives].szLocation, "Remote");
            else
                strcpy (DriveInfo[ulNumDrives].szLocation, "Local");

            /* Is the drive removable?   */
            if (bpb.fsDeviceAttr & 0x0001)
                strcpy (DriveInfo[ulNumDrives].szRemovable, "No");
            else
                strcpy (DriveInfo[ulNumDrives].szRemovable, "Yes");
                                  
            /* Set the description index */
            if (bpb.bDeviceType < 10)
                DriveInfo[ulNumDrives].ulDescriptionIndex = (LONG)bpb.bDeviceType;
            else
                DriveInfo[ulNumDrives].ulDescriptionIndex = 7L;

            /* Set the file system name  */
            strncpy (DriveInfo[ulNumDrives].szFileSystem,
                       (PSZ)(pfsq2->szName + 1 + pfsq2->cbName), 15);

            ulNumDrives++;
        }
    }

    DosError (FERR_ENABLEHARDERR);

    DosFreeMem (pfsq2);

    /* Add items to the drive listbox */
    for (ulInx = 0; ulInx < ulNumDrives; ulInx++)
        WinSendDlgItemMsg (hWnd, IDC_DRIVELIST, LM_INSERTITEM, (MPARAM)LIT_END, 
            DriveInfo[ulInx].szDrive);
    WinSendDlgItemMsg (hWnd, IDC_DRIVELIST, LM_SELECTITEM, 0L, (MPARAM)TRUE);

    WinSetPointer (HWND_DESKTOP, 
        WinQuerySysPointer (HWND_DESKTOP,SPTR_ARROW,FALSE));
    return;
}

/* ----------------------  Dialog Function ----------------------- */

MRESULT EXPENTRY DrivesDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;

    switch (msg)
    {           
        case WM_INITDLG:
            QueryDrives (hWnd);
            break;

        case WM_CONTROL:
            switch (LOUSHORT(mp1))
            {
                case IDC_DRIVELIST:
                    if (HIUSHORT(mp1) == CBN_EFCHANGE)
                    {
                        DisplayDriveInfo (hWnd,
                            (ULONG)WinSendDlgItemMsg (hWnd, IDC_DRIVELIST, 
                                LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L));
                    }
                    break;
            }
            break;

        case WM_SYSCOMMAND:
		      switch (SHORT1FROMMP(mp1))
            {
				    case SC_CLOSE:
                    WinDismissDlg (hWnd, FALSE);
                    bHandled = TRUE;
                    break;
            }
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefDlgProc (hWnd, msg, mp1, mp2);

    return (mReturn);
}


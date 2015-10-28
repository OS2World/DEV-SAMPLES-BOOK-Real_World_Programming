/* --------------------------------------------------------------------
                      Phone Communications Program
                              Chapter 14

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define INCL_DOS
#define INCL_DOSDEVIOCTL
#define INCL_WIN
#define INCL_GPI
#define INCL_ERRORS

#include <os2.h>
#include <stdio.h>
#include "phone.h"
#include "..\common\controls.h"
#include "led.h"
#include "..\common\about.h"

/* Window and Dialog Functions      */
MRESULT EXPENTRY ClientWndProc   (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY SettingsDlgProc (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY StatusDlgProc   (HWND,ULONG,MPARAM,MPARAM);

/* Thread Functions                 */
VOID CommStatusThread (ULONG);
VOID ReceiveDataThread (ULONG);

/* Local Functions                  */
VOID ConnectToPort (HWND);
VOID DeInitializeControls (HWND);
VOID HangUpFromPort (HWND);
VOID InitializeAppWindow (HWND);
VOID InitializeControls (HWND,BOOL);
VOID PositionWindows (ULONG,ULONG);
VOID QueryBaudRate (ULONG);
VOID QueryDCBInfo (ULONG);
VOID QueryLineCtrl (ULONG);
VOID QueryValidCommPorts (VOID);
BOOL SetPortSettings (HWND);
VOID ToggleTransmit (HWND);
VOID TransmitLine (VOID);

/* User-Defined messages            */
#define WM_USER_RECEIVEQUEUE    WM_USER+1
#define WM_USER_TRANSMITQUEUE   WM_USER+2
#define WM_USER_COMSTATUS       WM_USER+3
#define WM_USER_TRANSMITSTATUS  WM_USER+4
#define WM_USER_COMEVENT        WM_USER+5
#define WM_USER_COMERROR        WM_USER+6
#define WM_USER_MODEMINPUT      WM_USER+7
#define WM_USER_MODEMOUTPUT     WM_USER+8
#define WM_USER_RCVLIST         WM_USER+9
#define WM_USER_RCVTEXT         WM_USER+10

/* Baud Rate Information */
typedef struct tagBAUDRATE
{
    CHAR  szTxt[6];            /* Text format of baud rate        */
    ULONG ulValue;             /* Numeric format of baud rate     */
} BAUDRATE;

/* Communications Port Information */
typedef struct tagPORTINFO
{
    ULONG       BaudIndex;
    ULONG       BaudRate;
    DCBINFO     DCBInfo;
    LINECONTROL LineControl;
} PORTINFO;

/* Global Variables                 */
HAB      hab;
HWND     hWndFrame, 
         hWndClient,
         hWndButtons,
         hWndStatus,
         hWndSettings,
         hWndLocalTitle,
         hWndLocalEdit,
         hWndLocalList,
         hWndLocalBevel1,
         hWndLocalBevel2,
         hWndRemoteTitle,
         hWndRemoteText,
         hWndRemoteList,
         hWndRemoteBevel1,
         hWndRemoteBevel2;
CHAR     szTitle[64];
TID      StatusThreadID;            /* Thread ID of CommStatus thread  */ 
TID      ReadThreadID;              /* Thread ID of ReceiveData thread */
HEV      hevRcvQueue;               /* Receive Queue Event Semaphore   */
HFILE    hFile;                     /* Open Comm Port file handle      */
CHAR     szCom[5]         = "COM ";
PORTINFO PortInfo[4];               /* Array of comm port settings     */
ULONG    ulPort           = 0;      /* Currently selected port index   */
ULONG    ulNumPorts       = 0;      /* Number of comm ports supported  */
BOOL     bConnected       = FALSE;  /* Communications currently open   */
BOOL     bTransmitting    = TRUE;   /* Data transmission enabled       */
CHAR     szReceiveBuffer[255];      /* Receive Queue buffer            */
USHORT   usNextPos = 0;             /* Next char pos in receive queue  */
ULONG    ulNumLocalLines  = 0L,     /* # of lines in Local listbox     */
         ulNumRemoteLines = 0L;     /* # of lines in Remote listbox    */

/* Current line and status flags */
RXQUEUE     ReceiveQueue    = { 0, 0 };
RXQUEUE     TransmitQueue   = { 0, 0 };
BYTE        ComStatus       = 0;
BYTE        TransmitStatus  = 0;
USHORT      ComEvent        = 0;
USHORT      ComError        = 0;
BYTE        ModemInput      = 0; 
MODEMSTATUS ModemOutput     = { 0, 0 };

/* Baud Rate Table */
BAUDRATE BaudRates[13] =
{
    { "110",     110L },
    { "150",     150L },
    { "300",     300L },
    { "600",     600L },
    { "1200",   1200L },
    { "1800",   1800L },
    { "2000",   2000L },
    { "2400",   2400L },
    { "3600",   3600L },
    { "4800",   4800L },
    { "7200",   7200L },
    { "9600",   9600L },
    { "19200", 19200L }
};

  
/* ----------------- Main Application Function -------------------- */

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST |
                            FCF_ICON     | FCF_NOBYTEALIGN;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, ClientWndProc, 0, 0);
    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    /* Create the main application window */
    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
        &flFrameFlags, szClientClass, szTitle, 0, 0, ID_APPNAME, &hWndClient);

    /* Create child windows */
    InitializeAppWindow (hWndClient);

    /* Show the main window in a maximized state */
    WinSetWindowPos (hWndFrame, 0, 0L, 0L, 0L, 0L, SWP_MAXIMIZE | SWP_SHOW);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}

/* ---------------- CommStatus Thread Function -------------------- */

/* 
   The CommStatus thread is responsible for querying the current
   communications line and status flags and determining if any
   of the flags have changed.  For each set of flags or values that
   has changed a message is posted to the main message queue thread
   so that it can update the LED controls.

   Access to the communication port file handle (hFile) is protected
   within a critical section.

   This is a Non-Message queue thread so it cannot send messages to
   a message queue thread. 
*/

VOID CommStatusThread (ULONG ulThreadParam)
{
    RXQUEUE     NewReceiveQueue,
                NewTransmitQueue;
    BYTE        NewComStatus,
                NewTransmitStatus,
                NewModemInput;
    USHORT      NewComEvent,
                NewComError;
    MODEMSTATUS NewModemOutput;

    do
    {
        /* Query all of the line and status flags */

        DosEnterCritSec ();

        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETCOMMEVENT,
            NULL, 0L, 0L, &NewComEvent, 2L, 0L);
        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETCOMMSTATUS,
            NULL, 0L, 0L, &NewComStatus, 1L, 0L);
        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETINQUECOUNT,
            NULL, 0L, 0L, &NewReceiveQueue, sizeof(RXQUEUE), 0L);
        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETOUTQUECOUNT,
            NULL, 0L, 0L, &NewTransmitQueue, sizeof(RXQUEUE), 0L);
        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETMODEMINPUT,
            NULL, 0L, 0L, &NewModemInput, 1L, 0L);
        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETMODEMOUTPUT,
            NULL, 0L, 0L, &NewModemOutput, sizeof(MODEMSTATUS), 0L);
        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETLINESTATUS,
            NULL, 0L, 0L, &NewTransmitStatus, 1L, 0L);
        DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETCOMMERROR,
            NULL, 0L, 0L, &NewComError, 2L, 0L);

        DosExitCritSec ();

        /* Determine if any values have changed -- if so then post message */
        if (NewReceiveQueue.cch != ReceiveQueue.cch)
        {
            ReceiveQueue = NewReceiveQueue;
            WinPostMsg (hWndStatus, WM_USER_RECEIVEQUEUE, 0L, 0L);
        }
        if (NewTransmitQueue.cch != TransmitQueue.cch)
        {
            TransmitQueue = NewTransmitQueue;
            WinPostMsg (hWndStatus, WM_USER_TRANSMITQUEUE, 0L, 0L);
        }
        if (NewComStatus != ComStatus)
        {
            ComStatus = NewComStatus;
            WinPostMsg (hWndStatus, WM_USER_COMSTATUS, 0L, 0L);
        }
        if (NewTransmitStatus != TransmitStatus)
        {
            TransmitStatus = NewTransmitStatus;
            WinPostMsg (hWndStatus, WM_USER_TRANSMITSTATUS, 0L, 0L);
        }
        if (NewComEvent != ComEvent)
        {
            ComEvent = NewComEvent;
            WinPostMsg (hWndStatus, WM_USER_COMEVENT, 0L, 0L);
        }
        if (NewComError != ComError)
        {
            ComError = NewComError;
            WinPostMsg (hWndStatus, WM_USER_COMERROR, 0L, 0L);
        }
        if (NewModemInput != ModemInput)
        {
            ModemInput = NewModemInput;
            WinPostMsg (hWndStatus, WM_USER_MODEMINPUT, 0L, 0L);
        }
        if (NewModemOutput.fbModemOn != ModemOutput.fbModemOn)
        {
            ModemOutput = NewModemOutput;
            WinPostMsg (hWndStatus, WM_USER_MODEMOUTPUT, 0L, 0L);
        }

        /* Go to sleep for 0.2 seconds */
        DosSleep(200);
    }
    while (TRUE);
}

/* --------------- ReceiveData Thread Function ------------------- */

/*
   The ReceiveData thread is responsible for reading the data from
   the communications port and placing the bytes into the Receive
   Buffer.  When a carriage return character (0x0D) is received a
   message is posted to the main message queue thread so that it
   can insert the line into the Remote listbox.  If all data has
   been processed from the communications port and it has not received
   the carriage return character then a message is posted to the
   main message queue thread so that it can update the Remote
   text window.  

   Access to the communication port file handle (hFile) is protected
   within a critical section.  In order to protect the receive queue
   buffer from being updated before the main message queue thread
   can process the posted message an event semaphore is used.  The
   event semaphore is reset prior to posting the message and this
   thread will wait for the main message queue thread to post the
   event semaphore before continuing.  

   This is a Non-Message queue thread so it cannot send messages to
   a message queue thread. 
*/

VOID ReceiveDataThread (ULONG ulThreadParam)
{
    CHAR   szBuffer[255];
    ULONG  ulBytesRead;
    ULONG  ulEventCnt;
    ULONG  ulPos;
    BOOL   bInsert;

    do
    {
        /* Read data from the communications port */
        ulBytesRead = 0L;

        DosEnterCritSec ();

        DosRead (hFile, szBuffer, (ULONG)sizeof(szBuffer), &ulBytesRead);

        DosExitCritSec ();

        if (ulBytesRead)
        {
            ulPos   = 0;
            bInsert = FALSE;

            while (ulPos < ulBytesRead)
            {
                switch (szBuffer[ulPos])
                {
                    case 0x0A:  /* Line feed */
                        ulPos++;
                        break;

                    case 0x0D:  /* Carriage return */
                        szReceiveBuffer[usNextPos] = '\0';
                        usNextPos = 0;
                        bInsert   = TRUE;
                        ulPos++;
                        break;

                    case 0x08:  /* Backspace */
                        if (usNextPos)
                            usNextPos--;
                        ulPos++;
                        break;

                    default:
                        if (usNextPos == 254)
                        {
                            szReceiveBuffer[usNextPos] = '\0';
                            usNextPos = 0;
                            bInsert = TRUE;
                        }
                        else
                            szReceiveBuffer[usNextPos++] = szBuffer[ulPos++];
                        break;
                }
                if (bInsert)
                {
                    /* Clear the event semaphore */
                    DosResetEventSem (hevRcvQueue, &ulEventCnt);

                    WinPostMsg (hWndClient, WM_USER_RCVLIST, 0L, 0L);

                    /* Wait for main message queue thread to process message */
                    DosWaitEventSem (hevRcvQueue, 1000);

                    bInsert = FALSE;
                }
            }

            /* Clear the event semaphore */
            DosResetEventSem (hevRcvQueue, &ulEventCnt);

            szReceiveBuffer[usNextPos] = '\0';
            WinPostMsg (hWndClient, WM_USER_RCVTEXT, 0L, 0L);

            /* Wait for main message queue thread to process message */
            DosWaitEventSem (hevRcvQueue, 1000);
        }

        /* Go to sleep for 0.1 seconds */
        DosSleep (100);
    }
    while (TRUE);
}


/* ----------------------- Local Functions ----------------------- */

VOID ConnectToPort (HWND hWnd)
{
    ULONG ulAction;

    szCom[3] = (CHAR)('1' + ulPort);
    if (!DosOpen (szCom, &hFile, &ulAction, 0L,
        FILE_NORMAL, FILE_OPEN, OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE, 0L))
    {
        if (!(bConnected = SetPortSettings (hWnd)))
            DosClose (hFile);

        /* Create the event semaphore for the receive queue */
        DosCreateEventSem ("\\sem32\\RCVQUEUE", &hevRcvQueue, 0, TRUE);

        /* Start the CommStatus and ReceiveData threads */
        DosCreateThread (&StatusThreadID, (PFNTHREAD)CommStatusThread, 0L, 0, 0x10000);
        DosCreateThread (&ReadThreadID, (PFNTHREAD)ReceiveDataThread, 0L, 0, 0x10000);

        /* Set the priority of the CommStatus thread to idle time */
        DosSetPriority (PRTYS_THREAD, PRTYC_IDLETIME, 0, StatusThreadID);
    }

    if (bConnected)
    {
        WinEnableControl (hWnd, IDC_CONNECT, FALSE);
        WinEnableControl (hWnd, IDC_HANGUP, TRUE);
        WinEnableWindow (hWndLocalEdit, TRUE); 
        WinSetFocus (HWND_DESKTOP, hWndLocalEdit);
    }
    else
        WinMessageBox (HWND_DESKTOP, hWnd,
            "Unable to connect", "Phone Application",
            0, MB_ERROR | MB_OK);
    
    return;
}

VOID DeInitializeControls (HWND hWnd)
{
    CHAR  szTxt[2];
    ULONG ulValue;

    /* Query values */
    PortInfo[ulPort].BaudIndex =
        (ULONG)WinSendDlgItemMsg (hWnd, IDC_BAUDRATE, SLM_QUERYSLIDERINFO,
            MPFROM2SHORT (SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE), 0L);
    PortInfo[ulPort].BaudRate = BaudRates[PortInfo[ulPort].BaudIndex].ulValue;
    if (WinQueryButtonCheckstate (hWnd, IDC_DATABITS_5))
        PortInfo[ulPort].LineControl.bDataBits = 5;
    else if (WinQueryButtonCheckstate (hWnd, IDC_DATABITS_6))
        PortInfo[ulPort].LineControl.bDataBits = 6;
    else if (WinQueryButtonCheckstate (hWnd, IDC_DATABITS_7))
        PortInfo[ulPort].LineControl.bDataBits = 7;
    else if (WinQueryButtonCheckstate (hWnd, IDC_DATABITS_8))
        PortInfo[ulPort].LineControl.bDataBits = 8;
    if (WinQueryButtonCheckstate (hWnd, IDC_STOPBITS_1))
        PortInfo[ulPort].LineControl.bStopBits = 0;
    else if (WinQueryButtonCheckstate (hWnd, IDC_STOPBITS_15))
        PortInfo[ulPort].LineControl.bStopBits = 1;
    else if (WinQueryButtonCheckstate (hWnd, IDC_STOPBITS_2))
        PortInfo[ulPort].LineControl.bStopBits = 2;

    if (WinQueryButtonCheckstate (hWnd, IDC_PARITY_NONE))
        PortInfo[ulPort].LineControl.bParity = 0;
    else if (WinQueryButtonCheckstate (hWnd, IDC_PARITY_ODD))
        PortInfo[ulPort].LineControl.bParity = 1;
    else if (WinQueryButtonCheckstate (hWnd, IDC_PARITY_EVEN))
        PortInfo[ulPort].LineControl.bParity = 2;
    else if (WinQueryButtonCheckstate (hWnd, IDC_PARITY_MARK))
        PortInfo[ulPort].LineControl.bParity = 3;
    else if (WinQueryButtonCheckstate (hWnd, IDC_PARITY_SPACE))
        PortInfo[ulPort].LineControl.bParity = 4;

    PortInfo[ulPort].DCBInfo.fbCtlHndShake = 0;
    PortInfo[ulPort].DCBInfo.fbFlowReplace = 0;
    if (WinQueryButtonCheckstate (hWnd, IDC_HANDSHAKING_DTRC))
        PortInfo[ulPort].DCBInfo.fbCtlHndShake |= MODE_DTR_CONTROL;
    if (WinQueryButtonCheckstate (hWnd, IDC_HANDSHAKING_DTRI))
        PortInfo[ulPort].DCBInfo.fbCtlHndShake |= MODE_DTR_HANDSHAKE;
    if (WinQueryButtonCheckstate (hWnd, IDC_HANDSHAKING_CTSO))
        PortInfo[ulPort].DCBInfo.fbCtlHndShake |= MODE_CTS_HANDSHAKE;
    if (WinQueryButtonCheckstate (hWnd, IDC_HANDSHAKING_DSRO))
        PortInfo[ulPort].DCBInfo.fbCtlHndShake |= MODE_DSR_HANDSHAKE;
    if (WinQueryButtonCheckstate (hWnd, IDC_HANDSHAKING_DCDO))
        PortInfo[ulPort].DCBInfo.fbCtlHndShake |= MODE_DCD_HANDSHAKE;
    if (WinQueryButtonCheckstate (hWnd, IDC_HANDSHAKING_DSRI))
        PortInfo[ulPort].DCBInfo.fbCtlHndShake |= MODE_DSR_SENSITIVITY;
   
    if (WinQueryButtonCheckstate (hWnd, IDC_HANDSHAKING_RTSI))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_RTS_HANDSHAKE;
    if (WinQueryButtonCheckstate (hWnd, IDC_FLOWCONTROL_XONXOFFT))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_AUTO_TRANSMIT;
    if (WinQueryButtonCheckstate (hWnd, IDC_FLOWCONTROL_XONXOFFR))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_AUTO_RECEIVE;
    if (WinQueryButtonCheckstate (hWnd, IDC_FLOWCONTROL_ERRORREP))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_ERROR_CHAR;
    if (WinQueryButtonCheckstate (hWnd, IDC_FLOWCONTROL_NULLSTRIP))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_NULL_STRIPPING;
    if (WinQueryButtonCheckstate (hWnd, IDC_FLOWCONTROL_BREAKREP))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_BREAK_CHAR;
    if (WinQueryButtonCheckstate (hWnd, IDC_FLOWCONTROL_RTSC))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_RTS_CONTROL;
    if (WinQueryButtonCheckstate (hWnd, IDC_FLOWCONTROL_TOGGLING))
        PortInfo[ulPort].DCBInfo.fbFlowReplace |= MODE_TRANSMIT_TOGGLE;
    WinQueryDlgItemText (hWnd, IDC_XONCHAR, 2, szTxt);
    PortInfo[ulPort].DCBInfo.bXONChar = szTxt[0];
    WinQueryDlgItemText (hWnd, IDC_XOFFCHAR, 2, szTxt);
    PortInfo[ulPort].DCBInfo.bXOFFChar = szTxt[0];
    WinQueryDlgItemText (hWnd, IDC_ERRORREPCHAR, 2, szTxt);
    PortInfo[ulPort].DCBInfo.bErrorReplacementChar = szTxt[0];
    WinQueryDlgItemText (hWnd, IDC_BREAKREPCHAR, 2, szTxt);
    PortInfo[ulPort].DCBInfo.bBreakReplacementChar = szTxt[0];

    WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_WRITE, SPBM_QUERYVALUE, 
       (MPARAM)&ulValue, (MPARAM)0L);
    PortInfo[ulPort].DCBInfo.usWriteTimeout = (USHORT)ulValue;
    WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_READ, SPBM_QUERYVALUE, 
       (MPARAM)&ulValue, (MPARAM)0L);
    PortInfo[ulPort].DCBInfo.usReadTimeout = (USHORT)ulValue;

    PortInfo[ulPort].DCBInfo.fbTimeout = (BYTE)(PortInfo[ulPort].DCBInfo.fbTimeout & 0x0F8);
    if (PortInfo[ulPort].DCBInfo.usWriteTimeout == 0)
        PortInfo[ulPort].DCBInfo.fbTimeout |= MODE_NO_WRITE_TIMEOUT;
    if (PortInfo[ulPort].DCBInfo.usReadTimeout == 0)
        PortInfo[ulPort].DCBInfo.fbTimeout |= MODE_NOWAIT_READ_TIMEOUT;
    else
        PortInfo[ulPort].DCBInfo.fbTimeout |= MODE_READ_TIMEOUT;

    return;
}

VOID HangUpFromPort (HWND hWnd)
{
    BYTE Cmd;

    /* Flush the input and output buffers */
    DosDevIOCtl (hFile, IOCTL_GENERAL, DEV_FLUSHOUTPUT, (PVOID)&Cmd, 1L,
        NULL, NULL, 0L, NULL);
    DosDevIOCtl (hFile, IOCTL_GENERAL, DEV_FLUSHINPUT, (PVOID)&Cmd, 1L,
        NULL, NULL, 0L, NULL);

    if (!DosClose (hFile))
    {
        WinEnableControl (hWnd, IDC_CONNECT, TRUE);
        WinEnableControl (hWnd, IDC_HANGUP,  FALSE);
        WinEnableWindow (hWndLocalEdit, FALSE); 

        /* Stop the CommStatus and ReceiveData threads */
        DosKillThread (StatusThreadID);
        DosKillThread (ReadThreadID);

        /* Close the receive data event semaphore */
        DosCloseEventSem (hevRcvQueue);

        bConnected = FALSE;               
    }
    else
        WinMessageBox (HWND_DESKTOP, hWnd,
            "Unable to hangup", "Phone Application",
            0, MB_ERROR | MB_OK);
 
    return;
}

VOID InitializeAppWindow (HWND hWnd)
{
	LONG lColor;

   RegisterLEDClass (hab);

   /* Local Connections Windows */

   hWndLocalTitle  = WinCreateWindow (hWnd, WC_STATIC, "Local Connection",
       WS_VISIBLE | SS_TEXT | DT_BOTTOM, 
       0L, 0L, 140L, 16L, hWnd, HWND_BOTTOM, 1, NULL, NULL);

   hWndLocalEdit   = WinCreateWindow (hWnd, WC_ENTRYFIELD, "",
       WS_VISIBLE | ES_AUTOSCROLL | WS_GROUP | WS_TABSTOP | WS_DISABLED, 
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 2, NULL, NULL);

   hWndLocalBevel1 = WinCreateWindow (hWnd, BEVELCLASS, "", 
       WS_VISIBLE | BVS_PREVWINDOW, 
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 0, NULL, NULL);
   WinSendMsg (hWndLocalBevel1, BVM_SETTHICKNESS, MPFROMSHORT(2), 0L);

   hWndLocalList   = WinCreateWindow (hWnd, WC_LISTBOX, "",
       WS_VISIBLE | LS_NOADJUSTPOS | LS_HORZSCROLL | WS_TABSTOP,
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 3, NULL, NULL);

   hWndLocalBevel2 = WinCreateWindow (hWnd, BEVELCLASS, "", 
       WS_VISIBLE | BVS_PREVWINDOW, 
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 0, NULL, NULL);
   WinSendMsg (hWndLocalBevel2, BVM_SETTHICKNESS, MPFROMSHORT(2), 0L);


   /* Remote Connections Windows */

   hWndRemoteTitle  = WinCreateWindow (hWnd, WC_STATIC, "Remote Connection",
       WS_VISIBLE | SS_TEXT | DT_BOTTOM, 
       0L, 0L, 140L, 16L, hWnd, HWND_BOTTOM, 4, NULL, NULL);

   hWndRemoteText   = WinCreateWindow (hWnd, WC_STATIC, "",
       WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER,
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 5, NULL, NULL);      

   hWndRemoteBevel1 = WinCreateWindow (hWnd, BEVELCLASS, "", 
       WS_VISIBLE | BVS_PREVWINDOW, 
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 0, NULL, NULL);
   WinSendMsg (hWndRemoteBevel1, BVM_SETTHICKNESS, MPFROMSHORT(2), 0L);

   hWndRemoteList  = WinCreateWindow (hWnd, WC_LISTBOX, "",
       WS_VISIBLE | LS_NOADJUSTPOS | LS_HORZSCROLL,
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 6, NULL, NULL);

   hWndRemoteBevel2 = WinCreateWindow (hWnd, BEVELCLASS, "", 
       WS_VISIBLE | BVS_PREVWINDOW, 
       0L, 0L, 0L, 0L, hWnd, HWND_BOTTOM, 0, NULL, NULL);
   WinSendMsg (hWndRemoteBevel2, BVM_SETTHICKNESS, MPFROMSHORT(2), 0L);


   /* Load the dialog windows */

   hWndButtons = WinLoadDlg (hWnd, hWnd, StatusDlgProc, 0L, IDD_BUTTONS, NULL);
   hWndStatus  = WinLoadDlg (hWnd, hWnd, StatusDlgProc, 0L, IDD_STATUS, NULL);


   /* Initialize settings of certain control windows */

   WinSendMsg (hWndLocalEdit, EM_SETTEXTLIMIT, MPFROMSHORT(254), 0L);

   lColor = CLR_DARKCYAN;
   WinSetPresParam (hWndLocalList, PP_BACKGROUNDCOLORINDEX, sizeof(lColor), &lColor);

   lColor = CLR_DARKRED;
   WinSetPresParam (hWndRemoteList, PP_BACKGROUNDCOLORINDEX, sizeof(lColor), &lColor);

   return;                                                           
}

VOID InitializeControls (HWND hWnd, BOOL bInit)
{
    ULONG ulInx;
    CHAR  szTxt[2] = " ";

    /* Set limits and attributes */
    if (bInit)
    {
        for (ulInx = 0; ulInx < 13; ulInx++)
        {
            WinSendDlgItemMsg (hWnd, IDC_BAUDRATE, SLM_SETTICKSIZE,
                MPFROM2SHORT (SMA_SETALLTICKS, 5), 0L);
            WinSendDlgItemMsg (hWnd, IDC_BAUDRATE, SLM_SETSCALETEXT,
                MPFROMSHORT(ulInx), MPFROMP(BaudRates[ulInx].szTxt));
        }
        WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_WRITE, SPBM_SETTEXTLIMIT, 
            (MPARAM)4, (MPARAM)0);
        WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_WRITE, SPBM_SETLIMITS,
            (MPARAM)9999L, (MPARAM)0L);
        WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_READ, SPBM_SETTEXTLIMIT, 
            (MPARAM)4, (MPARAM)0);
        WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_READ, SPBM_SETLIMITS,
            (MPARAM)9999L, (MPARAM)0L);
        WinSendDlgItemMsg (hWnd, IDC_XONCHAR, EM_SETTEXTLIMIT, MPFROMSHORT(1), 0L);
        WinSendDlgItemMsg (hWnd, IDC_XOFFCHAR, EM_SETTEXTLIMIT, MPFROMSHORT(1), 0L);
        WinSendDlgItemMsg (hWnd, IDC_ERRORREPCHAR, EM_SETTEXTLIMIT, MPFROMSHORT(1), 0L);
        WinSendDlgItemMsg (hWnd, IDC_BREAKREPCHAR, EM_SETTEXTLIMIT, MPFROMSHORT(1), 0L);

        for (ulInx = 0; ulInx < ulNumPorts; ulInx++)
        {
            szCom[3] = (CHAR)('1' + ulInx);
            WinSendDlgItemMsg (hWnd, IDC_PORT, LM_INSERTITEM, (MPARAM)LIT_END, szCom);
        }
        WinSendDlgItemMsg (hWnd, IDC_PORT, LM_SELECTITEM, (MPARAM)ulPort, (MPARAM)TRUE);
    }

    /* Set values */
    WinSendDlgItemMsg (hWnd, IDC_BAUDRATE, SLM_SETSLIDERINFO,
        MPFROM2SHORT (SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
        MPFROMSHORT(PortInfo[ulPort].BaudIndex));
    WinCheckButton (hWnd, IDC_DATABITS_5 + PortInfo[ulPort].LineControl.bDataBits - 5, TRUE);
    WinCheckButton (hWnd, IDC_STOPBITS_1 + PortInfo[ulPort].LineControl.bStopBits, TRUE);
    WinCheckButton (hWnd, IDC_PARITY_NONE + PortInfo[ulPort].LineControl.bParity, TRUE);
    WinCheckButton (hWnd, IDC_HANDSHAKING_DTRC, 
        PortInfo[ulPort].DCBInfo.fbCtlHndShake & MODE_DTR_CONTROL ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_HANDSHAKING_DTRI, 
        PortInfo[ulPort].DCBInfo.fbCtlHndShake & MODE_DTR_HANDSHAKE ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_HANDSHAKING_CTSO, 
        PortInfo[ulPort].DCBInfo.fbCtlHndShake & MODE_CTS_HANDSHAKE ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_HANDSHAKING_DSRO, 
        PortInfo[ulPort].DCBInfo.fbCtlHndShake & MODE_DSR_HANDSHAKE ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_HANDSHAKING_DCDO, 
        PortInfo[ulPort].DCBInfo.fbCtlHndShake & MODE_DCD_HANDSHAKE ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_HANDSHAKING_DSRI, 
        PortInfo[ulPort].DCBInfo.fbCtlHndShake & MODE_DSR_SENSITIVITY ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_HANDSHAKING_RTSI, 
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_RTS_HANDSHAKE ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_FLOWCONTROL_XONXOFFT,
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_AUTO_TRANSMIT ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_FLOWCONTROL_XONXOFFR,
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_AUTO_RECEIVE ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_FLOWCONTROL_ERRORREP,
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_ERROR_CHAR ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_FLOWCONTROL_NULLSTRIP,
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_NULL_STRIPPING ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_FLOWCONTROL_BREAKREP,
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_BREAK_CHAR ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_FLOWCONTROL_RTSC,
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_RTS_CONTROL ? TRUE : FALSE);
    WinCheckButton (hWnd, IDC_FLOWCONTROL_TOGGLING,
        PortInfo[ulPort].DCBInfo.fbFlowReplace & MODE_TRANSMIT_TOGGLE ? TRUE : FALSE);
    szTxt[0] = PortInfo[ulPort].DCBInfo.bXONChar;
    WinSetDlgItemText (hWnd, IDC_XONCHAR, szTxt);
    szTxt[0] = PortInfo[ulPort].DCBInfo.bXOFFChar;
    WinSetDlgItemText (hWnd, IDC_XOFFCHAR, szTxt);
    szTxt[0] = PortInfo[ulPort].DCBInfo.bErrorReplacementChar;
    WinSetDlgItemText (hWnd, IDC_ERRORREPCHAR, szTxt);
    szTxt[0] = PortInfo[ulPort].DCBInfo.bBreakReplacementChar;
    WinSetDlgItemText (hWnd, IDC_BREAKREPCHAR, szTxt);
    WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_WRITE, SPBM_SETCURRENTVALUE, 
        (MPARAM)PortInfo[ulPort].DCBInfo.usWriteTimeout, (MPARAM)0L);
    WinSendDlgItemMsg (hWnd, IDC_TIMEOUT_READ, SPBM_SETCURRENTVALUE, 
        (MPARAM)PortInfo[ulPort].DCBInfo.usReadTimeout, (MPARAM)0L);

    return;
}

VOID PositionWindows (ULONG ulWidth, ULONG ulHeight)
{
    PSWP  pSwp;
    RECTL Rectl;

    /* Allocate memory to hold the SWP structures */
    if (!DosAllocMem ((PPVOID)&pSwp, sizeof(SWP) * 8L, fALLOC))
    {
        /* Position buttons window */
        WinQueryWindowRect (hWndButtons, &Rectl);
        pSwp[0].fl   = SWP_MOVE;
        pSwp[0].x    = ulWidth - Rectl.xRight - 5;
        pSwp[0].y    = ulHeight - Rectl.yTop - 20;
        pSwp[0].hwnd = hWndButtons;

        /* Position status window */
        WinQueryWindowRect (hWndStatus, &Rectl);
        pSwp[1].fl   = SWP_MOVE;
        pSwp[1].x    = 0;
        pSwp[1].y    = 0;
        pSwp[1].hwnd = hWndStatus;
             	
        ulWidth      = pSwp[0].x - 5;
        ulHeight     = (ulHeight - Rectl.yTop) >> 1;

        /* Position the local title window */
        pSwp[2].fl   = SWP_MOVE;
        pSwp[2].x    = 5;
        pSwp[2].y    = Rectl.yTop + (ulHeight << 1) - 16;
        pSwp[2].hwnd = hWndLocalTitle;

        /* Size and position local edit window */
        pSwp[3].fl   = SWP_MOVE | SWP_SIZE;
        pSwp[3].x    = 8;
        pSwp[3].y    = pSwp[2].y - 20;
        pSwp[3].cx   = ulWidth - 16;
        pSwp[3].cy   = 16;
        pSwp[3].hwnd = hWndLocalEdit;

        /* Size and position local list window */
        pSwp[4].fl   = SWP_MOVE | SWP_SIZE;
        pSwp[4].x    = 8;
        pSwp[4].y    = Rectl.yTop + ulHeight + 8;
        pSwp[4].cx   = ulWidth - 16;
        pSwp[4].cy   = ulHeight - 50;
        pSwp[4].hwnd = hWndLocalList;

        /* Position the remote title window */
        pSwp[5].fl   = SWP_MOVE;
        pSwp[5].x    = 5;
        pSwp[5].y    = Rectl.yTop + ulHeight - 16;
        pSwp[5].hwnd = hWndRemoteTitle;

        /* Size and position remote text window */
        pSwp[6].fl   = SWP_MOVE | SWP_SIZE;
        pSwp[6].x    = 8;
        pSwp[6].y    = pSwp[5].y - 20;
        pSwp[6].cx   = ulWidth - 16;
        pSwp[6].cy   = 16;
        pSwp[6].hwnd = hWndRemoteText;

        /* Size and position remote list window */
        pSwp[7].fl   = SWP_MOVE | SWP_SIZE;
        pSwp[7].x    = 8;
        pSwp[7].y    = Rectl.yTop + 8;
        pSwp[7].cx   = ulWidth - 16;
        pSwp[7].cy   = ulHeight - 50;
        pSwp[7].hwnd = hWndRemoteList;

        /* Arrange the windows */
        WinSetMultWindowPos (hab, pSwp, 8L);

        /* Tell the bevel windows to resize themself since the window
           they were sized around has changed its size */
        WinSendMsg (hWndLocalBevel1,  BVM_RESIZE, MPFROM2SHORT(2,2), 0L);
        WinSendMsg (hWndLocalBevel2,  BVM_RESIZE, MPFROM2SHORT(2,2), 0L);
        WinSendMsg (hWndRemoteBevel1, BVM_RESIZE, MPFROM2SHORT(2,2), 0L);
        WinSendMsg (hWndRemoteBevel2, BVM_RESIZE, MPFROM2SHORT(2,2), 0L);

        /* Free the allocated memory    */
        DosFreeMem ((PVOID)pSwp);

    }

    return;
}

VOID QueryBaudRate (ULONG ulPort)
{
    DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETBAUDRATE,
          NULL, 0L, 0L, (PVOID)&PortInfo[ulPort].BaudRate, 2L, 0L);
    PortInfo[ulPort].BaudIndex = 12;
    while ((PortInfo[ulPort].BaudIndex > 0) &&
           (PortInfo[ulPort].BaudRate < BaudRates[PortInfo[ulPort].BaudIndex].ulValue))
        PortInfo[ulPort].BaudIndex--;

    return;
}

VOID QueryDCBInfo (ULONG ulPort)
{
   DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETDCBINFO,
         NULL, 0L, 0L, (PVOID)&PortInfo[ulPort].DCBInfo, sizeof(DCBINFO), 0L);

   return;
}

VOID QueryLineCtrl (ULONG ulPort)
{
   DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_GETLINECTRL,
         NULL, 0L, 0L, (PVOID)&PortInfo[ulPort].LineControl, sizeof(LINECONTROL), 0L);

   return;
}

VOID QueryValidCommPorts ()
{
    ULONG ulInx;
    ULONG ulAction;
    BYTE  cCommPorts;

    /* Determine the number of communication ports on the system */
    DosDevConfig (&cCommPorts, DEVINFO_RS232);
    ulNumPorts = (ULONG)cCommPorts;

    /* Attempt to query current com port settings */
    for (ulInx = 0; ulInx < ulNumPorts; ulInx++)
    {
        szCom[3] = (CHAR)('1' + ulInx);
        if (!DosOpen (szCom, &hFile, &ulAction, 0L, FILE_NORMAL, FILE_OPEN,
            OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE, 0L))
        {
            QueryBaudRate (ulInx);
            QueryDCBInfo (ulInx);
            QueryLineCtrl (ulInx);
            DosClose (hFile);
            PortInfo[ulInx].DCBInfo.usWriteTimeout        = 0;
            PortInfo[ulInx].DCBInfo.usReadTimeout         = 0;
        }
        else
        {
            /* Set to default values */
            PortInfo[ulInx].BaudIndex                     = 11;
            PortInfo[ulInx].BaudRate                      = 9600;
            PortInfo[ulInx].LineControl.bDataBits         = 5;
            PortInfo[ulInx].LineControl.bStopBits         = 0;
            PortInfo[ulInx].LineControl.bParity           = 0;
            PortInfo[ulInx].DCBInfo.fbCtlHndShake         = 0;
            PortInfo[ulInx].DCBInfo.fbFlowReplace         = 0;
            PortInfo[ulInx].DCBInfo.bXONChar              = 0x11;
            PortInfo[ulInx].DCBInfo.bXOFFChar             = 0x13;
            PortInfo[ulInx].DCBInfo.bErrorReplacementChar = ' ';
            PortInfo[ulInx].DCBInfo.bBreakReplacementChar = ' ';
            PortInfo[ulInx].DCBInfo.usWriteTimeout        = 0;
            PortInfo[ulInx].DCBInfo.usReadTimeout         = 0;
        }
    }

    return;
}

BOOL SetPortSettings (HWND hWnd)
{
   BOOL bSet =
       !DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_SETBAUDRATE,
           &PortInfo[ulPort].BaudRate, 2L, NULL, NULL, 0L, NULL) &&
       !DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_SETLINECTRL,
           &PortInfo[ulPort].LineControl, 3L, NULL, NULL, 0L, NULL) &&
       !DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_SETDCBINFO,
           &PortInfo[ulPort].DCBInfo, sizeof(DCBINFO), 0L, NULL, 0L, NULL);

   if (!bSet)
        WinMessageBox (HWND_DESKTOP, hWnd,
            "Unable to set port settings", "Phone Application",
            0, MB_ERROR | MB_OK);

   return (bSet);
}

VOID ToggleTransmit (HWND hWnd)
{
    /* Toggle the data transmission state */

    if (bTransmitting)
    {
        if (!DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_STOPTRANSMIT,
                NULL, 0L, NULL, NULL, 0L, NULL))
        {
            bTransmitting = FALSE;
            WinSetDlgItemText (hWnd, IDC_TRANSMIT, "Start");
        }
    }
    else
    {
        if (!DosDevIOCtl (hFile, IOCTL_ASYNC, ASYNC_STARTTRANSMIT,
                NULL, 0L, NULL, NULL, 0L, NULL))
        {
            bTransmitting = TRUE;
            WinSetDlgItemText (hWnd, IDC_TRANSMIT, "Stop");
        }
    }

    return;
}

VOID TransmitLine ()
{
    CHAR  szBuffer[255];
    ULONG ulLen;
    ULONG ulBytesWritten;

    WinSendDlgItemMsg (hWndStatus, IDC_LINESTATUS_WRT, WM_ENABLE, 
        MPFROMSHORT(TRUE), 0L);

    /* If data transmission is turned off then beep -- can't send the data */
    if (!bTransmitting)
    {
        DosBeep (500, 50);
        return;
    }
    
    /* Limit the number of lines in the listbox */
    if (ulNumLocalLines == 100)
    {
        WinSendMsg (hWndLocalList, LM_DELETEITEM, (MPARAM)99L, 0L);
        ulNumLocalLines--;
    }
    ulLen = WinQueryWindowText (hWndLocalEdit, sizeof(szBuffer)-2, szBuffer);
    szBuffer[ulLen++] = 0x0D;
    szBuffer[ulLen++] = 0x0A;

    /* Write data to the communications port */
    DosEnterCritSec ();

    if (!DosWrite (hFile, szBuffer, ulLen, &ulBytesWritten))
    {
        szBuffer[ulLen-2] = '\0';
        WinSendMsg (hWndLocalList, LM_INSERTITEM, 0L, (MPARAM)szBuffer);
        WinSetWindowText (hWndLocalEdit, "");
        ulNumLocalLines++;
    }

    DosExitCritSec ();

    WinSendDlgItemMsg (hWndStatus, IDC_LINESTATUS_WRT, WM_ENABLE, 
        MPFROMSHORT(FALSE), 0L);

    return;
}

/* ----------------- Window and Dialog Functions ----------------- */

MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    RECTL   Rectl;
    HPS     hps;

    switch (msg)
    {
        case WM_CREATE:
            {
     	          LONG RGBBack = 0x00CCCCCCL;
	             LONG RGBText = 0x00000000L;
                WinSetPresParam (hWnd, PP_FONTNAMESIZE, FONTNAMELEN, HELV8FONT);
                WinSetPresParam (hWnd, PP_BACKGROUNDCOLOR,sizeof(RGBBack),&RGBBack);
                WinSetPresParam (hWnd, PP_FOREGROUNDCOLOR,sizeof(RGBText),&RGBText);
                QueryValidCommPorts ();
            }
            break;

        case WM_USER_RCVLIST:
            /* Data in the receive buffer is a complete line -- insert it
               into the Remote listbox. */

            WinSendDlgItemMsg (hWndStatus, IDC_LINESTATUS_RED, WM_ENABLE, 
                MPFROMSHORT(TRUE), 0L);

            /* Limit the number of lines in the listbox */
            if (ulNumRemoteLines == 100)
            {
                WinSendMsg (hWndRemoteList, LM_DELETEITEM, (MPARAM)99L, 0L);
                ulNumRemoteLines--;
            }
            WinSendMsg (hWndRemoteList, LM_INSERTITEM, 0L, (MPARAM)szReceiveBuffer);
            ulNumRemoteLines++;

            /* Post the receive data event semaphore to notify the
               ReceiveData thread that we are done with szReceiveBuffer */
            DosPostEventSem (hevRcvQueue);

            WinSendDlgItemMsg (hWndStatus, IDC_LINESTATUS_RED, WM_ENABLE, 
                MPFROMSHORT(FALSE), 0L);
            break;

        case WM_USER_RCVTEXT:
            /* Data in the receive buffer is an incomplete line -- update
               the Remote text window */

            WinSendDlgItemMsg (hWndStatus, IDC_LINESTATUS_RED, WM_ENABLE, 
                MPFROMSHORT(TRUE), 0L);

            WinSetWindowText (hWndRemoteText, szReceiveBuffer);

            /* Post the receive data event semaphore to notify the
               ReceiveData thread that we are done with szReceiveBuffer */
            DosPostEventSem (hevRcvQueue);

            WinSendDlgItemMsg (hWndStatus, IDC_LINESTATUS_RED, WM_ENABLE, 
                MPFROMSHORT(FALSE), 0L);
            break;

        case WM_PAINT:
            hps = WinBeginPaint (hWnd,0,0);
            WinQueryWindowRect (hWnd, &Rectl);
            WinFillRect (hps, &Rectl, CLR_PALEGRAY);
            WinEndPaint (hps);
            break;

        case WM_CONTROL:
            /* If we get a doubleclick or enter from the Local listbox
               then copy the text from the listbox to the Local edit window */

            if ( bConnected &&
                 (SHORT2FROMMP(mp1) == LN_ENTER) &&
                 ((HWND)mp2 == hWndLocalList) )
            {
                SHORT sIndex;
                CHAR  szTxt[255];

                sIndex = (SHORT)WinSendMsg (hWndLocalList, LM_QUERYSELECTION, 
                             (MPARAM)LIT_FIRST, 0L);
                if (sIndex != LIT_NONE)
                {
                    WinSendMsg (hWndLocalList, LM_QUERYITEMTEXT, 
                        MPFROM2SHORT(sIndex,255), szTxt);
                    WinSetWindowText (hWndLocalEdit, szTxt);
                    WinSetFocus (HWND_DESKTOP, hWndLocalEdit);
                }
            }
            break;

        case WM_CHAR:
            /* If the enter key is pressed in either the Local edit or
               Local listbox then transmit the data */

            if ( bConnected && 
                 (WinQueryFocus (HWND_DESKTOP) == hWndLocalEdit) &&
                 !(SHORT1FROMMP(mp1) & KC_KEYUP) &&
                 (SHORT1FROMMP(mp1) & KC_VIRTUALKEY) &&
                 ( (SHORT2FROMMP(mp2) == VK_ENTER)  ||
                   (SHORT2FROMMP(mp2) == VK_NEWLINE) ) )
                TransmitLine ();
            break;

        case WM_SIZE:
            PositionWindows ((ULONG)SHORT1FROMMP(mp2), (ULONG)SHORT2FROMMP(mp2));
            break;

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}

MRESULT EXPENTRY StatusDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mReturn  = 0L;
    BOOL    bHandled = TRUE;

    switch (msg)
    {
        case WM_INITDLG:
            CenterWindow (hWnd);
            break;

        case WM_USER_RECEIVEQUEUE:
            WinSetDlgItemShort (hWnd, IDC_QUEUE_IN, ReceiveQueue.cch, FALSE);
            break;

        case WM_USER_TRANSMITQUEUE: 
            WinSetDlgItemShort (hWnd, IDC_QUEUE_IN, TransmitQueue.cch, FALSE);
            break;

        case WM_USER_COMSTATUS:
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_CTS, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_FOR_CTS), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_DST, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_FOR_DSR), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_DCD, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_FOR_DCD), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_XON, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_FOR_XON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_XOF, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_TO_SEND_XON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_BRK, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_WHILE_BREAK_ON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_IMM, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_TO_SEND_IMM), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMSTATUS_DSR, WM_ENABLE, 
                MPFROMSHORT(ComStatus & TX_WAITING_FOR_DSR), 0L);
            break;

        case WM_USER_TRANSMITSTATUS:
            WinSendDlgItemMsg (hWnd, IDC_LINESTATUS_WRQ, WM_ENABLE, 
                MPFROMSHORT(TransmitStatus & WRITE_REQUEST_QUEUED), 0L);
            WinSendDlgItemMsg (hWnd, IDC_LINESTATUS_DTQ, WM_ENABLE, 
                MPFROMSHORT(TransmitStatus & DATA_IN_TX_QUE), 0L);
            WinSendDlgItemMsg (hWnd, IDC_LINESTATUS_HTX, WM_ENABLE, 
                MPFROMSHORT(TransmitStatus & HARDWARE_TRANSMITTING), 0L);
            WinSendDlgItemMsg (hWnd, IDC_LINESTATUS_CRS, WM_ENABLE, 
                MPFROMSHORT(TransmitStatus & CHAR_READY_TO_SEND_IMM), 0L);
            WinSendDlgItemMsg (hWnd, IDC_LINESTATUS_XON, WM_ENABLE, 
                MPFROMSHORT(TransmitStatus & WAITING_TO_SEND_XON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_LINESTATUS_XOF, WM_ENABLE, 
                MPFROMSHORT(TransmitStatus & WAITING_TO_SEND_XOFF), 0L);
            break;

        case WM_USER_COMEVENT:
            WinSendDlgItemMsg (hWnd, IDC_COMMEVENT_CTS, WM_ENABLE, 
                MPFROMSHORT(ComEvent & CTS_CHANGED), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMEVENT_DSR, WM_ENABLE, 
                MPFROMSHORT(ComEvent & DSR_CHANGED), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMEVENT_DCD, WM_ENABLE, 
                MPFROMSHORT(ComEvent & DCD_CHANGED), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMEVENT_BRK, WM_ENABLE, 
                MPFROMSHORT(ComEvent & BREAK_DETECTED), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMEVENT_ERR, WM_ENABLE, 
                MPFROMSHORT(ComEvent & ERROR_OCCURRED), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMEVENT_RI, WM_ENABLE, 
                MPFROMSHORT(ComEvent & RI_DETECTED), 0L);
            break;

        case WM_USER_COMERROR:      
            WinSendDlgItemMsg (hWnd, IDC_COMMERROR_QOV, WM_ENABLE, 
                MPFROMSHORT(ComError & RX_QUE_OVERRUN), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMERROR_HOV, WM_ENABLE, 
                MPFROMSHORT(ComError & RX_HARDWARE_OVERRUN), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMERROR_PAR, WM_ENABLE, 
                MPFROMSHORT(ComError & PARITY_ERROR), 0L);
            WinSendDlgItemMsg (hWnd, IDC_COMMERROR_FRM, WM_ENABLE, 
                MPFROMSHORT(ComError & FRAMING_ERROR), 0L);
            break;

        case WM_USER_MODEMINPUT:    
            WinSendDlgItemMsg (hWnd, IDC_MODEM_CTS, WM_ENABLE, 
                MPFROMSHORT(ModemInput & CTS_ON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_MODEM_CTS, WM_ENABLE, 
                MPFROMSHORT(ModemInput & CTS_ON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_MODEM_DSR, WM_ENABLE, 
                MPFROMSHORT(ModemInput & DSR_ON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_MODEM_RI, WM_ENABLE, 
                MPFROMSHORT(ModemInput & RI_ON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_MODEM_DCD, WM_ENABLE, 
                MPFROMSHORT(ModemInput & DCD_ON), 0L);
            break;

        case WM_USER_MODEMOUTPUT:
            WinSendDlgItemMsg (hWnd, IDC_MODEM_DTR, WM_ENABLE, 
                MPFROMSHORT(ModemOutput.fbModemOn & DTR_ON), 0L);
            WinSendDlgItemMsg (hWnd, IDC_MODEM_RTS, WM_ENABLE, 
                MPFROMSHORT(ModemOutput.fbModemOn & RTS_ON), 0L);
            break;

		  case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case IDC_CONNECT:
                    ConnectToPort (hWnd);
                    break;

                case IDC_HANGUP:
                    HangUpFromPort (hWnd);
                    break;

                case IDC_SETTINGS:
                    WinDlgBox (HWND_DESKTOP, hWnd, SettingsDlgProc,
                            0L, IDD_SETTINGS, NULL);
                    break;

                case IDC_TRANSMIT:
                    ToggleTransmit (hWnd);
                    break;

                case IDC_ABOUT:
                    DisplayAbout (hWnd, szTitle);
                    break;
            }
            if (bConnected)
                WinSetFocus (HWND_DESKTOP, hWndLocalEdit);
            break;

        case WM_DESTROY:
            if (bConnected)
                HangUpFromPort (hWndButtons);
            break;

        default:
           bHandled = FALSE;
           break;
    }

    if (!bHandled)
        mReturn = WinDefDlgProc (hWnd, msg, mp1, mp2);

    return (mReturn);
}

MRESULT EXPENTRY SettingsDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mReturn  = 0;
    BOOL    bHandled = TRUE;

    switch (msg)
    {
        case WM_INITDLG:
            CenterWindow (hWnd);
            InitializeControls (hWnd, TRUE);
            if (bConnected)
                WinEnableControl (hWnd, IDC_PORT, FALSE);
            break;   

        case WM_CONTROL:
            switch (LOUSHORT(mp1))
            {
                case IDC_PORT:
                    if (HIUSHORT(mp1) == CBN_EFCHANGE)
                    {
                        ulPort = (ULONG)WinSendDlgItemMsg (hWnd, IDC_PORT, 
                            LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);
                        InitializeControls (hWnd, FALSE);
                    }
                    break;
            }
            break;

		  case WM_COMMAND:
		      switch (LOUSHORT(mp1))
            {
                case DID_OK:
                    DeInitializeControls (hWnd);
                    if (bConnected)
                        SetPortSettings (hWnd);

                /* FALL THROUGH */

                case DID_CANCEL:
                    WinDismissDlg (hWnd, DID_OK);
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

              

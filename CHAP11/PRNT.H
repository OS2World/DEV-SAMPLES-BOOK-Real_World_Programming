/* --------------------------------------------------------------------
                         Print Sample Header File
                               Chapter 11

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* Stringtable ID's */
#define ID_APPNAME            1
#define ID_NOQUEUE_ERROR      2
#define ID_PRIORITY_HIGH      3
#define ID_PRIORITY_DEF       4
#define ID_PRIORITY_MIN       5
#define ID_STATUS_HELD        6
#define ID_STATUS_PEND        7
#define ID_STATUS_NORM        8
#define ID_CAPS_CURRENT       9
#define ID_CAPS_SELECTABLE    10
#define ID_CAPS_NONE          11
#define ID_TECH_BASE          12
#define ID_TECH_UNKNOWN       ID_TECH_BASE  
#define ID_TECH_PLOTTER       ID_TECH_BASE + 1
#define ID_TECH_RAST_DISP     ID_TECH_BASE + 2
#define ID_TECH_RAST_PRT      ID_TECH_BASE + 3
#define ID_TECH_RAST_CAM      ID_TECH_BASE + 4
#define ID_TECH_POSTSCRIPT    ID_TECH_BASE + 5
#define ID_ERR_NODEF_PRINTERS 18                                            
#define ID_ERR_FAIL_DEV_QUERY 19

/* Menu ID's */
#define IDM_FILE              100
#define IDM_CHOOSE_PRINTER    101
#define IDM_PRINT             102
#define IDM_PRINT_THREAD      103
#define IDM_ABOUT             104

#define IDM_DISPLAY           200
#define IDM_DISP_BLANK        201
#define IDM_DISP_BOX          202
#define IDM_DISP_ELLIPSE      203
#define IDM_DISP_X            204
#define IDM_PRINTER_INFO      205

/* Help hint string ID's */
#define IDS_JOB_TITLE         105

#define MAX_BUFF              128
#define MAX_DEVICENAME         32
#define MAX_QUEUENAME          10
#define MAX_QUEUEDESC          64
#define MAX_DRIVERNAME        128

#define QUEUENAME_LENGTH    10
#define QUEUEDESC_LENGTH    64
#define DEVICENAME_LENGTH   32
#define DRIVERNAME_LENGTH   128

#define WM_START_PRINT      WM_USER
#define WM_ABORT_PRINT      WM_USER+1
#define WM_END_PRINT        WM_USER+2
#define WM_PRINT_STATUS     WM_USER+3

#define JOB_STARTED      100
#define JOB_CANCELED     101
#define JOB_FINISHED     102
#define JOB_FAILED       103

//#define PAGE_OFFSET      40
#define PAGE_OFFSET      10

typedef struct DEVICEINFO
{
    HDC       hInfoDC;
    HPS       hInfoPS;
    HDC       hPrintDC;
    HPS       hPrintPS;
    HDC       hClientDC;         
    HPS       hClientPS;        
    SIZEL     PageSizel;   
    SIZEL     ClientSizel;
    SIZEL     PhyPageSizel;
    SIZEL     PrintOffsetSizel;    
    LONG      lCopies;
    LONG      lPriority;
    CHAR      szQueueDesc[MAX_QUEUEDESC];
    CHAR      szQueueName[MAX_QUEUENAME];
    CHAR      szDriverName[MAX_DRIVERNAME];
    LONG      lSizeDD;
    PDRIVDATA pDrivData;
    PHCINFO   pHCInfo;
    LONG      lSizeHC;
    int       cx;
    int       cy;
} DEVICEINFO;
typedef DEVICEINFO *PDEVICEINFO;


typedef struct DDINFO
{
    LONG      lSizeDD;
    PDRIVDATA pDrivData;
} DDINFO;
typedef DDINFO *PDDINFO;

VOID      APIENTRY center_window(HWND);
BOOL      APIENTRY create_info_DC(PPRQINFO3,PDEVICEINFO);
BOOL      APIENTRY create_print_DC(PDEVICEINFO);
void      APIENTRY create_status_line(HWND);
void      APIENTRY display_hint(SHORT);
void      APIENTRY draw_page(HPS);
void      APIENTRY draw_page_background(HPS);
PPRQINFO3 APIENTRY enum_print_queues(HWND,HWND,PULONG,PULONG,PULONG);
void      APIENTRY free_info_DCs(PDEVICEINFO,ULONG);
VOID      APIENTRY get_job_properties(PPRQINFO3,PDDINFO,ULONG);
VOID      APIENTRY get_page_metrics(PDEVICEINFO);
HPS       APIENTRY init_app(HWND);
USHORT    APIENTRY post_error(HWND,USHORT);
void      APIENTRY process_print(HWND);
void      APIENTRY process_size(MPARAM,MPARAM);
void      APIENTRY process_command(HWND,MPARAM,MPARAM);
void      APIENTRY query_device_info(HWND,PPRQINFO3,PDEVICEINFO);
void      APIENTRY query_form_info(HWND,PHCINFO);

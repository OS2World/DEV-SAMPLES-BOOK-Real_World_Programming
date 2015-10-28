/* --------------------------------------------------------------------
                      Common Dialogs Header File
                              Chapter 3

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define MAX_BUFF   128
#define MAX_DRIVES  26
#define ID_APPNAME   1

#define MAIN_MENU 1

#define IDM_OPEN1   10
#define IDM_OPEN2   11
#define IDM_OPEN3   12
#define IDM_SAVEAS1 13
#define IDM_ABOUT   14

#define IDM_FONT1   20
#define IDM_FONT2   21
#define IDM_FONT3   22

#define IDS_APPNAME         1
#define IDS_OPEN            2
#define IDS_SAVE            3
#define IDS_FONT            4
#define IDS_DASH            5
#define IDS_PATH            6
#define IDS_UPDATE          7
#define IDS_OPEN_ERROR      8
#define IDS_MEMORY_ERROR    9
#define IDS_READ_ERROR     10
#define IDS_FONTDLG_ERROR  11

#define   OPEN_DIALOG                  10
#define   DID_FILENAME_TXT            257
#define   DID_FILENAME_ED             258
#define   DID_DRIVE_TXT               259
#define   DID_DRIVE_CB                260
#define   DID_FILTER_TXT              261
#define   DID_FILTER_CB               262
#define   DID_DIRECTORY_TXT           263
#define   DID_DIRECTORY_LB            264
#define   DID_FILES_TXT               265
#define   DID_FILES_LB                266
#define   DID_HELP_PB                 267
#define   DID_APPLY_PB                268
#define   DID_OK_PB                   DID_OK
#define   DID_CANCEL_PB               DID_CANCEL

#define CLIENT_FONT 2
#define DID_SAVE_DIR     WM_USER
#define DID_UPDATE_FONT  WM_USER

HWND    APIENTRY add_checkbox(HWND);
void    APIENTRY cleanup(void);
void    APIENTRY get_fixed_drives(void);
void    APIENTRY load_file(PSZ);
USHORT  APIENTRY post_error(HWND,USHORT);
void    APIENTRY process_command(HWND,MPARAM,MPARAM);
void    APIENTRY set_font(PSZ);
void    APIENTRY set_path(PSZ,USHORT);
void    APIENTRY set_title(PSZ,USHORT);

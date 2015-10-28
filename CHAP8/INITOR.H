/* --------------------------------------------------------------------
                         Initor Header File
                              Chapter 8

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define ID_ADD          10
#define ID_DISPLAY      11
#define ID_DELETE       12
#define ID_SAVE         13
#define ID_EXIT         14
#define ID_FIRST_BUTTON ID_ADD
#define ID_LAST_BUTTON  ID_EXIT
#define NUM_BUTTONS     5
#define NUM_MENUS       4

#define ID_APPNAME       51
#define ID_APPLICATION   52
#define ID_KEY           53
#define ID_DATA          54
#define ID_HINIUSER      55
#define ID_HINISYS       56
#define ID_HINIBOTH      57
#define ID_HINIPRIVATE   58 
#define ID_DATA_HEADING  59
#define ID_TEST_DATA     60
#define ID_INITOR        61
#define ID_LOCATION      62
#define ID_WRITE_ERROR   63
#define ID_DELETE_ERROR  64
#define ID_SAVEPOS_ERROR 65
#define ID_QTEXT_ERROR   66

#define ADD_MENU       100
#define DISPLAY_MENU   101
#define DELETE_MENU    102
#define SAVE_MENU      103
#define EXIT_MENU      104
#define FIRST_MENU     ADD_MENU

#define IDM_DISPLAY_USER      200
#define IDM_DISPLAY_SYS       201
#define IDM_DISPLAY_USERSYS   202
#define IDM_DISPLAY_PRIVATE   203
#define IDM_REFRESH           204
#define IDM_ABOUT             205
#define DISPLAY_FIRST         IDM_DISPLAY_USER

#define IDM_DELETE_APP        300
#define IDM_DELETE_KEY        301

#define IDM_ADD_APP           400
#define IDM_ADD_KEY           401

#define IDM_SAVE_CHANGES      500
#define IDM_SAVE_LOCATION     501

#define MAX_TITLE     1024
#define MAX_BUFF      128
#define MAX_LINE      16

#define APP_LBOX      1
#define KEY_LBOX      2
#define DATA_WND      3

#define HEX_SIZE      47
#define HEX_START     7
#define ASCII_START   HEX_START + HEX_SIZE + 1


#define WM_UPDATE_DATA WM_USER + 1

#define FIXED_FONT_LCID 5

#define ADD_DLG                     1000
#define DID_APP_NAME                1001
#define DID_KEY_NAME                1002
#define DID_KEY_DATA                1003


#define WM_UPDATE_TITLE             WM_USER

void   APIENTRY change_ini_file(USHORT);
void   APIENTRY cleanup(void);
void   APIENTRY create_controls(HWND);
void   APIENTRY enum_app_names(HINI,USHORT);
void   APIENTRY enum_key_name(PSZ);
int    APIENTRY format_line(PSZ,PSZ,int,int);
void   APIENTRY get_key_data(PSZ,PSZ);
HPS    APIENTRY init_data_window(HWND);
int    APIENTRY paint_data_window(HWND,HPS,int);
USHORT APIENTRY post_error(HWND,USHORT);
void   APIENTRY process_command(HWND,MPARAM,MPARAM);
void   APIENTRY process_control(HWND,MPARAM,MPARAM);
void   APIENTRY refresh_display(void);
void   APIENTRY size_controls(MPARAM);

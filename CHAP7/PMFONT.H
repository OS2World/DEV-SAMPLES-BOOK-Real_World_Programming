/* --------------------------------------------------------------------
                        PM Font Resource File
                              Chapter 7

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define ID_APPNAME         1


#define IDM_DISPLAY       300
#define IDM_METRICS       301
#define IDM_ROTATION      302
#define IDM_ABOUT         303

#define IDM_CHMODE        400
#define IDM_CHMODE_ONE    401
#define IDM_CHMODE_TWO    402
#define IDM_CHMODE_THREE  403

#define IDM_ATTRIBUTE       550
#define IDM_ATTR_ITALIC     551
#define IDM_ATTR_BOLD       552
#define IDM_ATTR_UNDERSCORE 553
#define IDM_ATTR_STRIKEOUT  554
#define IDM_ATTR_OUTLINE    555

#define IDM_CODEPAGE      600
#define IDM_CP037         037
#define IDM_CP273         273
#define IDM_CP274         274
#define IDM_CP277         277
#define IDM_CP278         278
#define IDM_CP280         280
#define IDM_CP282         282
#define IDM_CP284         284
#define IDM_CP285         285
#define IDM_CP297         297
#define IDM_CP437         437
#define IDM_CP850         850
#define IDM_CP852         852
#define IDM_CP857         857
#define IDM_CP860         860
#define IDM_CP861         861
#define IDM_CP863         863
#define IDM_CP865         865
#define IDM_CP1004       1004


#define IDS_APPNAME         1
#define IDS_DISPLAY_TEXT    2
#define IDS_SPECIAL_TEXT    3
#define IDS_USER_TEXT       4

#define DEFAULT_SIZE 30
#define MAX_BUFF   128

#define DISPLAY_FONT 1
#define MAX_WIDTHS   9
#define MAX_STR      40
#define TWO_PIE (2.0 * 3.14159)  

#define WM_UPDATE_METRICS  WM_USER + 1

void    APIENTRY cleanup(void);
BOOL    APIENTRY create_controls (HWND);
BOOL    APIENTRY create_font(HWND,int,PFONTMETRICS);
void    APIENTRY draw_grid(HPS,PRECTL);
BOOL    APIENTRY init_print_info (void);
BOOL    APIENTRY init_sliders_n_spinners(HWND);
PFONTMETRICS  APIENTRY init_font_LBox (HWND,HPS);
USHORT  APIENTRY post_error(HWND,USHORT);
void    APIENTRY print_selected_font (void);
void    APIENTRY process_paint(HWND,HPS);
void    APIENTRY process_command(HWND,MPARAM,MPARAM);
void    APIENTRY process_control(HWND,HPS,MPARAM,MPARAM);
void    APIENTRY size_controls(MPARAM);


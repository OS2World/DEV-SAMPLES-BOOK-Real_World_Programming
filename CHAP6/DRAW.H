/* --------------------------------------------------------------------
                        GPI Drawing Header File
                              Chapter 6

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define ID_APPNAME     1
#define IDS_APPNAME    1
#define IDS_TIMERON    2
#define IDS_TIMEROFF   3
#define IDS_NOB_WIND   4
#define IDS_BOUND_WIND 5
#define IDS_NOB_ALT    6
#define IDS_BOUND_ALT  7

#define ID_STAR       50

#define IDM_DRAW              100
#define IDM_THERM             101
#define IDM_TIMER             102
#define IDM_ABOUT             103
#define IDM_AREAS             200
#define IDM_AREA_STARS        201
#define IDM_POLY_STARS        202
#define IDM_PATHS             300
#define IDM_FILL_PATH         301
#define IDM_OUTLINE_PATH      302
#define IDM_CONVERT_ALT_PATH  303
#define IDM_CONVERT_WIND_PATH 304
#define IDM_STROKE_PATH       305
#define IDM_CLIP_PATH         306
#define IDM_CLIP_STROKE_PATH  307
#define IDM_BEVEL             308
#define IDM_ROUND             309
#define IDM_MITRE             310
#define IDM_FLAT_END          311
#define IDM_ROUND_END         312
#define IDM_SQUARE_END        313


#define MAX_STRING 80

void  APIENTRY calculate_points(MPARAM);
void  APIENTRY process_command(HWND,MPARAM,MPARAM);
void  APIENTRY process_paint(HWND);


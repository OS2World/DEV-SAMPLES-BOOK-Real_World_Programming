/* --------------------------------------------------------------------
                         Memory Header File
                              Chapter 9

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define ID_APPNAME              1
#define IDD_ALLOCATE            2

#define IDL_MEMLIST            10
#define IDL_PAGELIST           11
#define IDB_ALLOCATE           12
#define IDB_FREE              100
#define IDB_SELECT            200
#define IDB_STATE             300
#define IDB_RIGHTS            400

#define IDB_NUMBYTES           20
#define IDB_COMMITTED          21
#define IDB_UNCOMMITTED        22
#define IDB_SHARED             23
#define IDB_NONSHARED          24
#define IDB_READ               25
#define IDB_WRITE              26
#define IDB_EXECUTE            27
#define IDB_TILED              28
#define IDB_REPEAT             29

#define FREE_MENU             100
#define SELECT_MENU           200
#define STATE_MENU            300
#define RIGHTS_MENU           400

#define IDM_FREE_SELECTED     101
#define IDM_FREE_ALL          102
#define IDM_SELECT_ALL        201
#define IDM_SELECT_NONE       202
#define IDM_COMMIT_SELECTED   301
#define IDM_DECOMMIT_SELECTED 302
#define IDM_READ              401
#define IDM_WRITE             402
#define IDM_READWRITE         403
#define IDM_EXECUTE           404
#define IDM_READEXECUTE       405
#define IDM_WRITEEXECUTE      406
#define IDM_READWRITEEXECUTE  407
#define IDM_ABOUT             408

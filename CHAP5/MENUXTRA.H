/* --------------------------------------------------------------------
                Undocumented PM Menu Messages Header File
                              Chapter 5

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#define WM_MENUCHAR                 0x003f

#define MM_SELECTITEMBYPOS          0x019e

#define MM_MATCHMNEMONIC            0x01f0
#define MM_DELETEITEMBYPOS          0x01f1
#define MM_REMOVEITEMBYPOS          0x01f2
#define MM_QUERYITEMBYPOS           0x01f3
#define MM_SETITEMBYPOS             0x01f4
#define MM_QUERYITEMATTRBYPOS       0x01f5
#define MM_SETITEMATTRBYPOS         0x01f6
#define MM_QUERYITEMTEXTBYPOS       0x01f7
#define MM_QUERYITEMTEXTLENGTHBYPOS 0x01f8
#define MM_SETITEMTEXTBYPOS         0x01f9
#define MM_SETITEMHANDLEBYPOS       0x01fa
#define MM_PORTHOLEINIT             0x01fb
#define MM_SETITEMCHECKMARKBYPOS    0x01fc

#define MM_SETITEMCHECKMARK         0x0210

#define WM_MENUHELP                 0x0433

#define WinSetCheckMark(hWndMenu, id, hbm, bcheck) \
    ((BOOL)WinSendMsg(hWndMenu,MM_SETITEMCHECKMARK, \
    MPFROM2SHORT(id, (USHORT)bcheck), (MPARAM)hbm))
#define WinSetCheckMarkByPos(hWndMenu, id, hbm, bcheck) \
    ((BOOL)WinSendMsg(hWndMenu,MM_SETITEMCHECKMARKBYPOS, \
    MPFROM2SHORT(id, (USHORT)bcheck), (MPARAM)hbm))

#define MAKE_16BIT_POINTER(p)  ((PVOID)MAKEULONG(LOUSHORT(p),(HIUSHORT(p) << 3) | 7))

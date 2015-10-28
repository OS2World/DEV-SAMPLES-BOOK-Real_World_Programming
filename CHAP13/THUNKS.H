/* --------------------------------------------------------------------
                          Thunks Header File
                              Chapter 13

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* Word align any structures shared with the 16-bit code */
#pragma pack(2)
#if defined(__BORLAND__)

typedef struct
{
    USHORT       usNumAuthors;
    char __far16 *pszBookTitle;
} BOOKINFORMATION;
typedef BOOKINFORMATION __far16 *PBOOKINFORMATION;

extern VOID __far16 pascal GetBookInformation (PBOOKINFORMATION);
extern char __far16 * __far16 pascal GetAuthorByIndex (USHORT);

#elif __IBMC__

typedef struct
{
    USHORT       usNumAuthors;
    char * _Seg16 pszBookTitle;
} BOOKINFORMATION;
typedef BOOKINFORMATION * _Seg16 PBOOKINFORMATION;

extern VOID _Far16 _Pascal GetBookInformation (PBOOKINFORMATION);
extern char * _Seg16 ( _Far16 _Pascal GetAuthorByIndex (USHORT));

#else

#include "book16.h"
extern VOID APIENTRY _THUNK_GetBookInformation (PBOOKINFORMATION);
extern PSZ  APIENTRY _THUNK_GetAuthorByIndex (USHORT);

#endif 
#pragma pack()

/* Dialog and Control Identifiers     */

#define ID_APPNAME                     1
#define IDD_BOOKINFO                   2

#define IDM_BOOKINFO                 100
#define IDM_ABOUT                    101

#define IDC_TITLE                    200
#define IDC_AUTHORLIST               201
 

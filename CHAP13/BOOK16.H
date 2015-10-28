/* --------------------------------------------------------------------
                          Book16 Header File
                              Chapter 13

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */
/* --------------------------------------------------------------------
                         BOOK16 Header File
-------------------------------------------------------------------- */

typedef struct
{
    USHORT usNumAuthors;
    PSZ    pszBookTitle;
} BOOKINFORMATION;
typedef BOOKINFORMATION FAR *PBOOKINFORMATION;

VOID EXPENTRY GetBookInformation (PBOOKINFORMATION);
PSZ  EXPENTRY GetAuthorByIndex (USHORT);

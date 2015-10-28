/* --------------------------------------------------------------------
                   Book16 16-Bit Dynamic Link Libary
                              Chapter 13

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

#include <os2.h>
#include "book16.h"

CHAR szBookTitle[]    = "Real World Programming for OS/2";
CHAR szAuthors[3][13] =
    {
        "Derrel Blain",
        "Kurt Delimon",
        "Jeff English"
    };

VOID EXPENTRY GetBookInformation (PBOOKINFORMATION pBookInfo)
{
    pBookInfo->usNumAuthors = 3;
    pBookInfo->pszBookTitle = szBookTitle;
    return;
}

PSZ EXPENTRY GetAuthorByIndex (USHORT usInx)
{
    return ((PSZ)((usInx < 3) ? &szAuthors[usInx] : NULL));
}

/* --------------------------------------------------------------------
                              Memory DLL
                              Chapter 10

                    Real World Programming for OS/2
             Copyright (c) 1993 Blain, Delimon, and English
-------------------------------------------------------------------- */

/* 
   Returned address is the allocated address plus the size of a
   memory object header.  

   Private memory allocation stores the object size in the header.

   Shared memory allocation stores the object size in the header.
   It also maintains a linked list of the memory objects so that memory 
   can be cleaned up if a process terminates without freeing all of its
   allocated shared memory.
*/

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN

#include <os2.h>
#include <stdio.h>
#include "memdll.h"
#include "meminst.h"

#define PRIVATEALLOC     0x20000      /* 128 K */
#define SHAREDALLOC      0x40000      /* 256 K */

typedef struct tagPRIVATE_MEMORY_OBJECT
{
    ULONG ulSize;        /* Size of memory object  */
} PRIVATE_MEMORY_OBJECT;
typedef PRIVATE_MEMORY_OBJECT *PPRIVATE_MEMORY_OBJECT;

typedef struct tagSHARED_MEMORY_OBJECT
{
    PVOID pPrev;         /* Previous memory object */
    PVOID pNext;         /* Next memory object     */
    ULONG ulSize;        /* Size of memory object  */
} SHARED_MEMORY_OBJECT;
typedef SHARED_MEMORY_OBJECT *PSHARED_MEMORY_OBJECT;

/* Nonshared data in segment _INSTANCEDATA     */

extern INSTANCEDATA_STRUCT InstanceData;

/* Shared data placed into initialized variables data segment */

/* IBMC compiler defines data segments with #pragma data_seg */
#ifdef __IBMC__
#pragma data_seg (DATA32)
#endif

PVOID   pMemShared     = NULL;
BOOL    bFirstInstance = TRUE;
HMODULE hModDll        = 0;


#ifdef __IBMC__
ULONG _System _DLL_InitTerm (HMODULE hModule, ULONG ulTerminating)
#else
ULONG _stdcall InitializeMemDll (HMODULE hModule, ULONG ulTerminating)
#endif
{
    BOOL bResult;

    if (ulTerminating & 0x01)
    {
        PSHARED_MEMORY_OBJECT pMem = InstanceData.pMemSharedHead;

        /* Free all the shared suballocated memory the app forgot to free */
        while (pMem)
        {
            PVOID pMemPrev = pMem->pPrev;
            if (!DosSubFreeMem (pMemShared, pMem, pMem->ulSize))
                pMem = (PSHARED_MEMORY_OBJECT)pMemPrev;
            else
                break;
        }

        /* Release access to semaphore used for shared memory serialization */
        DosSubUnset (pMemShared);

        /* Free the private memory allocated for this instance */
        DosFreeMem (InstanceData.pMemPrivate);

        return (FALSE);
    }
    else
    {
        /* Allocate the private memory and initialize for suballocation */
        bResult = !DosAllocMem ((PPVOID)&InstanceData.pMemPrivate, 
                        PRIVATEALLOC, PAG_READ | PAG_WRITE) &&
                  !DosSubSet (InstanceData.pMemPrivate, 
                        DOSSUB_INIT | DOSSUB_SPARSE_OBJ, PRIVATEALLOC);

        if (bResult)
        {
            if (bFirstInstance)
            {
                /* Allocate the shared memory and initialize for 
                   suballocation.  Only allocate for first instance of DLL */
                bFirstInstance = FALSE;
                bResult = 
                      !DosAllocSharedMem ((PPVOID)&pMemShared, NULL, 
                            SHAREDALLOC, PAG_READ | PAG_WRITE | fSHARE) &&
                      !DosSubSet (pMemShared, 
                            DOSSUB_INIT | DOSSUB_SPARSE_OBJ | DOSSUB_SERIALIZE, 
                            SHAREDALLOC);

                /* Free shared memory if an error occurred */
                if (!bResult && pMemShared)
                    DosFreeMem (pMemShared);
            }
            else
            {
                /* Get access to the shared memory and the semaphore used
                   for memory suballocation of shared memory. */
                bResult =
                      !DosGetSharedMem (pMemShared, PAG_READ | PAG_WRITE) &&
                      !DosSubSet (pMemShared, 
                            DOSSUB_SPARSE_OBJ | DOSSUB_SERIALIZE,
                            SHAREDALLOC);
            }
        }
        if (!bResult)
        {
            /* Free private memory if an error occurred */
            if (InstanceData.pMemPrivate)
                DosFreeMem (InstanceData.pMemPrivate);
        }
    }
 
    /* Save the module handle when we get loaded.  The module handle
       will be the same for all instances of the DLL */

    hModDll = hModule;

    return ((ULONG)bResult);
} 

PVOID EXPENTRY PrivateAllocate (ULONG ulSize)
{
    PVOID pMem;
    
    /* Add room for the memory object header */
    ulSize += sizeof(PRIVATE_MEMORY_OBJECT);

    /* Align on 8 byte boundary */
    ulSize  = (ulSize + 7) & 0xFFFFFFF8;

    if (!DosSubAllocMem (InstanceData.pMemPrivate, &pMem, ulSize))
    {
        /* Save size in memory object header */
        ((PPRIVATE_MEMORY_OBJECT)pMem)->ulSize = ulSize;

        /* Data starts immediately after the memory object header */
        pMem = (PVOID)((ULONG)pMem + sizeof(PRIVATE_MEMORY_OBJECT));

        /* Increment total suballocation of private memory */
        InstanceData.ulTotalSubAlloc  += ulSize;
    }
    else
    {
        WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
            "Private Memory Suballocation Failure",
            "Memory Thermometer",
            0, MB_ERROR | MB_OK);
        pMem = NULL;
    }

    return (pMem);
}

VOID EXPENTRY PrivateFree (PVOID pData)
{
    PPRIVATE_MEMORY_OBJECT pMem;
    ULONG                  ulSize;

    /* Set pointer to memory object header */
    pMem   = (PPRIVATE_MEMORY_OBJECT)((ULONG)pData - sizeof(PRIVATE_MEMORY_OBJECT));
    ulSize = pMem->ulSize;

    if (!DosSubFreeMem (InstanceData.pMemPrivate, pMem, ulSize))

        /* Decrement total suballocation of private memory */
        InstanceData.ulTotalSubAlloc  -= ulSize;

    else
        WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
            "Private Memory Free Failure",
            "Memory Thermometer",
            0, MB_ERROR | MB_OK);

    return;
}

VOID EXPENTRY PrivateQuery (PULONG pTotalAlloc, PULONG pTotalSubAlloc)
{
    *pTotalAlloc    = PRIVATEALLOC;
    *pTotalSubAlloc = InstanceData.ulTotalSubAlloc;
    return;
}

PVOID EXPENTRY SharedAllocate (ULONG ulSize)
{
    PVOID pMem;
    
    /* Add room for the memory object header */
    ulSize += sizeof(SHARED_MEMORY_OBJECT);

    /* Align on 8 byte boundary */
    ulSize  = (ulSize + 7) & 0xFFFFFFF8;

    if (!DosSubAllocMem (pMemShared, &pMem, ulSize))
    {
        /* Save size in memory object header */
        ((PSHARED_MEMORY_OBJECT)pMem)->ulSize = ulSize;
        ((PSHARED_MEMORY_OBJECT)pMem)->pNext  = NULL;
        ((PSHARED_MEMORY_OBJECT)pMem)->pPrev  = InstanceData.pMemSharedHead;
        if (InstanceData.pMemSharedHead)
            ((PSHARED_MEMORY_OBJECT)InstanceData.pMemSharedHead)->pNext = pMem;
        InstanceData.pMemSharedHead    = pMem;

        /* Data starts immediately after the memory object header */
        pMem = (PVOID)((ULONG)pMem + sizeof(SHARED_MEMORY_OBJECT));

        /* Increment total suballocation of shared memory */
        InstanceData.ulTotalSharedSubAlloc  += ulSize;
    }
    else
    {
        WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
            "Shared Memory Suballocation Failure",
            "Memory Thermometer",
            0, MB_ERROR | MB_OK);
        pMem = NULL;
    }

    return (pMem);
}

VOID EXPENTRY SharedFree (PVOID pData)
{
    PSHARED_MEMORY_OBJECT pMem;
    ULONG                 ulSize;

    /* Set pointer to memory object header */
    pMem   = (PSHARED_MEMORY_OBJECT)((ULONG)pData - sizeof(SHARED_MEMORY_OBJECT));
    ulSize = pMem->ulSize;
    if (pMem == InstanceData.pMemSharedHead)
    {
        if (pMem->pPrev)
            ((PSHARED_MEMORY_OBJECT)(pMem->pPrev))->pNext = NULL;
        InstanceData.pMemSharedHead = pMem->pPrev;
    }
    else
    {
        ((PSHARED_MEMORY_OBJECT)(pMem->pNext))->pPrev = pMem->pPrev;
        if (pMem->pPrev)
            ((PSHARED_MEMORY_OBJECT)(pMem->pPrev))->pNext = pMem->pNext;
    }

    if (!DosSubFreeMem (pMemShared, pMem, ulSize))

        /* Decrement total suballocation of private memory */
        InstanceData.ulTotalSharedSubAlloc  -= ulSize;

    else
        WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
            "Shared Memory Free Failure",
            "Memory Thermometer",
            0, MB_ERROR | MB_OK);

    return;
}

VOID EXPENTRY SharedQuery (PULONG pTotalAlloc, PULONG pTotalSharedSubAlloc)
{
    *pTotalAlloc          = SHAREDALLOC;
    *pTotalSharedSubAlloc = InstanceData.ulTotalSharedSubAlloc;
    return;
}





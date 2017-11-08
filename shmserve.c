/*  shmserve.c   v1.1

    This code sample illustrates the usage of suballocated shared memory
    between two or more processes.

    shmserve.exe is the Server Process and should be running at all times.
    shmclien.exe is the Client Process and can be run multiple times.

    A Named-Shared memory segment, called SHMSERVE.SHM is created by the
    Server Process.  It is fairly small and includes various control
    values found in SERVERCONTROLREC.  During startup, the Server Process
    sub-allocates an initial pool of shared memory, and prepares it for
    suballocation.  The address of this pool of memory is stored in the
    (smaller) Named-Shared memory segment for rapid access by clients.

    When a Client Process is run, it attempts to read the Named-Shared
    memory segment, whose presence signifies that a Server Process is
    running.  The Client Process then tries to add some application
    data (GLOM) to the pool of memory.

    Suballocated memory is more efficient in the use/reuse of memory,
    than static blocks/pages of memory, especially with dynamic data
    usage patterns.

*/

/* (c) Copyright IBM Corp. 1998  All rights reserved.

This sample program is owned by International Business Machines
Corporation or one of its subsidiaries ("IBM") and is copyrighted
and licensed, not sold.

You may copy, modify, and distribute this sample program in any
form without payment to IBM,  for any purpose including developing,
using, marketing or distributing programs that include or are
derivative works of the sample program.

The sample program is provided to you on an "AS IS" basis, without
warranty of any kind.  IBM HEREBY  EXPRESSLY DISCLAIMS ALL
WARRANTIES, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.

Some jurisdictions do not allow for the exclusion or limitation of
implied warranties, so the above limitations or exclusions may not
apply to you.  IBM shall not be liable for any damages you suffer
as a result of using, modifying or distributing the sample program
or its derivatives.

Each copy of any portion of this sample program or any derivative
work,  must include a the above copyright notice and disclaimer of
warranty.
*/

/************************************
 * Define Constants
 ************************************/

#define TRACE                      0

/************************************
 * Include Files
 ************************************/

#define INCL_DOSSEMAPHORES
#define INCL_DOSMEMMGR
#define INCL_DOS
#define INCL_BASE
#include <os2.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <shmserve.h>


/************************************
 * Define Statics (data and private functions)
 ************************************/

ULONG server_init      (SERVERCONTROLREC **pSrvCtlRec);
ULONG server_term      (SERVERCONTROLREC **pSrvCtlRec);

/************************************
 * Source Code Instructions
 ************************************/

int main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
  {
    APIRET                rc        = NO_ERROR;   /* Return code */
    CHAR                  tempch;
    SERVERCONTROLREC     *pSrvCtlRec;

    printf("Shared-Memory Server\n");

#if TRACE > 0
    printf("\nSetting up Shared Memory...\n");
#endif

    rc = server_init(&pSrvCtlRec);
    if (rc != 0)
      {
        return(1);
      }

    printf("\nReady for Clients....(press any key to end Server)\n");

    tempch = getchar();

    rc = server_term(&pSrvCtlRec);

    return NO_ERROR;

 }

/* =====================================================================
 *
 * FUNCTION NAME:    server_init
 *
 * DESCRIPTIVE NAME: Initialize server internal memory
 *
 * DESCRIPTION:
 *
 * INPUT:
 *
 * OUTPUT:           Address to Server Control Record (Shared Memory)
 *
 * RETURN CODES:     OK - normal return
 *
** ================================================================== */
ULONG server_init      (SERVERCONTROLREC **pSrvCtlRec)
  {
    APIRET            ret;
    PVOID             pSubBaseOffset;
    SERVERCONTROLREC *SrvCtlRec;

#if TRACE > 0
    printf("server_init starting\n");
#endif

    // Get Shared Memory used for Server Control Record
    ret = DosAllocSharedMem((PVOID *) pSrvCtlRec, SERVER_CONTROL_MEM_NAME,
                            SCR_MEM_SIZE, PAG_WRITE | PAG_READ);
    if (ret != 0)
      {
#if TRACE > 0
        printf("DosAllocSharedMem ret= %d\n", ret);
#endif
        if (ret == ERROR_ALREADY_EXISTS)
          {
            printf("Another process is already running; probably a server\n");
          }
        return(ret);
      }

    SrvCtlRec = *pSrvCtlRec;

    // Commit the 1st chunk to ensure that the Server Control Rec is
    // always at the top of the allocated area.
    ret = DosSetMem((PVOID) SrvCtlRec, SHMEMINIT,
                    PAG_COMMIT | PAG_WRITE | PAG_READ );
    if (ret != 0)
      {
#if TRACE > 0
        printf("DosSetMem ret= %d\n", ret);
#endif
        DosFreeMem((PVOID) SrvCtlRec);
        return(ret);
      }

    // This is a pointer to the area that will be used as a Suballocation
    // Pool for the Server Control Record.
    pSubBaseOffset = (PVOID)(((PUCHAR) SrvCtlRec) + SHMEMINIT);

    // Initialize Heap area as uncommitted memory.
    ret = DosSubSetMem(pSubBaseOffset, DOSSUB_INIT | DOSSUB_SPARSE_OBJ,
                       INIT_SUBALLOC_MEM_SIZE);
    if (ret != 0)
      {
#if TRACE > 0
        printf("DosSubSetMem ret= %d\n", ret);
#endif
        DosFreeMem((PVOID) SrvCtlRec);
        return(ret);
      }

    SrvCtlRec->Server_Status = 2;      /* Don't want trx processed yet */
    SrvCtlRec->SubAllocPool = pSubBaseOffset;
    SrvCtlRec->SubAllocUsage = 0L;
    SrvCtlRec->SubAllocAvail = INIT_SUBALLOC_MEM_SIZE;

    // Create the Semaphores used to access Service Control Record
    ret = DosCreateMutexSem((PSZ)NULL, &(SrvCtlRec->SubAlloc_Lock),
                            DC_SEM_SHARED, FALSE);
    if (ret != 0)
      {
        printf("Unable to create SubAlloc_Lock semaphone %d\n", ret);
        return(ret);
      }

    // Create the Semaphores used to access Service Control Record
    ret = DosCreateMutexSem((PSZ)NULL, &(SrvCtlRec->GLOM_List_Lock),
                            DC_SEM_SHARED, FALSE);
    if (ret != 0)
      {
        printf("Unable to create GLOM_List_Lock semaphone %d\n", ret);
        return(ret);
      }

    SrvCtlRec->Server_Status = 0;      /* running */

#if TRACE > 0
    printf("server_init ending\n");
#endif

    return(NO_ERROR);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    server_term
 *
 * DESCRIPTIVE NAME: Terminate server internal memory
 *
 * DESCRIPTION:      Free memory resources
 *
 * INPUT:
 *
 * OUTPUT:           Address to Server Control Record (Shared Memory)
 *
 * RETURN CODES:     OK - normal return
 *
** ================================================================== */
ULONG server_term      (SERVERCONTROLREC **pSrvCtlRec)
  {
    APIRET            ret;
    PVOID             pSubBaseOffset;
    SERVERCONTROLREC *SrvCtlRec;

#if TRACE > 0
    printf("server_term starting\n");
#endif

    SrvCtlRec = *pSrvCtlRec;

    // Close the Semaphores used to access Service Control Record
    ret = DosCloseMutexSem(SrvCtlRec->SubAlloc_Lock);
    SrvCtlRec->SubAlloc_Lock = 0L;

    // Close the Semaphores used to access Service Control Record
    ret = DosCloseMutexSem(SrvCtlRec->GLOM_List_Lock);
    SrvCtlRec->GLOM_List_Lock = 0L;

    SrvCtlRec->Server_Status = 2;      /* Don't want trx processed yet */

    ret = DosSubUnsetMem(SrvCtlRec->SubAllocPool);
    if (ret != 0)
      {
#if TRACE > 0
        printf("DosSubUnsetMem (Free) ret= %d\n", ret);
#endif
        return(1);
      }

    ret = DosFreeMem((PVOID) SrvCtlRec);
    if (ret != 0)
      {
#if TRACE > 0
        printf("DosFreeMem  ret= %d\n", ret);
#endif
        return(1);
      }

    *pSrvCtlRec = (PVOID) 0L;

#if TRACE > 0
    printf("server_term ending\n");
#endif

    return(NO_ERROR);
  }
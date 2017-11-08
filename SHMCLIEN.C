/*  shmclien.c        v1.1

    This code sample illustrates the usage of suballocated shared memory
    between two or more processes.

    shmserve.exe is the Server Process and should be running at all times.
    shmclien.exe is the Client Process and can be run multiple times.

    This client sample presumes that a Server Process is already running.

    You can use the Client Process to Add, Delete, Read, or List
    application (GLOM) data to/from the Shared memory area.

    You can also inquire the values in the Server Control record.

    Client Commands:
       To add GLOM Data (Name and value):
          SHMCLIEN A Glom_name glom_value
       To delete 1st occurence of GLOM Data (Name):
          SHMCLIEN D Glom_name
       To list GLOM Data (Names and values):
          SHMCLIEN L
       To Inquire the contents of the Server Control Record
          SHMCLIEN I

    You should be able to notice that Suballocation usage will
       increase/decrease, as GLOM items are added/removed.
    Each time you reach a certain threshold, additional memory
       is allocated.

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
SERVERCONTROLREC        *servercontrol;

ULONG client_init      (SERVERCONTROLREC **pSrvCtlRec);
ULONG client_term      (SERVERCONTROLREC **pSrvCtlRec);
ULONG process_data     (int argc, char *argv[], char *envp[],
                        SERVERCONTROLREC *pSrvCtlRec);
ULONG GrowServerCtlSuballoc (SERVERCONTROLREC *serverctl);
ULONG AddGLOMItem      (UCHAR  *glomname, UCHAR  *glomvalue);
ULONG DeleteGLOMItem   (UCHAR  *glomname);
ULONG GetGLOMItem      (UCHAR  *glomname, UCHAR  *glomvalue);
ULONG ListGLOMItems    (void);

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

    printf("Shared-Memory Client\n");

#if TRACE > 1
    printf("\nGetting Shared Memory from Server...\n");
#endif

    rc = client_init(&servercontrol);
    if (rc != 0)
      {
        return(1);
      }

#if TRACE > 1
    printf("\nClient Processing data...\n");
#endif

    rc = process_data(argc, argv, envp, servercontrol);

    rc = client_term(&servercontrol);

    return NO_ERROR;

 }

/* =====================================================================
 *
 * FUNCTION NAME:    client_init
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
ULONG client_init      (SERVERCONTROLREC **pSrvCtlRec)
  {
    APIRET            ret;
    PVOID             pSubBaseOffset;
    SERVERCONTROLREC *SrvCtlRec;

#if TRACE > 1
    printf("client_init starting\n");
#endif

    // Get Shared Memory used for Server Control Record
    ret = DosGetNamedSharedMem((PVOID *) pSrvCtlRec, SERVER_CONTROL_MEM_NAME,
                               PAG_WRITE | PAG_READ);
    if (ret != 0)
      {
#if TRACE > 1
        printf("DosGetNamedSharedMem ret= %d\n", ret);
#endif
        if (ret == ERROR_FILE_NOT_FOUND)
          {
            printf("Server Process is not running\n");
          }
        return(ret);
      }

    SrvCtlRec = *pSrvCtlRec;

    if (SrvCtlRec->Server_Status != 0)
      {
        printf("Server memory is not ready.  An old client may be running!\n");
        return(1);
      }

    // Create the Semaphores used to access Service Control Record
    ret = DosOpenMutexSem((PSZ)NULL, &(SrvCtlRec->SubAlloc_Lock));
    if (ret != 0)
      {
        printf("Unable to open SubAlloc_Lock semaphone %d\n", ret);
        return(ret);
      }

    // Create the Semaphores used to access Service Control Record
    ret = DosOpenMutexSem((PSZ)NULL, &(SrvCtlRec->GLOM_List_Lock));
    if (ret != 0)
      {
        printf("Unable to open GLOM_List_Lock semaphone %d\n", ret);
        return(ret);
      }

#if TRACE > 1
    printf("client_init ending\n");
#endif

    return(NO_ERROR);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    client_term
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
ULONG client_term      (SERVERCONTROLREC **pSrvCtlRec)
  {
    APIRET            ret;
    PVOID             pSubBaseOffset;
    SERVERCONTROLREC *SrvCtlRec;

#if TRACE > 1
    printf("client_term starting\n");
#endif

    SrvCtlRec = *pSrvCtlRec;

    // Close the Semaphores used to access Service Control Record
    ret = DosCloseMutexSem(SrvCtlRec->SubAlloc_Lock);

    // Close the Semaphores used to access Service Control Record
    ret = DosCloseMutexSem(SrvCtlRec->GLOM_List_Lock);

    ret = DosFreeMem((PVOID) SrvCtlRec);
    if (ret != 0)
      {
        printf("DosFreeMem  ret= %d\n", ret);
        return(1);
      }

    *pSrvCtlRec = (PVOID) 0L;

#if TRACE > 1
    printf("client_term ending\n");
#endif

    return(NO_ERROR);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    GrowServerCtlSuballoc
 *
 * DESCRIPTIVE NAME: Increase the Sub-allocatable shared memory
 *                   in the server control record.
 *
 * DESCRIPTION:
 *
 * INPUT:            Address of server control record
 *
 * OUTPUT:           <none>
 *
 * RETURN CODES:     0        - normal return
 *
** ================================================================== */
ULONG GrowServerCtlSuballoc (SERVERCONTROLREC *serverctl)
  {
    APIRET              ret;

    ret = DosSubSetMem(serverctl->SubAllocPool,
                       DOSSUB_GROW | DOSSUB_SPARSE_OBJ,
                       serverctl->SubAllocAvail + GROW_SUBALLOC_MEM_SIZE);
    if (ret != 0)
      {
        printf("DosSubSetMem (Grow) ret= %d\n", ret);
        return(1);
      }

    (serverctl->SubAllocAvail) += GROW_SUBALLOC_MEM_SIZE;

    return(NO_ERROR);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    process_data
 *
 * DESCRIPTIVE NAME: Add, Delete, Read, or List Data, and Inquire Server
 *
 * DESCRIPTION:
 *
 * INPUT:            parameters from commandline
 *
 * OUTPUT:           <none>
 *
 * RETURN CODES:     0        - normal return
 *
** ================================================================== */
ULONG process_data     (int argc, char *argv[], char *envp[],
                        SERVERCONTROLREC *pSrvCtlRec)
  {
    APIRET              ret;
    GLOM                glom_in;

    if (argc < 2)
      {
        printf(
            "Missing Command:  A=Add, D=Delete, R=Read, L=List, I=Inquire\n");
        return(1);
      }

    if (argc == 2)
      {
        if (argv[1][0] == 'L' ||
            argv[1][0] == 'l'   )
          {
            /* List of GLOMs requested: */
            printf("\nList of GLOM data:\n");
            ret = ListGLOMItems();
            if (ret != NO_ERROR)
              {
                printf("NO RECORDS FOUND\n");
              }
            return(ret);
          }
        if (argv[1][0] == 'I' ||
            argv[1][0] == 'i'   )
          {
            /* Inquire contents of servercontrol record: */
            printf("\n\nServer Control Record:\n");
            printf("ServerControl Address   0x%08lX\n",
                   pSrvCtlRec);
            printf("SubAllocPool Address    0x%08lX\n",
                   pSrvCtlRec->SubAllocPool);
            printf("SubAlloc Usage   %lu\n",
                   pSrvCtlRec->SubAllocUsage);
            printf("SubAlloc Avail   %lu\n",
                   pSrvCtlRec->SubAllocAvail);
            printf("Server Status    %u\n",
                   pSrvCtlRec->Server_Status);
            return(NO_ERROR);
          }
      }

    if (argc == 3)
      {
        if (argv[1][0] == 'D' ||
            argv[1][0] == 'd'   )
          {
            /* Delete a GLOM record: */
#if TRACE > 0
            printf("Delete GLOM record:\n");
#endif
            memset(&glom_in, 0, sizeof(GLOM));
            strncpy(glom_in.GLOMName, argv[2], GLOMNameLen);
            glom_in.GLOMName[GLOMNameLen] = (UCHAR) 0;
            ret = DeleteGLOMItem(glom_in.GLOMName);
            return(ret);
          }
        if (argv[1][0] == 'R' ||
            argv[1][0] == 'r'   )
          {
            /* Read a GLOM record: */
#if TRACE > 0
            printf("Read GLOM record:\n");
#endif
            memset(&glom_in, 0, sizeof(GLOM));
            strncpy(glom_in.GLOMName, argv[2], GLOMNameLen);
            glom_in.GLOMName[GLOMNameLen] = (UCHAR) 0;
            ret = GetGLOMItem(glom_in.GLOMName, glom_in.GLOMValue);
            if (ret == NO_ERROR)
              {
                printf("Value of %s = %s\n",
                       glom_in.GLOMName,
                       glom_in.GLOMValue);
              }
            else
              {
                printf("GLOM record: NOT FOUND\n");
              }
            return(ret);
          }
      }

    if (argc == 4)
      {
        if (argv[1][0] == 'A' ||
            argv[1][0] == 'a'   )
          {
            /* Add a GLOM record: */
#if TRACE > 0
            printf("Add GLOM record:\n");
#endif
            memset(&glom_in, 0, sizeof(GLOM));
            strncpy(glom_in.GLOMName, argv[2], GLOMNameLen);
            glom_in.GLOMName[GLOMNameLen] = (UCHAR) 0;
            strncpy(glom_in.GLOMValue, argv[3], GLOMValueLen);
            glom_in.GLOMValue[GLOMValueLen] = (UCHAR) 0;
            ret = AddGLOMItem(glom_in.GLOMName, glom_in.GLOMValue);
            return(ret);
          }
      }

    printf("Invalid Client command\n");
    return(1);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    AddGLOMItem
 *
 * DESCRIPTIVE NAME: Add item to GLOM-list in Server Control record.
 *
 * DESCRIPTION:
 *
 * INPUT:            glomname (up to GLOMNameLen characters)
 *                   glom data (up to GLOMVAlueLen characters)
 *
 * OUTPUT:           <none>  (record added to linked list)
 *
 * RETURN CODES:     NO_ERROR        - normal return
 *                   1    - failure
 *                   2    - glomname item already exists
 *
** ================================================================== */
ULONG AddGLOMItem          (UCHAR  *glomname,
                            UCHAR  *glomvalue)
  {
    APIRET                ret;
    USHORT                i;
    GLOM                 *GLOMitem;
    GLOM                 *newGLOMitem = 0L;

#if TRACE > 9
    printf("AddGLOMItem called\n");
#endif

    /* Hold up the GLOM list, to insert new one: */
    ret = DosRequestMutexSem(servercontrol->GLOM_List_Lock,
                             SEM_INDEFINITE_WAIT);
    if (ret != NO_ERROR)
      {
#if TRACE > 0
        printf("DosRequestMutexSem AddGLOMItem failed ret=%d\n", ret);
#endif
      }

    /* Check to see if GLOM item for this dataserver is already present: */
    GLOMitem = servercontrol->GLOM_List_ll_ptr;
    while (GLOMitem != 0L)
      {
        if ((i=strcmp(GLOMitem->GLOMName, glomname)) == 0)
          {
            DosReleaseMutexSem(servercontrol->GLOM_List_Lock);
            printf("Duplicate GLOM name.  Not Added\n");
            return(2); /* duplicate */
          }
        GLOMitem = GLOMitem->next_ptr;
      }

    if (newGLOMitem == 0L)
      {
        /* Need to Sub Alloc some shared memory: (block while doing) */
        ret = DosRequestMutexSem(servercontrol->SubAlloc_Lock,
                                 SEM_INDEFINITE_WAIT);
get_some_memory:
        ret = DosSubAllocMem(servercontrol->SubAllocPool, (PPVOID) &newGLOMitem,
                             sizeof(GLOM));
        if (ret != NO_ERROR)
          {
#if TRACE > 1
            printf("AddGLOMItem DosSubAllocMem ret= %d\n", ret);
#endif
            /* Try to Grow Suballoc memory: */
            ret = GrowServerCtlSuballoc(servercontrol);
            if (ret == NO_ERROR)
              {
                goto get_some_memory;
              }
            ret = DosReleaseMutexSem(servercontrol->SubAlloc_Lock);
            ret = DosReleaseMutexSem(servercontrol->GLOM_List_Lock);
            printf("Unable to add item, suballocation error\n");
            return(1);
          }

        (servercontrol->SubAllocUsage) += (ULONG) sizeof(GLOM);
#if TRACE > 1
        printf("AddGLOMItem() SubAllocUsage=%8ld\n",
                servercontrol->SubAllocUsage);
#endif

        /* Suballoc done, now unblock: */
        ret = DosReleaseMutexSem(servercontrol->SubAlloc_Lock);
        memset(newGLOMitem, 0, sizeof(GLOM));
        strcpy(newGLOMitem->GLOMName, glomname);
        strcpy(newGLOMitem->GLOMValue, glomvalue);
      }
    else   /* re-use item */
      {
        strcpy(newGLOMitem->GLOMValue, glomvalue);
        ret = DosReleaseMutexSem(servercontrol->GLOM_List_Lock);
        return(NO_ERROR);
      }

    if (servercontrol->GLOM_List_ll_ptr == 0L)
      {
        servercontrol->GLOM_List_ll_ptr = newGLOMitem;
        ret = DosReleaseMutexSem(servercontrol->GLOM_List_Lock);
        return(NO_ERROR);
      }

    GLOMitem = servercontrol->GLOM_List_ll_ptr;
    while (GLOMitem->next_ptr != 0L)
      {
        GLOMitem = GLOMitem->next_ptr;
      }
    GLOMitem->next_ptr = newGLOMitem;
    newGLOMitem->prev_ptr = GLOMitem;

    ret = DosReleaseMutexSem(servercontrol->GLOM_List_Lock);

#if TRACE > 9
    printf("AddGLOMItem done\n");
#endif

    return(NO_ERROR);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    DeleteGLOMItem
 *
 * DESCRIPTIVE NAME: Remove GLOMed item from GLOMed-list in Server Control
 *                   record.
 *
 * DESCRIPTION:
 *
 * INPUT:            glomname
 *
 * OUTPUT:           <none>  (record removed from linked list)
 *
 * RETURN CODES:     NO_ERROR - normal return
 *
** ================================================================== */
ULONG DeleteGLOMItem       (UCHAR  *glomname)
  {
    ULONG                     rc = ERROR_FILE_NOT_FOUND;
    USHORT                    i;
    APIRET                    ret;
    GLOM                     *GLOMitem, *xGLOMitem;
    GLOM                     *delGLOMitem = 0L;
    UCHAR                     deleted_flag = 0;

#if TRACE > 9
    printf("DeleteGLOMItem called\n");
#endif

    /* Hold up the GLOM list, to delete old one: */
    ret = DosRequestMutexSem(servercontrol->GLOM_List_Lock,
                             SEM_INDEFINITE_WAIT);
    if (ret != NO_ERROR)
      {
#if TRACE > 0
        printf("DosRequestMutexSem DelGLOMItem failed ret=%d\n", ret);
#endif
      }

    /* Scan thru linked list, locate GLOM item to delete: */
    GLOMitem = servercontrol->GLOM_List_ll_ptr;
    while (GLOMitem != 0L)
      {
        if ((i=strcmp(GLOMitem->GLOMName, glomname)) == 0)
          {
            rc = NO_ERROR;
            deleted_flag = 1;
            delGLOMitem = GLOMitem;
            GLOMitem = GLOMitem->next_ptr;
            /* delete the item out of the chain: */
            /* bust the linkages: */
            if (delGLOMitem->prev_ptr != 0L)
              {
                /* change the linkage of the previous item in list: */
                xGLOMitem = delGLOMitem->prev_ptr;
                xGLOMitem->next_ptr = delGLOMitem->next_ptr;
                /* change the linkage of the following item in list, if any: */
                xGLOMitem = delGLOMitem->next_ptr;
                if (xGLOMitem != 0L)
                  {
                    xGLOMitem->prev_ptr = delGLOMitem->prev_ptr;
                  }
              }
            else /* This is the first item in the list. */
              {
                /* Hit the starting pointer: */
                servercontrol->GLOM_List_ll_ptr = delGLOMitem->next_ptr;
                /* Hit the next item, make it the first one: */
                xGLOMitem = delGLOMitem->next_ptr;
                if (xGLOMitem != 0L)
                  {        /* Is there at least 1 more item in list? */
                    /* make it the first list entry: */
                    xGLOMitem->prev_ptr = 0L;
                  }
              }
            /* Need to Free Sub Alloc shared memory: (block while doing) */
            ret = DosRequestMutexSem(servercontrol->SubAlloc_Lock,
                                     SEM_INDEFINITE_WAIT);
            ret = DosSubFreeMem(servercontrol->SubAllocPool,
                              (PVOID) delGLOMitem, sizeof(GLOM));
            if (ret != NO_ERROR)
              {
#if TRACE > 0
                printf("DeleteGLOMItem DosSubFreeMem ret= %d\n", ret);
#endif
              }
           (servercontrol->SubAllocUsage) -= (ULONG) sizeof(GLOM);
#if TRACE > 1
            printf("DeleteGLOMItem() SubAllocUsage=%8ld\n",
                   servercontrol->SubAllocUsage);
#endif
            /* Suballoc done, now unblock suballoc: */
            ret = DosReleaseMutexSem(servercontrol->SubAlloc_Lock);
            break;
          }
        else
          {
            GLOMitem = GLOMitem->next_ptr;
          }
      }

    if (deleted_flag == 0)
      {
        printf("Item NOT FOUND\n");
      }

    ret = DosReleaseMutexSem(servercontrol->GLOM_List_Lock);

#if TRACE > 9
    printf("DeleteGLOMItem done\n");
#endif

    return(rc);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    GetGLOMItem
 *
 * DESCRIPTIVE NAME: Get GLOM item info from list in Server Control record
 *
 * DESCRIPTION:
 *
 * INPUT:            glomname
 *
 * OUTPUT:           glomvalue (caller allocates memory)
 *                      (maxsize of DirValue)
 *
 * RETURN CODES:     NO_ERROR        - normal return
 *                   1    - failure
 *                   ERROR_FILE_NOT_FOUND - glomname not found in list
 *
 *
** ================================================================== */
ULONG GetGLOMItem          (UCHAR        *glomname,
                            UCHAR        *glomvalue)
  {
    APIRET                  ret;
    USHORT                  i;
    ULONG                   rc = ERROR_FILE_NOT_FOUND;
    GLOM                   *GLOMitem;

#if TRACE > 9
    printf("GetGLOMItem called\n");
#endif

    if (glomvalue == 0L)
      {
        return(1);
      }

    glomvalue[0] = (UCHAR) 0;

    /* Hold up the GLOM list, to scan list: */
    ret = DosRequestMutexSem(servercontrol->GLOM_List_Lock,
                             SEM_INDEFINITE_WAIT);
    if (ret != NO_ERROR)
      {
#if TRACE > 0
        printf("DosRequestMutexSem GetGLOMItem failed ret=%d\n", ret);
#endif
      }

    /* Scan thru linked list, locate GLOM item to return a copy of: */
    GLOMitem = servercontrol->GLOM_List_ll_ptr;
    while (GLOMitem != 0L)
      {
        if ((i=strcmp(GLOMitem->GLOMName, glomname)) == 0)
          {
            strcpy(glomvalue, GLOMitem->GLOMValue);
            rc = NO_ERROR;
            break;
          }
        GLOMitem = GLOMitem->next_ptr;
      }

    ret = DosReleaseMutexSem(servercontrol->GLOM_List_Lock);

#if TRACE > 9
    printf("GetGLOMItem done\n");
#endif

    return(rc);
  }

/* =====================================================================
 *
 * FUNCTION NAME:    ListGLOMItems
 *
 * DESCRIPTIVE NAME: List GLOM items from list in Server Control record
 *
 * DESCRIPTION:
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * RETURN CODES:     NO_ERROR        - normal return
 *                   1    - failure
 *
 *
** ================================================================== */
ULONG ListGLOMItems        (void)
  {
    APIRET                  ret;
    USHORT                  i;
    ULONG                   rc = ERROR_FILE_NOT_FOUND;
    GLOM                   *GLOMitem;

#if TRACE > 9
    printf("ListGLOMItems called\n");
#endif

    /* Hold up the GLOM list, to scan list: */
    ret = DosRequestMutexSem(servercontrol->GLOM_List_Lock,
                             SEM_INDEFINITE_WAIT);
    if (ret != NO_ERROR)
      {
#if TRACE > 0
        printf("DosRequestMutexSem GetGLOMItem failed ret=%d\n", ret);
#endif
      }

    /* Scan thru linked list, locate GLOM item to return a copy of: */
    GLOMitem = servercontrol->GLOM_List_ll_ptr;
    while (GLOMitem != 0L)
      {
        printf("%s: %s\n", GLOMitem->GLOMName, GLOMitem->GLOMValue);
        rc = NO_ERROR;
        GLOMitem = GLOMitem->next_ptr;
      }

    ret = DosReleaseMutexSem(servercontrol->GLOM_List_Lock);

#if TRACE > 9
    printf("ListGLOMItems done\n");
#endif

    return(rc);
  }
/*  shmserve.h

    This code sample illustrates the usage of suballocated shared memory
    between two or more processes.

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

#define SERVER_CONTROL_MEM_NAME     "\\SHAREMEM\\SHMSERVE.SHM"

/* Total size of Server Control (Shared) Memory (mostly non-committed/sparse) */
#define SCR_MEM_SIZE            2550000L

/* Space at beginning of Server Control (Shared) Memory reserved for */
/*   ServerControlRecord.  (SubAllocated memory starts after this):  */
#define SHMEMINIT                0x1000L

/* Initial size of SubAlloc'd shared memory: */
#define INIT_SUBALLOC_MEM_SIZE    32768L

/* Grow size of SubAlloc'd shared memory (in addition to current memory): */
#define GROW_SUBALLOC_MEM_SIZE    32768L

#define GLOMNameLen            20
#define GLOMValueLen           30


/************************************
 * Define Structures, Unions
 ************************************/

/*----------------------------------------------------------------------------*/
/* struct GLOM                                                                */
/* Data Record of any type, for use by application.                           */
/*----------------------------------------------------------------------------*/

typedef struct um_glom
  {
    UCHAR            GLOMName[GLOMNameLen+1];
    UCHAR            GLOMValue[GLOMValueLen+1];
    struct um_glom  *prev_ptr;
    struct um_glom  *next_ptr;
  } GLOM;

/*----------------------------------------------------------------------------*/
/* struct SERVERCONTROLREC                                                    */
/* Server Control Record:                                                     */
/*   The Server Control Record is shared amoung all Server processes and      */
/*   is the primary access point for all common Server data.  It is named     */
/*   SERVER_CONTROL_MEM_NAME.                                                 */
/*----------------------------------------------------------------------------*/

typedef struct
  {
    PVOID              SubAllocPool;          /* Address of memory pool       */
    HMTX               SubAlloc_Lock;         /* Lock Handle for SubAllocation*/
    ULONG              SubAllocUsage;         /* SubAllocPool current usage   */
    ULONG              SubAllocAvail;         /* SubAllocPool current set size*/
    USHORT             Server_Status;         /* status                       */
                                              /*  0 = Running      */
                                              /*  1 = ShuttingDown */
                                              /*  2 = Starting     */
    GLOM              *GLOM_List_ll_ptr;      /* Linklist of GLOM records     */
    HMTX               GLOM_List_Lock;        /* Lock Handle for GLOM access  */
  } SERVERCONTROLREC;

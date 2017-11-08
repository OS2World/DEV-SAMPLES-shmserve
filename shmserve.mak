#===================================================================
#
#   shmserve.mak - Shared Memory Client/Server example
#   Copyright  IBM Corporation 1998
#
#===================================================================

CFLAGS          =      -Gd- -Se -Re -Ss -Ms -Gm+
LFLAGS          =      /NOE /NOD /ALIGN:16 /EXEPACK
LINK            =      icc /B"$(LFLAGS)" $(CFLAGS) -Fe

MTLIBS          =      CPPOM30.LIB + OS2386.LIB

#-------------------------------------------------------------------

HEADERS = shmserve.h

#-------------------------------------------------------------------
#
#   A list of all of the object files
#
#-------------------------------------------------------------------

OBJS = shmserve.obj shmclien.obj

ALL_IPF =

#-------------------------------------------------------------------
#   This section lists all files to be built by  make.  The
#   makefile builds the executable as well as its associated help
#   file.
#-------------------------------------------------------------------
all: shmserve.exe shmclien.exe

#-------------------------------------------------------------------
#   Dependencies
#     This section lists all object files needed to be built for the
#     application, along with the files it is dependent upon (e.g.
#     its source and any header files).
#-------------------------------------------------------------------

shmserve.obj:   shmserve.c $(HEADERS)

shmclien.obj:   shmclien.c $(HEADERS)

shmserve.exe: shmserve.obj shmserve.def
   -$(CREATE_PATH)
   $(LINK) $@ shmserve.def $(MTLIBS) shmserve.obj

shmclien.exe: shmclien.obj shmclien.def
   -$(CREATE_PATH)
   $(LINK) $@ shmclien.def $(MTLIBS) shmclien.obj

clean :
        @if exist *.obj del *.obj
        @if exist *.dll del *.dll
        @if exist *.exe del *.exe

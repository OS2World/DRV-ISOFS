/* openclos.c -- Create, open and close files.
   Copyright (C) 1999 Eelco Dolstra (edolstra@students.cs.uu.nl).

   Modifications for ISOFS:
   Copyright (C) 2000 Chris Wohlgemuth (chris.wohlgemuth@cityweb.de)
   Copyright (C) 2005 Paul Ratcliffe

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <stdlib.h>
#include <string.h>
#include <sysdep.h>

#include "ifsdmn.h"

IsoDirEntry* isoQueryIsoEntryFromPath(VolData * pVolData, char * pszPath);


void isoToSffsi(Bool fHidden,
   struct sffsi * psffsi, IsoDirEntry* pIsoDirEntry)
{
   isoEntryTimeToOS2(pIsoDirEntry,
      (FDATE *) &psffsi->sfi_cdate, (FTIME *) &psffsi->sfi_ctime);
   isoEntryTimeToOS2(pIsoDirEntry,
      (FDATE *) &psffsi->sfi_adate, (FTIME *) &psffsi->sfi_atime);
   isoEntryTimeToOS2(pIsoDirEntry,
      (FDATE *) &psffsi->sfi_mdate, (FTIME *) &psffsi->sfi_mtime);
   psffsi->sfi_size = pIsoDirEntry->iSize;
   psffsi->sfi_position = 0;
   psffsi->sfi_type = (psffsi->sfi_type & STYPE_FCB) | STYPE_FILE;
   psffsi->sfi_DOSattr &= ~FILE_NON83;
   psffsi->sfi_DOSattr |= pIsoDirEntry->flFlags;
}


/* Finish opening the file by creating an OpenFileData structure and
   filling in the sffsi structure and the opencreate result fields. */
static APIRET finalizeOpen(struct opencreate * popencreate,
    char * pszName,
    Bool fHidden, USHORT usAction, IsoDirEntry* pIsoDirEntry)
{
   OpenFileData * pOpenFileData;
      
   /* Allocate and fill in the OpenFileData structure. */
   pOpenFileData = malloc(sizeof(OpenFileData));
   if (!pOpenFileData)
      return ERROR_NOT_ENOUGH_MEMORY;

   pOpenFileData->iExtent=pIsoDirEntry->iExtent;
   pOpenFileData->iSize=pIsoDirEntry->iSize;
   strcpy(pOpenFileData->szName, pszName);
   pOpenFileData->pIsoEntry=pIsoDirEntry;   

   popencreate->pOpenFileData = pOpenFileData;
   popencreate->usAction = usAction;
   popencreate->pVolData->cOpenFiles++;
   strcpy(popencreate->szName, pszName);
   
   /* Fill in the sffsi structure. */
   isoToSffsi(fHidden, &popencreate->sffsi, pIsoDirEntry);

   if (hasNon83Name(pszName))
      popencreate->sffsi.sfi_DOSattr |= FILE_NON83;
   logsffsi(&popencreate->sffsi);

   logMsg(L_DBG, "     Filesize in sffsi.sfi_size: %d ",popencreate->sffsi.sfi_size );
   logMsg(L_DBG, "     Filename in pOpenFileData->szName: %s ",pOpenFileData->szName );
   return NO_ERROR;
}


APIRET fsOpenCreate(ServerData * pServerData,
   struct opencreate * popencreate) 
{
   CHAR szDir[CCHMAXPATH], szName[CCHMAXPATH], szRealName[CCHMAXPATH];
   Bool fHidden;
   IsoDirEntry* pIsoDirEntry=NULL;

   popencreate->pOpenFileData = 0;
   popencreate->fsGenFlag = 0;
   popencreate->oError = 0;
   
   if (VERIFYFIXED(popencreate->szName) ||
       verifyPathName(popencreate->szName))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG,
      "FS_OPENCREATE, szName=%s, iCurDirEnd=%d, flOpenMode=%08lx, "
      "fsOpenFlag=%04hx, fsAttr=%04hx, fHasEAs=%hd",
      popencreate->szName, popencreate->iCurDirEnd, popencreate->flOpenMode,
      popencreate->fsOpenFlag, popencreate->fsAttr,
      popencreate->fHasEAs);
   logsffsi(&popencreate->sffsi);

   /* We don't want DASD opens. */
   if (popencreate->flOpenMode & OPEN_FLAGS_DASD) {
      logMsg(L_WARN, "Direct access open requested");
      return ERROR_NOT_SUPPORTED;
   }

   /* Split the file name. */
   splitPath(popencreate->szName + 2, szDir, szName);

   /* Find file entry */
   pIsoDirEntry=isoQueryIsoEntryFromPath(popencreate->pVolData, popencreate->szName + 2);

   if(!pIsoDirEntry) {
     logMsg(L_DBG, "Entry for %s not found", popencreate->szName + 2);
   }
   else
     logMsg(L_DBG, "Entry for %s found", popencreate->szName + 2);
   

   if(pIsoDirEntry) {
     /* Found */

     /* Fail if exists? */
     if ((popencreate->fsOpenFlag & 0x000f) ==
         OPEN_ACTION_FAIL_IF_EXISTS) {
       free(pIsoDirEntry);
       return ERROR_OPEN_FAILED;
     }

     /* Sanity check. */
     if (strlen((char *) pIsoDirEntry->chrName) >= CCHMAXPATH) {
       free(pIsoDirEntry);
       return ERROR_FILENAME_EXCED_RANGE;
     }

     strcpy(szRealName, (char *) pIsoDirEntry->chrName);
     fHidden = pIsoDirEntry->flFlags & CDF_HIDDEN;

     /* It must be a regular file. */
     if (pIsoDirEntry->flFlags & FILE_DIRECTORY) {
       free(pIsoDirEntry);
       return ERROR_ACCESS_DENIED;
     }
          
     switch (popencreate->fsOpenFlag & 0x000f) {
       
     case OPEN_ACTION_OPEN_IF_EXISTS:
       
       /* This access mode (3) is used if OS/2 itself tries to open an executable
          or load a module. See ifs.inf for info */
       if ((popencreate->flOpenMode & 3 )==3) {
         logMsg(L_DBG, "%s, line %d, %s: flOpenMode=3 ->OS/2 opens a DLL/EXE", __FILE__, __LINE__, __FUNCTION__);
         return finalizeOpen(popencreate,
                             szRealName, fHidden, FILE_EXISTED, pIsoDirEntry);
       }
       if ((popencreate->flOpenMode & OPEN_ACCESS_WRITEONLY) |
           (popencreate->flOpenMode & OPEN_ACCESS_READWRITE)) {
         free(pIsoDirEntry);       
         return ERROR_ACCESS_DENIED;
       }

       return finalizeOpen(popencreate,
                        szRealName, fHidden, FILE_EXISTED, pIsoDirEntry);
       
     case OPEN_ACTION_REPLACE_IF_EXISTS:
       free(pIsoDirEntry);       
       /* Delete the existing file. */
       return ERROR_ACCESS_DENIED;
     default:
       free(pIsoDirEntry);       
       return ERROR_NOT_SUPPORTED;
     }
              
   }
   else {
   
     /* file does not exist */
     /* Should we create it? */
     if (!(popencreate->fsOpenFlag & OPEN_ACTION_CREATE_IF_NEW))
       return ERROR_OPEN_FAILED;
     
     /* Yes.  Create it with the requested initial size. */
     return ERROR_OPEN_FAILED;
     return ERROR_WRITE_PROTECT;

   }
   return ERROR_ACCESS_DENIED;

}


/* Close a file.  Problem: what should we do in case of an error?
   (And: what does the kernel do in case of an error?)
   Should we flush the entire file? */
APIRET fsClose(ServerData * pServerData, struct close * pclose)
{
   
   logMsg(L_DBG,
      "FS_CLOSE, usType=%hu, fsIOFlag=%04hx",
      pclose->usType, pclose->fsIOFlag);
   logsffsi(&pclose->sffsi);


   /* Free the OpenFileData structure if this is the final close for
      this open file reference in the system. */
   if (pclose->usType == FS_CL_FORSYS) {
      free(pclose->pOpenFileData->pIsoEntry);
      free(pclose->pOpenFileData);
      pclose->pVolData->cOpenFiles--;
      logMsg(L_DBG, "File closed");
   }
   else
      logMsg(L_DBG, "File not closed (not final)");
   
   return NO_ERROR;
}

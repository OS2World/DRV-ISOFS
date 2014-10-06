/* fileinfo.c -- Set and query file information.
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
#include <alloca.h>

#include "ifsdmn.h"
int isoEntryTimeToOS2(IsoDirEntry* pIsoDirEntry, FDATE * pfdate, FTIME * pftime);

APIRET storeFileInfo(
   PGEALIST pgeas, /* DosFindXXX level 3 only */
   char * pszFileName, /* DosFindXXX only */
   Bool fHidden,
   char * * ppData,
   ULONG * pcbData,
   ULONG ulLevel,
   ULONG flFlags,
   int iNext,
   IsoDirEntry * pEntry)
{
   APIRET  finalrc = NO_ERROR;
   PFILESTATUS pBuf;
   int c;
   APIRET rc;

   /* This information allows the search to continue from a specific
      file (for FS_FINDFROMNAME). */
   if (flFlags & FF_GETPOS) {
      if (*pcbData < sizeof(ULONG))
         return ERROR_BUFFER_OVERFLOW;
      * (PULONG) *ppData = iNext;
      *ppData += sizeof(ULONG);
      *pcbData -= sizeof(ULONG);
   }

   /* Store the level 1 file info. */
   if (*pcbData < sizeof(FILESTATUS))
     return ERROR_BUFFER_OVERFLOW;

   pBuf = (PFILESTATUS) *ppData;
   memset(pBuf,0,sizeof(FILESTATUS));

   /* Date and time */
   isoEntryTimeToOS2(pEntry, &pBuf->fdateLastWrite, &pBuf->ftimeLastWrite);
   pBuf->fdateCreation=pBuf->fdateLastWrite;
   pBuf->fdateLastAccess=pBuf->fdateLastWrite;
   pBuf->ftimeCreation=pBuf->ftimeLastAccess=pBuf->ftimeLastWrite;

   pBuf->attrFile = 0L;
   if (S_ISDIR(pEntry->fstat_buf.st_mode)) {
     pBuf->cbFile = 0;
     pBuf->cbFileAlloc = 0;
     pBuf->attrFile |= FILE_DIRECTORY; /* directory */
   } else {
      pBuf->cbFile = pEntry->fstat_buf.st_size;
      pBuf->cbFileAlloc = pEntry->fstat_buf.st_size ;
      logMsg(L_DBG, "In %s, line %d, %s() : Filesize %d stored", __FILE__, __LINE__, __FUNCTION__, pBuf->cbFile);
      logMsg(L_DBG, "In %s, line %d, %s() : Time: %x ", __FILE__, __LINE__, __FUNCTION__, pEntry->fstat_buf.st_ctime);
   }
   pBuf->attrFile |= FILE_READONLY; 
   pEntry->flFlags=(int)pBuf->attrFile;

   *ppData += sizeof(FILESTATUS);
   *pcbData -= sizeof(FILESTATUS);

   /* Store the requested or all EAs for FS_FINDXXX level 3 and
      FS_PATH/FILEINFO levels 3 and 4. */
   if (ulLevel == FIL_QUERYEASFROMLIST) {/* 3 */
     rc = storeEAsInFEAList(pgeas, *pcbData, *ppData);
     if (rc == ERROR_BUFFER_OVERFLOW) {
       /* If the EAs don't fit, try to store level 2 info.  If that
          works, return ERROR_EAS_DIDNT_FIT.  However, if that
          doesn't fit either, return ERROR_BUFFER_OVERFLOW (see
          below). */
       ulLevel = FIL_QUERYEASIZE;
       finalrc = ERROR_EAS_DIDNT_FIT;
     } else {
       if (rc)
         return rc;
       *pcbData -= ((PFEALIST) *ppData)->cbList;
       *ppData += ((PFEALIST) *ppData)->cbList;
     }
   }

   /* Store the size of the EA set on disk for level 2. */
   if (ulLevel == FIL_QUERYEASIZE) {

      if (*pcbData < sizeof(ULONG))
         return ERROR_BUFFER_OVERFLOW;
      * (PULONG) *ppData = 0;
      
      *ppData += sizeof(ULONG);
      *pcbData -= sizeof(ULONG);
   }

   /* Store the file name, if one was given (for FS_FINDXXX). */
   if (pszFileName) {
      c = strlen(pszFileName);
      if (c > 255)
         return ERROR_FILENAME_EXCED_RANGE;
      if (*pcbData < c + 2)
         return ERROR_BUFFER_OVERFLOW;
      * (PUCHAR) *ppData = c;
      memcpy(*ppData + 1, pszFileName, c + 1);
      *ppData += c + 2;
      *pcbData -= c + 2;
   }
   return finalrc;
}


APIRET doFileInfo(struct sffsi * psffsi,
   ULONG flFlag, ULONG ulLevel,
   VolData * pVolData, Bool fHidden,
   ULONG cbData, char * pData, IsoDirEntry* pIsoDirEntry)
{
   PGEALIST pgeas;

   /* Access the file and get file info. */
   if (flFlag & FI_SET) {
      /* Set file info. */
     /*     return ERROR_ACCESS_DENIED;*/
     return ERROR_WRITE_PROTECT;
   } else {
     /* Query file info. */
     switch (ulLevel) {
       
     case FIL_STANDARD:     
     case FIL_QUERYEASIZE: /* Query level 1 or 2 file info. */
       
       memset(pData, 0, cbData);
       return storeFileInfo(0, 0,
                            fHidden,
                            &pData,
                             &cbData,
                             ulLevel,
                             0, 0,
                             pIsoDirEntry);
       
     case FIL_QUERYEASFROMLIST: /* Query level 3 (EA) file info. */
      

       /* The GEAs are stored in the exchange buffer which is
          about to be overwritten; so make a copy. */
       pgeas = alloca(((PGEALIST) pData)->cbList);
       memcpy(pgeas, pData, ((PGEALIST) pData)->cbList);
       logMsg(L_DBG,
          "In doFileInfo(): pgeas: %x ",pgeas);

       return storeEAsInFEAList(pgeas, 65536, pData);
       
     case 4: /* Store the entire EA set. */

       return storeEAsInFEAList(0, 65536, pData);
       
     default:
       logMsg(L_EVIL,
              "Unknown query-FS_[FILE|PATH]INFO info level: %d",
              ulLevel);
       return ERROR_NOT_SUPPORTED;
     }
   }  
}



APIRET fsFileInfo(ServerData * pServerData,
   struct fileinfo * pfileinfo)
{
   logMsg(L_DBG,
      "FS_FILEINFO, usLevel=%hd, cbData=%hd, "
      "fsFlag=%04hx, fsIOFlag=%04hx",
      pfileinfo->usLevel, pfileinfo->cbData,
      pfileinfo->fsFlag, pfileinfo->fsIOFlag);
   logsffsi(&pfileinfo->sffsi);

   return doFileInfo(
      &pfileinfo->sffsi,
      pfileinfo->fsFlag,
      pfileinfo->usLevel,
      pfileinfo->pVolData,
      pfileinfo->sffsi.sfi_DOSattr & FILE_HIDDEN,
      pfileinfo->cbData,
      (char *) pServerData->pData,
      pfileinfo->pOpenFileData->pIsoEntry);
}


APIRET fsPathInfo(ServerData * pServerData,
   struct pathinfo * ppathinfo)
{
   APIRET rc;
   CHAR szDir[CCHMAXPATH], szName[CCHMAXPATH];
   Bool fHidden=FALSE;
   IsoDirEntry* pIsoDirEntry=NULL;
   IsoDirEntry* pIsoTemp;

   if (VERIFYFIXED(ppathinfo->szName) ||
       verifyPathName(ppathinfo->szName))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG,
      "FS_PATHINFO, szName=%s, usLevel=%hd, "
      "cbData=%hd, fsFlag=%04hx",
      ppathinfo->szName, ppathinfo->usLevel,
      ppathinfo->cbData, ppathinfo->fsFlag);
   
   /* Split the file name. */
   splitPath(ppathinfo->szName + 2, szDir, szName);

   /* Find the directory/file. */
   pIsoDirEntry=isoQueryIsoEntryFromPath( ppathinfo->pVolData,
                            ppathinfo->szName+2);


   if(!pIsoDirEntry) {
     logMsg(L_DBG, "Entry for %s not found",  ppathinfo->szName+2);
     return ERROR_PATH_NOT_FOUND;
   }

   pIsoTemp=pIsoDirEntry;

   rc= doFileInfo(
      0,
      ppathinfo->fsFlag,
      ppathinfo->usLevel,
      ppathinfo->pVolData,
      fHidden,
      ppathinfo->cbData,
      (char *) pServerData->pData,
      pIsoDirEntry);

   free(pIsoDirEntry);

   return rc;
}


APIRET fsFileAttribute(ServerData * pServerData,
   struct fileattribute * pfileattribute)
{
   CHAR szDir[CCHMAXPATH], szName[CCHMAXPATH];
   IsoDirEntry* pIsoDirEntry=NULL;
   
   if (VERIFYFIXED(pfileattribute->szName) ||
       verifyPathName(pfileattribute->szName))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG,
      "FS_FILEATTRIBUTE, szName=%s, fsFlag=%hd, fsAttr=%hd, iCurDirEnd: %d",
      pfileattribute->szName, pfileattribute->fsFlag,
      pfileattribute->fsAttr,pfileattribute->iCurDirEnd);
   
   /* Split the file name. */
   splitPath(pfileattribute->szName + 2, szDir, szName);
   strcpy(szName,pfileattribute->szName);

   /* Find the directory/file. */
   pIsoDirEntry=isoQueryIsoEntryFromPath(
                                  pfileattribute->pVolData,
                                  szName+2);
   if(!pIsoDirEntry) {
     return ERROR_PATH_NOT_FOUND;
   }

   if (pfileattribute->fsFlag & FA_SET) {
      
      /* Set the file attributes. */
      free(pIsoDirEntry);
      return ERROR_WRITE_PROTECT;
                  
   } else {

     /* Query the file attributes. */
     pfileattribute->fsAttr = FILE_READONLY;
     if(S_ISDIR(pIsoDirEntry->fstat_buf.st_mode))
       pfileattribute->fsAttr |= FILE_DIRECTORY;
     free(pIsoDirEntry);
     return NO_ERROR;
   }
}

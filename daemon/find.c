/* find.c -- Read directory contents with file info.
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <io.h>


#include "ifsdmn.h"


static void freeSearchData(SearchData * pSearchData)
{
    while (pSearchData->pFirstInIsoDir)
    {
	IsoDirEntry *pIsoDirEntry = pSearchData->pFirstInIsoDir->pNext;

	free(pSearchData->pFirstInIsoDir);
	pSearchData->pFirstInIsoDir = pIsoDirEntry;
    }

    free(pSearchData);
}


/* Determine whether the name matches with the search specification
   according to OS/2's meta-character rules.  !!! Time complexity is
   probably exponential in the worst case. */
static BOOL nameMatches(char * pszExp, char * pszName)
{
   switch (*pszExp) {

      case 0:
         while (*pszName == '.') pszName++;
         return !*pszName;

      case '*':
         return
            (*pszName && nameMatches(pszExp, pszName + 1)) ||
            nameMatches(pszExp + 1, pszName);

      case '?':
         return
            (!*pszName) ||
            ((*pszName == '.') &&
               nameMatches(pszExp + 1, pszName)) ||
            ((*pszName != '.') &&
               nameMatches(pszExp + 1, pszName + 1));

      case '.':
         return
            ((!*pszName) &&
               nameMatches(pszExp + 1, pszName)) ||
            ((*pszName == '.') &&
               nameMatches(pszExp + 1, pszName + 1));

      default:
         return (tolower(*pszExp) == tolower(*pszName)) &&
            nameMatches(pszExp + 1, pszName + 1);
      
   }
}


/* Check whether the directory entry matches the search criteria. */
static BOOL entryMatches(SearchData * pSearchData,
   IsoDirEntry * pEntry)
{
   if ((pEntry->flFlags & CDF_HIDDEN) &&
       !(pSearchData->flAttr & FILE_HIDDEN))
      return FALSE;
   
   if (!(pEntry->flFlags & CDF_HIDDEN) &&
       (pSearchData->flAttr & (FILE_HIDDEN << 8)))
      return FALSE;

   if (!(pSearchData->flAttr & FILE_NON83) &&
       hasNon83Name((char *) pEntry->chrName))
      return FALSE;
   
   return nameMatches(pSearchData->szName, (char *) pEntry->chrName);
}

/* Check whether the file matches the search criteria.  Note: the
   kernel never passes in "may-have" bits for the read-only and
   archive attributes. */
static BOOL fileMatches2(SearchData * pSearchData,
   IsoDirEntry * pEntry)
{
   BOOL fDir = pEntry->flFlags & FILE_DIRECTORY;
   BOOL fSystem = pEntry->flFlags & FILE_SYSTEM;

   /* Note: These checks may not be perfect for ISOFS, read there may be bugs in
      this routine. */

   if (fDir && !(pSearchData->flAttr & FILE_DIRECTORY))
      return FALSE;
 
   if (!fDir && (pSearchData->flAttr & (FILE_DIRECTORY << 8)))
      return FALSE;
 
   if (fSystem && !(pSearchData->flAttr & FILE_SYSTEM))
      return FALSE;
   
   if (!fSystem && (pSearchData->flAttr & (FILE_SYSTEM << 8)))
      return FALSE;
   
   if ((pEntry->flFlags & CFF_IWUSR) &&
       (pSearchData->flAttr & (FILE_READONLY << 8)))
     return FALSE;
   
   if (!(pEntry->flFlags & CFF_OS2A) &&
       (pSearchData->flAttr & (FILE_ARCHIVED << 8)))
     return FALSE;
   
   return TRUE;
}


static void advanceSearch(SearchData * pSearchData)
{
   pSearchData->pIsoNext = pSearchData->pIsoNext->pNext;
   pSearchData->iNext++;
}


static APIRET storeNextFileInfo(
   SearchData * pSearchData,
   PGEALIST pgeas,
   char * * ppData,
   ULONG * pcbData,
   ULONG ulLevel,
   ULONG flFlags)
{
   APIRET rc;
   IsoDirEntry * pEntry;
   
   while ((pEntry = pSearchData->pIsoNext)) {

     logMsg(L_DBG, "Testing entry %ld, pEntry->flFlags=%x, flFlags=%x,  name=%s",
            pSearchData->iNext,
            pEntry->flFlags, flFlags, (char *) pEntry->chrName);

     /* OS/2 file info relevant to file matching is stored in the
        directory (file name and hidden flag) and in the file's info
        sector (read-only, system, and archive flags).  So we try to
        reject the file by first looking at pEntry (fast) and then at
        the corresponding file's info sector (slow).  The last step
        also gives us all the necessary file info. */

     /* Does the current directory entry match the search criteria? */
     if (entryMatches(pSearchData, pEntry)) {

       /* We must read the file's info sector to see whether the
          other flags (directory, read-only, archived) match with
          the criteria, and to return info about the file.  If
          the storage file no longer exists, however, we silently
          skip this entry.  Such a condition can occur when the file
          was deleted after the directory contents had been read.
          (It can also indicate an inconsistency (directory
          referring to non-existing file) but this should be handled
          by chkdsk). */

       if (fileMatches2(pSearchData, pEntry)) {
         
         /* Store the file information in the buffer. */
         if (!(pSearchData->flAttr & FILE_NON83))
           strupr((char *) pEntry->chrName); /* !!! codepage */

         rc = storeFileInfo(pgeas,
                            (char *) pEntry->chrName,
                            pEntry->flFlags & CDF_HIDDEN,
                            ppData, pcbData,
                            ulLevel, flFlags, pSearchData->iNext,pEntry);
         if (rc) return rc;
         
         advanceSearch(pSearchData);
         logMsg(L_DBG, "OK, leaving storeNextFileInfo()");
         return NO_ERROR;
       }/* end of: if (fileMatches2(pSearchData, pEntry)) */
       
     }
     advanceSearch(pSearchData);

   }

   return ERROR_NO_MORE_FILES;
}

static APIRET storeDirContents(
   SearchData * pSearchData,
   PGEALIST pgeas,
   char * pData,
   ULONG cbData,
   ULONG ulLevel,
   PUSHORT pcMatch,
   ULONG flFlags)
{
   int cAdded = 0;
   int cMatch = *pcMatch;
   APIRET rc;

   if (!cMatch)
      return ERROR_INVALID_PARAMETER;

   *pcMatch = 0;
   
   do {
      
      rc = storeNextFileInfo(
         pSearchData, pgeas,
         &pData, &cbData,
         ulLevel, flFlags);

      if (rc) {
         if (cAdded &&
             ((rc == ERROR_BUFFER_OVERFLOW) ||
              (rc == ERROR_EAS_DIDNT_FIT) ||  
              (rc == ERROR_NO_MORE_FILES)))
            break;
         if (rc == ERROR_EAS_DIDNT_FIT) {
            /* In the case that the EAs for the first entry don't fit,
               advance anyway so that the next query will return the
               next entry. */ 
            advanceSearch(pSearchData);
            *pcMatch = 1;
         }
         return rc; 
      }

      cAdded++;
      
   } while (cAdded < cMatch);

   *pcMatch = cAdded;

   return NO_ERROR;
}


APIRET fsFindFirst(ServerData * pServerData, struct
   findfirst * pfindfirst)
{
   APIRET rc;
   CHAR szDir[CCHMAXPATH];
   SearchData * pSearchData;
   PGEALIST pgeas = 0;
   struct iso_directory_record * idr;
   IsoDirEntry* pIsoDirEntry=NULL;
   int iSize;
   int iExtent;

   pfindfirst->pSearchData = 0;

   if (VERIFYFIXED(pfindfirst->szName) ||
       verifyPathName(pfindfirst->szName))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG, "FS_FINDFIRST, curdir=%s, name=%s, "
      "iCurDirEnd=%d, fsAttr=%04hx, cMatch=%d, "
      "usLevel=%d, fsFlags=%04hx, cbData=%d",
      pfindfirst->cdfsi.cdi_curdir,
      pfindfirst->szName,
      pfindfirst->iCurDirEnd,
      pfindfirst->fsAttr,
      pfindfirst->cMatch,
      pfindfirst->usLevel,
      pfindfirst->fsFlags,
      pfindfirst->cbData);

   /* FIL_STANDARD    : 1
      FIL_QUERYEASIZE : 2
      FIL_QUERYEASFROMLIST : 3 */
   if (pfindfirst->usLevel != FIL_STANDARD &&
       pfindfirst->usLevel != FIL_QUERYEASIZE &&
       pfindfirst->usLevel != FIL_QUERYEASFROMLIST
       ) {
     logMsg(L_EVIL, "Unknown FS_FINDFIRST info level: %d",
            pfindfirst->usLevel);
     return ERROR_NOT_SUPPORTED;
   }


   /* Allocate the SearchData structure. */
   pSearchData = malloc(sizeof(SearchData));
   if (!pSearchData)
      return ERROR_NOT_ENOUGH_MEMORY;
   pSearchData->pFirstInIsoDir = 0;
   pSearchData->flAttr = pfindfirst->fsAttr;

   /* Split the search specification. */
   splitPath(pfindfirst->szName + 2, szDir, pSearchData->szName);

   logMsg(L_DBG, "dir=%s, spec=%s", szDir, pSearchData->szName);

   if (!*pSearchData->szName) {
      freeSearchData(pSearchData);
      return ERROR_INVALID_PARAMETER;
   }

   if(strlen(szDir)) {
     /* Walk the directory tree to find the name in szDir. */
     pIsoDirEntry=isoQueryIsoEntryFromPath( pfindfirst->pVolData,
                              szDir);

     if(!pIsoDirEntry) {
       logMsg(L_DBG, "Entry for %s not found", szDir);
       freeSearchData(pSearchData);
       return ERROR_PATH_NOT_FOUND;
     }
     logMsg(L_DBG, "Entry for %s found", szDir);

     iExtent=pIsoDirEntry->iExtent;
     iSize=pIsoDirEntry->iSize;

     if(!S_ISDIR(pIsoDirEntry->fstat_buf.st_mode)) {
       free(pIsoDirEntry);
       logMsg(L_DBG, "Entry is not an directory", szDir);
       freeSearchData(pSearchData);
       return ERROR_PATH_NOT_FOUND;
     }

     free(pIsoDirEntry);
   }
   else {
     iExtent=pfindfirst->pVolData->iRootExtent;
     /* Get the contents of the directory from ISO file. */
     idr=(struct iso_directory_record *)pfindfirst->pVolData->root_directory_record;
     iSize=isonum_733((unsigned char *)idr->size);
   }
   pIsoDirEntry=getDirEntries(szDir, iExtent,
               iSize, pfindfirst->pVolData, 0);

   pSearchData->pFirstInIsoDir=pIsoDirEntry;
   pSearchData->pIsoNext=pIsoDirEntry;

   if(pIsoDirEntry) {
#if 0
     logMsg(L_DBG, "Found following entries:");
     do {
       logMsg(L_DBG, "Name is: %s, extent: %d, parent extent: %d",
              pIsoDirEntry->chrName,pIsoDirEntry->iExtent,pIsoDirEntry->iParentExtent);
       pIsoDirEntry=pIsoDirEntry->pNext;
     }while(pIsoDirEntry);     
#endif
   }
   else {
     freeSearchData(pSearchData);
     return ERROR_FILE_NOT_FOUND;
   }


   /* The GEAs are stored in the exchange buffer which is
      about to be overwritten; so make a copy. */
   if (pfindfirst->usLevel == FIL_QUERYEASFROMLIST) {
     pgeas = alloca(((PGEALIST) pServerData->pData)->cbList);
     memcpy(pgeas, pServerData->pData,
            ((PGEALIST) pServerData->pData)->cbList);
   }
   
   /* Store up to the requested number of items. */
   rc = storeDirContents(
      pSearchData,
      pgeas,
      (char *) pServerData->pData,
      pfindfirst->cbData,
      pfindfirst->usLevel,
      &pfindfirst->cMatch,
      pfindfirst->fsFlags);
   if (rc && (rc != ERROR_EAS_DIDNT_FIT)) {
     freeSearchData(pSearchData);
     return rc;
   }

   logMsg(L_DBG, "%d entries returned", pfindfirst->cMatch);

#if 0 /*!! ???????????*/
   if(!pfindfirst->cMatch) {
     freeSearchData(pSearchData);
     /*return ERROR_NO_MORE_FILES;*/
     return ERROR_FILE_NOT_FOUND;
   }
#endif

   pfindfirst->pSearchData = pSearchData;

   pfindfirst->pVolData->cSearches++;

   return rc;
}


/* FS_FINDNEXT is used by the 16-bit DosFindNext API. */
APIRET fsFindNext(ServerData * pServerData,
   struct findnext * pfindnext)
{
   APIRET rc;
   SearchData * pSearchData = pfindnext->pSearchData;
   PGEALIST pgeas = 0;
   
   logMsg(L_DBG, "FS_FINDNEXT, cMatch=%d, "
      "usLevel=%d (1: FIL_STANDARD, 2: FIL_QUERYEASIZE, 3: FIL_QUERYEASFROMLIST), fsFlags=%d, cbData=%d",
      pfindnext->cMatch,
      pfindnext->usLevel,
      pfindnext->fsFlags,
      pfindnext->cbData);



   /* The GEAs are stored in the exchange buffer which is
      about to be overwritten; so make a copy. */   
   if (pfindnext->usLevel == FIL_QUERYEASFROMLIST) {
     pgeas = alloca(((PGEALIST) pServerData->pData)->cbList);
     memcpy(pgeas, pServerData->pData,
            ((PGEALIST) pServerData->pData)->cbList);
   }

   /* Store up to the requested number of items. */
   rc = storeDirContents(
      pSearchData,
      pgeas,
      (char *) pServerData->pData,
      pfindnext->cbData,
      pfindnext->usLevel,
      &pfindnext->cMatch,
      pfindnext->fsFlags);
   if (rc) return rc;

   logMsg(L_DBG, "%d entries returned", pfindnext->cMatch);

   return NO_ERROR;
}


/* FS_FINDFROMNAME is used by the 32-bit DosFindNext API. */
APIRET fsFindFromName(ServerData * pServerData,
   struct findfromname * pfindfromname)
{
   APIRET rc;
   SearchData * pSearchData = pfindfromname->pSearchData;
   PGEALIST pgeas = 0;

   if (VERIFYFIXED(pfindfromname->szName))
      return ERROR_INVALID_PARAMETER;

   logMsg(L_DBG, "FS_FINDFROMNAME, cMatch=%d, "
      "usLevel=%d, fsFlags=%d, cbData=%d, "
      "ulPosition=%d, szName=%s",
      pfindfromname->cMatch,
      pfindfromname->usLevel,
      pfindfromname->fsFlags,
      pfindfromname->cbData,
      pfindfromname->ulPosition,
      pfindfromname->szName);

#if 0
   if (pSearchData->iNext != pfindfromname->ulPosition + 1) {
      /* Does the kernel actually give us ulPositions not equal to
         the previous pSearchData->iNext?  Apparently only when the
         previous item(s) did not match, but then they still won't
         match so we can skip them.  So this code is commented out. */
      logMsg(L_EVIL, "Interesting ulPosition (%ld vs. %ld)",
         pfindfromname->ulPosition, pSearchData->iNext);
#if 0
      pSearchData->pNext = &pSearchData->dot;
#endif
      pSearchData->iNext = 0;
      while (pSearchData->pIsoNext && (pSearchData->iNext !=
         pfindfromname->ulPosition))
         advanceSearch(pSearchData);
      if (!pSearchData->pIsoNext)
         return ERROR_INVALID_PARAMETER;
      advanceSearch(pSearchData);
   }
#endif

   /* The GEAs are stored in the exchange buffer which is
      about to be overwritten; so make a copy. */
   if (pfindfromname->usLevel == FIL_QUERYEASFROMLIST) {
      pgeas = alloca(((PGEALIST) pServerData->pData)->cbList);
      memcpy(pgeas, pServerData->pData,
         ((PGEALIST) pServerData->pData)->cbList);
   }

   /* Store up to the requested number of items. */
   rc = storeDirContents(
      pSearchData,
      pgeas,
      (char *) pServerData->pData,
      pfindfromname->cbData,
      pfindfromname->usLevel,
      &pfindfromname->cMatch,
      pfindfromname->fsFlags);
   if (rc) return rc;

   logMsg(L_DBG, "%d entries returned", pfindfromname->cMatch);

   return NO_ERROR;
}


APIRET fsFindClose(ServerData * pServerData,
   struct findclose * pfindclose)
{
   logMsg(L_DBG, "FS_FINDCLOSE");

   /* It is possible to receive FS_FINDCLOSE _after_ the volume that
      the search applies to has been detached!  So it is important
      that FS_ATTACH[detach] is not over-zealous in cleaning up search
      data. */

   freeSearchData(pfindclose->pSearchData);
  
   pfindclose->pVolData->cSearches--;
   return NO_ERROR;
}

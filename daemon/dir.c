/* dir.c -- Directory operations: chdir, mkdir, and rmdir.
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

#include "ifsdmn.h"


static APIRET changeDir(ServerData * pServerData,
   struct chdir * pchdir)
{
   int iExtent;   

   if (VERIFYFIXED(pchdir->szDir) ||
       verifyPathName(pchdir->szDir))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG, "CD_EXPLICIT, curdir=%s, newdir=%s", pchdir->cdfsi.cdi_curdir, pchdir->szDir);
    

   /* Walk the directory tree to find the name in szDir. */
   iExtent=isoQueryDirExtentFromPath(
      pchdir->pVolData,
      pchdir->pVolData->iRootExtent,
      pchdir->szDir + 2,
      NULL);

   if(!iExtent) {
     return ERROR_PATH_NOT_FOUND;
   }

   return NO_ERROR;
}


static APIRET verifyDir(ServerData * pServerData,
   struct chdir * pchdir)
{
   return NO_ERROR;
}


static APIRET freeDir(ServerData * pServerData,
   struct chdir * pchdir)
{
   return NO_ERROR;
}


APIRET fsChDir(ServerData * pServerData, struct chdir * pchdir)
{
   if (VERIFYFIXED(pchdir->cdfsi.cdi_curdir))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG, "FS_CHDIR, flag=%d, cdfsi.dir=%s, cdfsi.flags=%d",
      pchdir->fsFlag, pchdir->cdfsi.cdi_curdir,
      pchdir->cdfsi.cdi_flags);

   switch (pchdir->fsFlag) {

      case CD_EXPLICIT:
         return changeDir(pServerData, pchdir);
      
      case CD_VERIFY:
         return verifyDir(pServerData, pchdir);
      
      case CD_FREE:
         return freeDir(pServerData, pchdir);
      
      default:
         logMsg(L_EVIL, "Unknown FS_CHDIR flag: %d", pchdir->fsFlag);
         return ERROR_NOT_SUPPORTED;
         
   }
}


APIRET fsMkDir(ServerData * pServerData, struct mkdir * pmkdir)
{

   pmkdir->oError = 0;
   
   if (VERIFYFIXED(pmkdir->szName) ||
       verifyPathName(pmkdir->szName))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG, "FS_CHDIR, szName=%s, fsFlags=%d",
      pmkdir->szName, pmkdir->fsFlags);

   return ERROR_WRITE_PROTECT;
}


APIRET fsRmDir(ServerData * pServerData, struct rmdir * prmdir)
{
   if (VERIFYFIXED(prmdir->szName) ||
       verifyPathName(prmdir->szName))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG, "FS_RMDIR, szName=%s", prmdir->szName);

   return ERROR_WRITE_PROTECT;
}

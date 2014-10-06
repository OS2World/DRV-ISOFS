/* fileop.c -- File operations (move and delete).
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


APIRET deleteFile(VolData * pVolData, char * pszFullName)
{

   return ERROR_WRITE_PROTECT;

}


APIRET fsDelete(ServerData * pServerData, struct delete * pdelete)
{
   if (VERIFYFIXED(pdelete->szName) ||
       verifyPathName(pdelete->szName))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG, "FS_DELETE, szName=%s", pdelete->szName);

   return deleteFile(pdelete->pVolData, pdelete->szName);
}

APIRET fsMove(ServerData * pServerData, struct move * pmove)
{
   
   if (VERIFYFIXED(pmove->szSrc) ||
       verifyPathName(pmove->szSrc) ||
       VERIFYFIXED(pmove->szDst) ||
       verifyPathName(pmove->szDst))
      return ERROR_INVALID_PARAMETER;
   
   logMsg(L_DBG, "FS_MOVE, szSrc=%s, szDst=%s",
      pmove->szSrc, pmove->szDst);

   return ERROR_WRITE_PROTECT;

}


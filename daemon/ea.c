/* ea.c -- EA list <-> FEALIST conversion.
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

#include <string.h>
#include "ifsdmn.h"


static APIRET storeEA(char * pszName, ULONG * pcbData, PFEA * ppfea)
{
   int cbName = 0;
   int cbSize;
   PFEA pfea = *ppfea;

   logMsg(L_DBG,
          "In %s, line %d, storeEA(): pszName: '%s', *pcbData: %d, *ppfea: %x\n",
          __FILE__, __LINE__, pszName, *pcbData, *ppfea );
   
   cbName = strlen(pszName);
   cbSize = sizeof(FEA) + cbName + 1;

   /* Enough space? */
   if (*pcbData < cbSize) {
      logMsg(L_DBG,
            "In %s, line %d, storeEA(): ERROR_BUFFER_OVERFLOW\n",
            __FILE__, __LINE__);
      return ERROR_BUFFER_OVERFLOW;
   }
   
   /* Construct an empty EA. */
   pfea->fEA = 0;
   pfea->cbName = cbName;
   pfea->cbValue = 0;

   memcpy((char*)(&pfea[1]), pszName, cbName + 1);

   *ppfea = (PFEA) ((char*)pfea + cbSize);
   *pcbData -= cbSize;

   return NO_ERROR;
}


static APIRET storeEAsInFEAList2( PGEALIST pgeas, ULONG cbData, char * pData)
{
   APIRET rc;
   PFEALIST pfeas = (PFEALIST) pData;
   PFEA pfea = pfeas->list;
   PGEA pgea;
   int cbLeft;

   logMsg(L_DBG,
      "In %s, line %d, storeEAsInFEAList2(): pgeas: %xh, cbData: %d, pData: %xh ",__FILE__, __LINE__, pgeas, cbData, pData );

   if (cbData < 4) 
     return ERROR_BUFFER_OVERFLOW;

   /* In case of a buffer overflow, cbList is expected to be the size
      of the EAs on disk. */ 

   pfeas->cbList = 0;
   cbData -= 4;

   if (pgeas) {      
      /* Store EAs matching the GEA list. */
     pgea = pgeas->list;
     cbLeft = pgeas->cbList;
     if (cbLeft < 4)
       return ERROR_EA_LIST_INCONSISTENT;
     cbLeft -= 4;
     while (cbLeft) {
       if ((cbLeft < pgea->cbName + 2) ||
           (pgea->szName[pgea->cbName]))
         return ERROR_EA_LIST_INCONSISTENT;
       rc = storeEA(pgea->szName, &cbData, &pfea);
       if (rc) return rc;
       cbLeft -= pgea->cbName + 2;
       pgea = (PGEA) (pgea->cbName + 2 + (char *) pgea);
     }
   }

   pfeas->cbList = (char *) pfea - (char *) pData;

   return NO_ERROR;
}

     
/* Stores the EAs specified by pgeas in the buffer.  If pgeas = 0 all
   EAs are to be stored. */
APIRET storeEAsInFEAList(PGEALIST pgeas, ULONG cbData, char * pData)
{
   APIRET rc;

   rc = storeEAsInFEAList2( pgeas, cbData, pData);

   if(rc)
     return rc;

   return NO_ERROR;
}


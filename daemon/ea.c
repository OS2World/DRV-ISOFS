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


int compareEANames(char * pszName1, char * pszName2) 
{
   return stricmp(pszName1, pszName2);
}


static APIRET storeEA(char * pszName, int cbValue, octet * pabValue,
   ULONG * pcbData, PFEA * ppfea)
{
   int cbName, cbSize;
   PFEA pfea = *ppfea;
   
   logMsg(L_DBG,
          "In %s, line %d, storeEA(): pszName: %s",
          __FILE__, __LINE__, pszName);

   cbName = strlen(pszName);
   cbSize = sizeof(FEA) + cbName + 1 + cbValue;

   /* Enough space? */
   if (*pcbData < cbSize) return ERROR_BUFFER_OVERFLOW;
   
   /* Copy the EA to the FEA. We only have empty EAs. */
   pfea->fEA = 0;
   pfea->cbName = cbName;

   pfea->cbValue = cbValue;

   memcpy(sizeof(FEA) + (char *) pfea, pszName, cbName + 1);

   if (cbValue)
      memcpy(sizeof(FEA) + cbName + 1 + (char *) pfea,
         pabValue, cbValue);

   *ppfea = (PFEA) (cbSize + (char *) pfea);

   *pcbData -= cbSize;

   return NO_ERROR;
}


static APIRET storeMatchingEAs( char * pszName, ULONG * pcbData, PFEA * ppfea)
{
   APIRET rc;
   ULONG cbData = *pcbData;
   PFEA pfea = *ppfea;

   logMsg(L_DBG,
          "In %s, line %d, storeMatchingEAs(): pszName: %s, pcbData: %xh, ppfea: %xh",
          __FILE__, __LINE__, pszName, pcbData, ppfea );

   if (pszName) {
     rc = storeEA(pszName, 0, 0, &cbData, &pfea);
     if (rc)
       return rc;
   }

   *pcbData = cbData;
   *ppfea = pfea;

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
       rc = storeMatchingEAs( pgea->szName, &cbData, &pfea);
       if (rc) return rc;
       cbLeft -= pgea->cbName + 2;
       pgea = (PGEA) (pgea->cbName + 2 + (char *) pgea);
     }
     
   } else {
     /* Store all EAs. */
     rc = storeMatchingEAs(0, &cbData, &pfea);
     if (rc)
       return rc;
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


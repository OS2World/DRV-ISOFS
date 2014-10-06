/* utils.c -- Helper routines for the daemon.
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
#include <time.h>

#include "ifsdmn.h"
#include "sysdep.h"


/* Verify that the specified string is no longer than cbMaxLen bytes;
   i.e. the null character should be encountered within cbMaxLen bytes
   from pszStr.  Return 1 iff the string is too long. */
int verifyString(char * pszStr, int cbMaxLen)
{
   while (cbMaxLen--) if (!*pszStr++) return 0;
   logMsg(L_DBG, "String too long");
   return 1;
}


/* Verify that the specified string starts with "x:\" ("x" being a
   random character).  Return 1 iff it isn't so. */
int verifyPathName(char * pszName)
{
   return
      (!*pszName) ||
      (pszName[1] != ':') ||
      ((pszName[2] != '/') && (pszName[2] != '\\'));
}


/* Invalid characters for DOS file names (from CP Guide & Reference,
   "File Names in the FAT File System"). */
#define DOSBADCHARS "<>|+=:;,.\"/\\[]"


/* Return TRUE iff the specified name is not a valid DOS (that is,
   8.3) filename. */
Bool hasNon83Name(char * pszName)
{
   char * p;
   int ext = 0, i = 0;
   if ((strcmp(pszName, ".") == 0) ||
       (strcmp(pszName, "..") == 0))
      return FALSE;
   for (p = pszName; *p; p++) {
      i++;
      if (ext) {
         if (*p == '.') return TRUE;
         if (i > 3) return TRUE;
         if (strchr(DOSBADCHARS, *p)) return TRUE;
      } else {
         if (*p == '.') {
            ext = 1;
            i = 0;
         } else {
            if (i > 8) return TRUE;
            if (strchr(DOSBADCHARS, *p)) return TRUE;
         }
      }
   }
   return FALSE;
}


/* Split a path into its last component and the stuff that precedes
   it (minus the separating (back)slash).
   Examples:
   "/foo/bar" -> ("/foo", "bar")
   "/foo/bar/" -> ("/foo/bar", "")
   "/foo" -> ("", "foo")
   (All paths must begin with a (back)slash).
   The buffers pointed to by pszPrefix and pszLast must be at least
   CCHMAXPATH bytes large, and pszFull must be no more than CCHMAXPATH
   bytes large. */
void splitPath(char * pszFull, char * pszPrefix, char * pszLast)
{
   char * p, * p2;

   p = pszFull;
   p2 = 0;

   while (*p) {
      if (IS_PATH_SEPARATOR(*p)) p2 = p;
      p++;
   }

   strcpy(pszLast, p2 + 1);

   while (p2 > pszFull) {
      if (!IS_PATH_SEPARATOR(*p2)) break;
      p2--;
   }

   if (p2 == pszFull)
      *pszPrefix = 0;
   else {
      memcpy(pszPrefix, pszFull, p2 - pszFull + 1);
      pszPrefix[p2 - pszFull + 1] = 0;
   }
}


void logsffsi(struct sffsi * psffsi)
{
   logMsg(L_DBG,
      "sfi_mode=%08lx, sfi_size=%08lx, sfi_position=%08lx, "
      "sfi_PID=%04hx, sfi_PDB=%04hx, sfi_selfsfn=%04hx, "
      "sfi_tstamp=%02x, sfi_type=%04hx, sfi_DOSattr=%04hx",
      psffsi->sfi_mode,
      psffsi->sfi_size,
      psffsi->sfi_position,
      psffsi->sfi_PID,
      psffsi->sfi_PDB,
      psffsi->sfi_selfsfn,
      (int) psffsi->sfi_tstamp,
      psffsi->sfi_type,
      psffsi->sfi_DOSattr);
}


int isoEntryTimeToOS2(IsoDirEntry* pIsoDirEntry, FDATE * pfdate, FTIME * pftime)
{
   /* Date and time */
  memset(pfdate,0,sizeof(FDATE));
  pfdate->year=pIsoDirEntry->date_buf[0]-80;
  pfdate->month=pIsoDirEntry->date_buf[1];
  pfdate->day=pIsoDirEntry->date_buf[2];
  memset(pftime,0,sizeof(FTIME));
  pftime->hours=pIsoDirEntry->date_buf[3];
  pftime->minutes=pIsoDirEntry->date_buf[4];
  pftime->twosecs=pIsoDirEntry->date_buf[5]/2;

  /*logMsg(L_DBG, "isoEntryTimeToOS2()");*/
  
  return 0;  
}


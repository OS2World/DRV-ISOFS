/* os2.c -- OS/2 (EMX)-specific low-level code.
   Copyright (C) 1999 Eelco Dolstra (edolstra@students.cs.uu.nl).

   Modifications for ISOFS:
   Copyright (C) 2005-2006 Paul Ratcliffe

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
#include <fcntl.h>
#include <io.h>
#define INCL_DOSERRORS
#include <os2.h>

#include "sysdep.h"

File * sysOpenFile(char * pszName, int flFlags,
   FilePos cbInitialSize)
{
   APIRET rc;
   HFILE h;
   ULONG ulAction;
   File * pFile;

   if ((rc = DosOpenL((PSZ) pszName, &h, &ulAction, cbInitialSize,
      FILE_NORMAL | FILE_ARCHIVED,
      flFlags >> 16,
      (flFlags & 0xffff) | OPEN_FLAGS_FAIL_ON_ERROR |
      OPEN_FLAGS_NOINHERIT, 0)))
   {
      return 0;
   }

   pFile = malloc(sizeof(File));
   if (!pFile) {
      DosClose(h);
      return 0;
   }
   pFile->h = h;
   setmode(h, O_BINARY);

   return pFile;
}


Bool sysCloseFile(File * pFile)
{
   APIRET rc;
   int h = pFile->h;
   free(pFile);
   rc = DosClose(h);
   return rc == NO_ERROR;
}


Bool sysSetFilePos(File * pFile, FilePos ibNewPos)
{
   FilePos ibActual;
   return !DosSetFilePtrL(pFile->h, ibNewPos, FILE_BEGIN, &ibActual);
}


Bool sysReadFromFile(File * pFile, FilePos cbLength,
   octet * pabBuffer, FilePos * pcbRead)
{
   ULONG cbActual;
   if (DosRead(pFile->h, pabBuffer, cbLength, &cbActual))
      return FALSE;
   *pcbRead = cbActual;
   return TRUE;
}


Bool sysWriteToFile(File * pFile, FilePos cbLength,
   octet * pabBuffer, FilePos * pcbWritten)
{
   ULONG cbActual;
   if (DosWrite(pFile->h, pabBuffer, cbLength, &cbActual))
      return FALSE;
   *pcbWritten = cbActual;
   return TRUE;
}


Bool sysSetFileSize(File * pFile, FilePos cbSize)
{
   return !DosSetFileSizeL(pFile->h, cbSize);
}


Bool sysQueryFileSize(File * pFile, FilePos * pcbSize)
{
   FILESTATUS3L info;
   if (DosQueryFileInfo(pFile->h, FIL_STANDARDL, &info, sizeof(info)))
      return FALSE;
   *pcbSize = info.cbFile;
   return TRUE;
}


Bool sysDeleteFile(char * pszName, Bool fFastDelete)
{
   return fFastDelete
      ? !DosForceDelete((PSZ) pszName)
      : !DosDelete((PSZ) pszName);
}


Bool sysFileExists(char * pszName)
{
   FILESTATUS3L info;
   if (DosQueryPathInfo((PSZ) pszName,
      FIL_STANDARDL, &info, sizeof(info)))
      return FALSE;
   return !(info.attrFile & FILE_DIRECTORY);
}


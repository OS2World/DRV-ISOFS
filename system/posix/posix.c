/* posix.c -- Posix compatible low-level code.
   Copyright (C) 1999 Eelco Dolstra (edolstra@students.cs.uu.nl).

   Modifications for ISOFS:
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
#include <time.h>
#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sysdep.h"

File * sysOpenFile(char * pszName, int flFlags,
   FilePos cbInitialSize)
{
   int h;
   int f = O_BINARY;
   int pmode = S_IREAD | S_IWRITE;
   File * pFile;

   switch (flFlags & SOF_RWMASK) {
      case SOF_READONLY:  f |= O_RDONLY; break;
      case SOF_WRITEONLY: f |= O_WRONLY; break;
      case SOF_READWRITE: f |= O_RDWR;   break;
      default: return 0;
   }
   
   if (flFlags & SOF_WRITE_THROUGH) f |= O_SYNC;

   if (sysFileExists(pszName)) {

      switch (flFlags & SOF_EXMASK) {
         case SOF_FAIL_IF_EXISTS: f |= O_EXCL; return 0;
         case SOF_OPEN_IF_EXISTS: break;
         case SOF_REPLACE_IF_EXISTS:
            if (!sysDeleteFile(pszName, TRUE)) return 0;
            f |= O_CREAT;
            break;
         default: return 0;
      }

   } else {
      if ((flFlags & SOF_NXMASK) != SOF_CREATE_IF_NEW) return 0;
      f |= O_CREAT;
   }
   
   h = open(pszName, f, pmode);
   if (h == -1) return 0;

   pFile = malloc(sizeof(File));
   if (!pFile) {
      close(h);
      return 0;
   }
   pFile->h = h;

   if (f & O_CREAT)
      if (!sysSetFileSize(pFile, cbInitialSize)) {
         close(h);
         free(pFile);
         return 0;
      }

   return pFile;
}


Bool sysCloseFile(File * pFile)
{
   int h = pFile->h;
   free(pFile);
   return !close(h);
}


Bool sysSetFilePos(File * pFile, FilePos ibNewPos)
{
   return lseek(pFile->h, ibNewPos, SEEK_SET) == ibNewPos;
}


Bool sysReadFromFile(File * pFile, FilePos cbLength,
   octet * pabBuffer, FilePos * pcbRead)
{
   int r;
   r = read(pFile->h, pabBuffer, cbLength);
   if (r == -1) return FALSE;
   *pcbRead = r;
   return TRUE;
}


Bool sysWriteToFile(File * pFile, FilePos cbLength,
   octet * pabBuffer, FilePos * pcbWritten)
{
   int r;
   r = write(pFile->h, pabBuffer, cbLength);
   if (r == -1) return FALSE;
   *pcbWritten = r;
   return TRUE;
}


Bool sysSetFileSize(File * pFile, FilePos cbSize)
{
/*    struct stat s; */
/*    if (fstat(pFile->h, &s) == -1) return FALSE; */
/*    if (!sysSetFilePos(pFile, cbSize)) return FALSE; */
/*    if (cbSize > s.st_size) { */
/*    } */

   /* !!! not POSIX! */
   return !chsize(pFile->h, cbSize);
}


Bool sysQueryFileSize(File * pFile, FilePos * pcbSize)
{
   struct stat s;
   if (fstat(pFile->h, &s) == -1) return FALSE;
   *pcbSize = s.st_size;
   return TRUE;
}


Bool sysDeleteFile(char * pszName, Bool fFastDelete)
{
   return !remove(pszName);
}


Bool sysFileExists(char * pszName)
{
   struct stat s;
   if (stat(pszName, &s) == -1) return FALSE;
   return S_ISREG(s.st_mode);
}


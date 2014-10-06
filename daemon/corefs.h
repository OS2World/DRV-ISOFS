/* corefs.h -- Header file to the system-independent FS code.
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

#ifndef _COREFS_H
#define _COREFS_H

#include "types.h"

/*
 * Low-level volume stuff
 */

/* Flags for files. These are equal to the Unix flags.
   Most of them are meaningless to the OS/2 FSD. */

#define CFF_EXTEAS 04000000 /* file has external EAs */

#define CFF_OS2A   02000000 /* file has been modified */
#define CFF_OS2S   01000000 /* system file */

#define CFF_IFMT   00370000
#define CFF_IFEA   00200000
#define CFF_IFSOCK 00140000
#define CFF_IFLNK  00120000
#define CFF_IFREG  00100000
#define CFF_IFBLK  00060000
#define CFF_IFDIR  00040000
#define CFF_IFCHR  00020000
#define CFF_IFIFO  00010000

#define CFF_ISUID  00004000
#define CFF_ISGID  00002000
#define CFF_ISVTX  00001000

#define CFF_ISLNK(m)      (((m) & CFF_IFMT) == CFF_IFLNK)
#define CFF_ISREG(m)      (((m) & CFF_IFMT) == CFF_IFREG)
#define CFF_ISDIR(m)      (((m) & CFF_IFMT) == CFF_IFDIR)
#define CFF_ISCHR(m)      (((m) & CFF_IFMT) == CFF_IFCHR)
#define CFF_ISBLK(m)      (((m) & CFF_IFMT) == CFF_IFBLK)
#define CFF_ISFIFO(m)     (((m) & CFF_IFMT) == CFF_IFIFO)
#define CFF_ISSOCK(m)     (((m) & CFF_IFMT) == CFF_IFSOCK)
#define CFF_ISEA(m)       (((m) & CFF_IFMT) == CFF_IFEA)

#define CFF_IRWXU 00700
#define CFF_IRUSR 00400
#define CFF_IWUSR 00200
#define CFF_IXUSR 00100

#define CFF_IRWXG 00070
#define CFF_IRGRP 00040
#define CFF_IWGRP 00020
#define CFF_IXGRP 00010

#define CFF_IRWXO 00007
#define CFF_IROTH 00004
#define CFF_IWOTH 00002
#define CFF_IXOTH 00001

#define CDF_HIDDEN      2

#endif /* !_COFEFS_H */





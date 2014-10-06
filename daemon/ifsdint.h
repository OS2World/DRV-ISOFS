/* ifsdint.h -- External interface to the IFS.
   Copyright (C) 1999 Eelco Dolstra (edolstra@students.cs.uu.nl).

   Modifications for ISOFS:
   Copyright (C) 2000 Chris Wohlgemuth (chris.wohlgemuth@cityweb.de)
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

#ifndef _IFSDINT_H
#define _IFSDINT_H

#include "stubfsd.h"

#define IFS_NAME "ISOFS"
#define IFS_VERSION "ISOFS 1.0.3"

#define ERROR_ISOFS_BASE          (ERROR_STUBFSD_BASE + 100)
#define ERROR_ISOFS_SETPARAMS (ERROR_ISOFS_BASE + 1) /* error settings params */
/* Mount errors for ISOFS */
#define ERROR_ISOFS_INVALIDOFFSET (ERROR_ISOFS_BASE + 2)
#define ERROR_ISOFS_FILEOPEN (ERROR_ISOFS_BASE + 3)
#define ERROR_ISOFS_NOTISO (ERROR_ISOFS_BASE + 4)

/* FSCTL_IFS_SETPARAMS sets daemon parameters. */
#define FSCTL_IFS_SETPARAMS      0x8020

/* Flags for DETACHPARMS.flFlags. */
#define DP_FORCE               1  /* unmount even if unable to flush */

/* Structure for DosFSAttach, subcode FS_ATTACH. */
typedef struct {
      CHAR szBasePath[CCHMAXPATH];
      CHAR szCharSet[256];
      int iOffset;
} IFS_ATTACH;

/* Structure for DosFSAttach, subcode FS_DETACH. */
typedef struct {
      ULONG flFlags;
} IFS_DETACH;

/* Structure for FSCTL_IFS_SETPARAMS. */
typedef struct {
      /* Array of zero-terminated strings, zero-terminated.  For
         example, "Foo" \000 "Bar" \000 \000. */
      CHAR szParams[1024];
} IFS_SETPARAMS;

#endif /* !_IFSDINT_H */

/* ifsdmn.h -- Header file for the daemon code.
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

#ifndef _IFSDMN_H
#define _IFSDMN_H

#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "iso9660.h"
#include "stubfsd.h"
#include "ifsdint.h"
#include "corefs.h"
#include "sysdep.h"

#define sector_offset 0

typedef struct _ServerData ServerData;

struct _ServerData{
      Bool fRunning;
      Bool fQuit;
      /* Exchange buffers. */
      PFSREQUEST pRequest;
      PFSDATA pData;
      /* Head of linked list of volumes. */
      VolData * pFirstVolume;
};

typedef struct _IsoDirEntry IsoDirEntry;
struct _IsoDirEntry {
  IsoDirEntry * pNext;
  int cbName;
  int flFlags;
  int iExtent; /* Extent of this entry */
  int iSize;   /* Len of extent */
  int iParentExtent; /* Extent of the parent */
  char chrFullPath[CCHMAXPATH];
  char chrName[CCHMAXPATH];
  struct stat fstat_buf;
  char date_buf[7];
};

struct _SearchData {
      CHAR szName[CCHMAXPATH];
      ULONG flAttr;
      IsoDirEntry * pFirstInIsoDir;
      IsoDirEntry isoDot , isoDotDot;
      IsoDirEntry * pIsoNext;
      int iNext;
};

struct _OpenFileData {
  IsoDirEntry* pIsoEntry;
  /* Path name through which the file was opened. */
      CHAR szName[CCHMAXPATH];
  int iExtent;
  int iSize;
};

struct _VolData {
      ServerData * pServerData;
      char chDrive;
      unsigned long idRoot;
  File  *isoFile; /* The mounted ISO file */
  char   fileName[CCHMAXPATH];
  struct iso_primary_descriptor ipd; /* and its descriptor */
  char  chrCDName[34];
  struct iso_directory_record *root_directory_record;
  int iRootExtent; /* The root extent of the ISO */
  int iRootSize;
  int high_sierra;
  int got_joliet;
  IsoDirEntry* curDir; /* The linked list of the contents of the current dir */
  int ucs_level;
  int iSectorOffset;
  char szCharSet[CCHMAXPATH];
  struct nls_table *nls;
      /* Statistics. */
      int cOpenFiles;
      int cSearches;
      /* Next and previous elements in linked list of volumes. */
      VolData * pNext;
      VolData * pPrev;
};

/* Message severity codes. */
#define L_FATAL  1
#define L_EVIL   2
#define L_ERR    3
#define L_WARN   4
#define L_INFO   7
#define L_DBG    9

/* Additional value for DOS attribute fields. */
#define FILE_NON83 0x40

/* Global functions. */

IsoDirEntry* isoQueryIsoEntryFromPath(VolData * pVolData, char * pszPath);

int isoQueryDirExtentFromPath(VolData * pVolData,
                              int iRootExtent, char * pszPath, int* pSize);

IsoDirEntry* getDirEntries(char	*rootname, int	extent,	int	len, VolData  * pVolData, int iOnlyDirs);

int isoEntryTimeToOS2(IsoDirEntry* pIsoDirEntry, FDATE * pfdate, FTIME * pftime);

int isonum_723 (char	*p);

int isonum_733 (unsigned char	*p);

void logMsg(int level, char * pszMsg, ...);

int processArgs(ServerData * pServerData, int argc, char * * argv,
   int startup);

int verifyString(char * pszStr, int cbMaxLen);

#define VERIFYFIXED(str) verifyString(str, sizeof(str))

int verifyPathName(char * pszName);

Bool hasNon83Name(char * pszName);

void splitPath(char * pszFull, char * pszPrefix, char * pszLast);

void logsffsi(struct sffsi * psffsi);

APIRET storeFileInfo(
   PGEALIST pgeas, /* DosFindXXX level 3 only */
   char * pszFileName, /* DosFindXXX only */
   Bool fHidden,
   char * * ppData,
   ULONG * pcbData,
   ULONG ulLevel,
   ULONG flFlags,
   int iNext,
   IsoDirEntry * pEntry);

APIRET deleteFile(VolData * pVolData, char * pszFullName);

APIRET storeEAsInFEAList(PGEALIST pgeas, ULONG cbData, char * pData);
  
/* FSD functions. */
APIRET fsFsCtl(ServerData * pServerData, struct fsctl * pfsctl);
APIRET fsAttach(ServerData * pServerData, struct attach * pattach);
APIRET fsIOCtl(ServerData * pServerData, struct ioctl * pioctl);
APIRET fsFsInfo(ServerData * pServerData, struct fsinfo * pfsinfo);
APIRET fsFlushBuf(ServerData * pServerData,
   struct flushbuf * pflushbuf);
APIRET fsShutdown(ServerData * pServerData,
   struct shutdown * pshutdown);
APIRET fsOpenCreate(ServerData * pServerData,
   struct opencreate * popencreate);
APIRET fsClose(ServerData * pServerData, struct close * pclose);
APIRET fsRead(ServerData * pServerData, struct read * pread);
APIRET fsWrite(ServerData * pServerData, struct write * pwrite);
APIRET fsChgFilePtr(ServerData * pServerData,
   struct chgfileptr * pchgfileptr);
APIRET fsNewSize(ServerData * pServerData, struct newsize * pnewsize);
APIRET fsFileAttribute(ServerData * pServerData,
   struct fileattribute * pfileattribute);
APIRET fsFileInfo(ServerData * pServerData,
   struct fileinfo * pfileinfo);
APIRET fsCommit(ServerData * pServerData, struct commit * pcommit);
APIRET fsPathInfo(ServerData * pServerData,
   struct pathinfo * ppathinfo);
APIRET fsDelete(ServerData * pServerData, struct delete * pdelete);
APIRET fsMove(ServerData * pServerData, struct move * pmove);
APIRET fsChDir(ServerData * pServerData, struct chdir * pchdir);
APIRET fsMkDir(ServerData * pServerData, struct mkdir * pmkdir);
APIRET fsRmDir(ServerData * pServerData, struct rmdir * prmdir);
APIRET fsFindFirst(ServerData * pServerData,
   struct findfirst * pfindfirst);
APIRET fsFindNext(ServerData * pServerData,
   struct findnext * pfindnext);
APIRET fsFindFromName(ServerData * pServerData,
   struct findfromname * pfindfromname);
APIRET fsFindClose(ServerData * pServerData,
   struct findclose * pfindclose);
APIRET fsProcessName(ServerData * pServerData,
   struct processname * pprocessname);

#endif /* !_IFSDMN_H */

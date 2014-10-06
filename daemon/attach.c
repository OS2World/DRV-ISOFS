/* attach.h -- Handles (at|de)tachments and other volume stuff.
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

#define INCL_DOSNLS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ifsdmn.h"

#include "sysdep.h"
#include "nls.h"


APIRET fsFsCtl(ServerData * pServerData, struct fsctl * pfsctl)
{
   int argc, ok, i;
   IFS_SETPARAMS * params;
   char * p;
   char * * argv;
   
   APIRET error;
   
   logMsg(L_DBG,
      "FS_FSCTL, iArgType=%hd, usFunc=0x%hX, cbParm=%d, cbMaxData=%d",
      pfsctl->iArgType, pfsctl->usFunc, pfsctl->cbParm,
      pfsctl->cbMaxData);

   switch (pfsctl->usFunc) {

      case FSCTL_ERROR_INFO: /* return error info */
         if (pfsctl->cbParm < sizeof(USHORT))
            return ERROR_INVALID_PARAMETER;
         error = * (USHORT *) pServerData->pData;

         sprintf((char *) pServerData->pData, "%s error %ld",
            IFS_NAME, error);
         pfsctl->cbData = strlen((char *) pServerData->pData) + 1;
         if (pfsctl->cbData > pfsctl->cbMaxData)
            return ERROR_BUFFER_OVERFLOW;

         return NO_ERROR;

      case FSCTL_MAX_EASIZE: /* return max EA sizes */
         pfsctl->cbData = sizeof(EASIZEBUF);
         if (pfsctl->cbMaxData < sizeof(EASIZEBUF))
            return ERROR_BUFFER_OVERFLOW;

         /* We are readonly and we don't have any EAs on the CD
            but by telling the system we have them we get all the pages
            in the settings notebook of files and folders like
            class, sort, background, symbol and so on. If we return 0
            there're only the first two file pages. */
         ((PEASIZEBUF) pServerData->pData)->cbMaxEASize = 65535;
         ((PEASIZEBUF) pServerData->pData)->cbMaxEAListSize = 65535;

         return NO_ERROR;

      case FSCTL_IFS_SETPARAMS: /* set daemon parameters */
         if (pfsctl->cbParm != sizeof(IFS_SETPARAMS))
            return ERROR_INVALID_PARAMETER;

         params = (IFS_SETPARAMS *) pServerData->pData;

         argc = 1;
         ok = 0;
         for (p = params->szParams;
              p < params->szParams + sizeof(params->szParams) - 1;
              p++)
            if (!*p) {
               argc++;
               if (!p[1]) {
                  ok = 1;
                  break;
               }
            }
         if (!ok) return ERROR_INVALID_PARAMETER;

         argv = alloca(argc * sizeof(char *));
         argv[0] = "fsctl";

         for (i = 1, p = params->szParams; i < argc; i++) {
            argv[i] = p;
            while (*p++) ;
         }
         
         return processArgs(pServerData, argc, argv, 0) ?
            ERROR_ISOFS_SETPARAMS : NO_ERROR;

      default:
         return ERROR_NOT_SUPPORTED;
   }
}

static BOOL checkCpSupport(int iCp)
{
  switch(iCp)
    {
    case 437:
    case 737:
    case 775:
    case 850:
    case 852:
    case 855:
    case 857:
    case 860:
    case 861:
    case 862:
    case 863:
    case 864:
    case 865:
    case 866:
    case 869:
    case 874:
      return TRUE;
    default:
      return FALSE;
    }
  return FALSE;
}


static APIRET attachVolume(ServerData * pServerData,
   struct attach * pattach)
{
   VolData * pVolData;
   IFS_ATTACH * parms = (IFS_ATTACH *) pServerData->pData;
   int rc;
   struct iso_primary_descriptor jpd;
   struct hs_primary_descriptor *hpd;
   struct iso_directory_record *idr = 0;
   /* For codepage */
   ULONG ulCp[4];
   ULONG ulInfoLen=0;
   char *ptr;

   pattach->pVolData = 0;
   if ((pattach->cbParm != sizeof(IFS_ATTACH)) ||
       VERIFYFIXED(parms->szBasePath)
       )
     return ERROR_INVALID_PARAMETER;

   for (ptr = parms->szBasePath; *ptr; ptr++)
        if (*ptr == '/')
           *ptr = '\\';
   
   logMsg(L_DBG, "attaching drive, isopath=%s, offset: %d, charset: %s",
          parms->szBasePath, parms->iOffset, parms->szCharSet);
   
   if ((strncmp(parms->szBasePath, "\\\\", 2) != 0) && /* UNC */
       (strncmp(parms->szBasePath, "////", 2) != 0) && /* UNC */
       ((strlen(parms->szBasePath) < 3) ||
        (!isalpha((unsigned char) parms->szBasePath[0])) ||
        (parms->szBasePath[1] != ':') ||
        ((parms->szBasePath[2] != '\\') &&
         (parms->szBasePath[2] != '/'))))
     return ERROR_INVALID_PARAMETER;
   
   /* Max sector not tested, yet */
   if(parms->iOffset<0)
     return ERROR_ISOFS_INVALIDOFFSET;
   
   /* Allocate a VolData structure. */
   pVolData = malloc(sizeof(VolData));
   if (!pVolData) {
      logMsg(L_EVIL, "out of memory");
      return ERROR_NOT_ENOUGH_MEMORY;
   }
   memset(pVolData,0,sizeof(VolData));
   pVolData->pServerData = pServerData;
   pVolData->chDrive = toupper(pattach->szDev[0]);
   pVolData->cOpenFiles = 0;
   pVolData->cSearches = 0;
   strncpy(pVolData->fileName,parms->szBasePath,sizeof(pVolData->fileName));
   pVolData->iSectorOffset=parms->iOffset;
   strncpy(pVolData->szCharSet,parms->szCharSet,sizeof(pVolData->szCharSet));

   /* Load translation table */
   if(strlen(pVolData->szCharSet)) {
     pVolData->nls = load_nls(pVolData->szCharSet);
     if(!pVolData->nls)
       logMsg(L_EVIL, "Can't load table for charset %s",pVolData->szCharSet);
   }
   else {
     /* Use default system codepage */
     if(DosQueryCp(sizeof(ulCp),ulCp,&ulInfoLen)==NO_ERROR) {
       /* Check if mkisofs supports our CP */
       if(checkCpSupport((int)ulCp[0])) {
         sprintf(pVolData->szCharSet,"cp%d",(int)ulCp[0]);
         pVolData->nls = load_nls(pVolData->szCharSet);
         if(!pVolData->nls)
           logMsg(L_EVIL, "Can't load table for system codepage %s",pVolData->szCharSet);
       }
     }
   }

   /* ISO file */
   pVolData->isoFile=sysOpenFile(pVolData->fileName, 
        SOF_FAIL_IF_NEW | SOF_OPEN_IF_EXISTS | SOF_DENYWRITE | SOF_READONLY,
        0);
   if(!pVolData->isoFile) {
     logMsg(L_EVIL, "Can't open ISO file");
     /* Unload translation table */
     if(pVolData->nls)
       unload_nls(pVolData->nls);
     free(pVolData);
     return ERROR_ISOFS_FILEOPEN;
   }

   /* Get info from ISO file */
   lseek(pVolData->isoFile->h, ((off_t)(16 + pVolData->iSectorOffset)) <<11, 0);
   rc=read(pVolData->isoFile->h, &pVolData->ipd, sizeof(pVolData->ipd));
   logMsg(L_DBG, "ISO primary descriptor read from ISO file (%d Bytes)",rc);

   /****************************************/
   hpd = (struct hs_primary_descriptor *) &pVolData->ipd;
   if ((hpd->type[0] == ISO_VD_PRIMARY) &&
       (strncmp(hpd->id, HIGH_SIERRA_ID, sizeof(hpd->id)) == 0) &&
       (hpd->version[0] == 1)) {
           pVolData->high_sierra = 1;
           idr = (struct iso_directory_record *) hpd->root_directory_record;
           memcpy(&pVolData->chrCDName,&hpd->volume_id,32);
   }
   else
	if ((pVolData->ipd.type[0] != ISO_VD_PRIMARY) ||
	    (strncmp(pVolData->ipd.id, ISO_STANDARD_ID, sizeof(pVolData->ipd.id)) != 0) ||
	    (pVolData->ipd.version[0] != 1)) {
                logMsg(L_EVIL, "Unable to find PVD");
                /* Unload translation table */
                if(pVolData->nls)
                    unload_nls(pVolData->nls);
                sysCloseFile(pVolData->isoFile);
                free(pVolData);
                return ERROR_ISOFS_NOTISO;
	}

    if (!pVolData->high_sierra)
    {
       int block = 16;
       memcpy(&pVolData->chrCDName,&pVolData->ipd.volume_id,32);
       memcpy(&jpd, &pVolData->ipd, sizeof(pVolData->ipd));
       while (((unsigned char) jpd.type[0] != ISO_VD_END) &&
	    (strncmp(jpd.id, ISO_STANDARD_ID, sizeof(jpd.id)) == 0) &&
	    (jpd.version[0] == 1))
         {
           if( (unsigned char) jpd.type[0] == ISO_VD_SUPPLEMENTARY )
             /*
              * Find the UCS escape sequence.
              */
             if(    jpd.escape_sequences[0] == '%'
                    && jpd.escape_sequences[1] == '/'
                    && (jpd.escape_sequences[3] == '\0'
                        || jpd.escape_sequences[3] == ' ')
                    && (jpd.escape_sequences[2] == '@'
                        || jpd.escape_sequences[2] == 'C'
                        || jpd.escape_sequences[2] == 'E') )
               {
		 pVolData->got_joliet = 1;
                 break;
               }
           
           block++;
           lseek(pVolData->isoFile->h, ((off_t)(block + pVolData->iSectorOffset)) <<11, 0);
           read(pVolData->isoFile->h, &jpd, sizeof(jpd));
         }
       
       if(pVolData->got_joliet)
         switch(jpd.escape_sequences[2])
         {
         case '@':
           pVolData->ucs_level = 1;
           break;
         case 'C':
           pVolData->ucs_level = 2;
           break;
         case 'E':
           pVolData->ucs_level = 3;
           break;
         }
       
	if (pVolData->got_joliet)
            memcpy(&pVolData->ipd, &jpd, sizeof(pVolData->ipd));

        idr = (struct iso_directory_record *) pVolData->ipd.root_directory_record;
     }
   
   /****************************************/
   
   /* Fill in extent of root */
   pVolData->root_directory_record = idr;
   pVolData->idRoot = isonum_733((unsigned char *)idr->extent);/* 733 */
   pVolData->iRootExtent = pVolData->idRoot;
   pVolData->iRootSize=isonum_733((unsigned char *)idr->size);
   
   logMsg(L_DBG, "Extent of root is: %d ",pVolData->idRoot);
   
   pattach->pVolData = pVolData;
   
   pVolData->pNext = pServerData->pFirstVolume;
   pVolData->pPrev = 0;
   if (pVolData->pNext) pVolData->pNext->pPrev = pVolData;
   pServerData->pFirstVolume = pVolData;
   
   return NO_ERROR;
}


static APIRET detachVolume(ServerData * pServerData,
   struct attach * pattach)
{
   VolData * pVolData = pattach->pVolData;
   IFS_DETACH * parms = (IFS_DETACH *) pServerData->pData;
   static IFS_DETACH defparms = { 0 };

   if (pattach->cbParm != sizeof(IFS_DETACH)) parms = &defparms;
   
   logMsg(L_DBG, "detaching drive, flFlags=%lx", parms->flFlags);

   /* Open files or searches? */
   if (pVolData->cOpenFiles) {
     logMsg(L_EVIL, "volume still has %d open files",
            pVolData->cOpenFiles);
     if (!(parms->flFlags & DP_FORCE)) return ERROR_DRIVE_LOCKED;
   }
   
   if (pVolData->cSearches) {
     logMsg(L_EVIL, "volume still has %d open searches",
            pVolData->cSearches);
     if (!(parms->flFlags & DP_FORCE)) return ERROR_DRIVE_LOCKED;
   }

   /* Close the volume */
   if (pVolData->pNext) pVolData->pNext->pPrev = pVolData->pPrev;
   if (pVolData->pPrev)
      pVolData->pPrev->pNext = pVolData->pNext;
   else
      pServerData->pFirstVolume = pVolData->pNext;

   /* Close ISO file */
   if(pVolData->isoFile)
     sysCloseFile(pVolData->isoFile);

   /* Unload translation table */
   if(pVolData->nls)
     unload_nls(pVolData->nls);

   free(pVolData);
   return NO_ERROR;
}


static APIRET queryAttachmentInfo(ServerData * pServerData,
   struct attach * pattach)
{
   VolData * pVolData = pattach->pVolData;
   int len;

   len = sizeof(USHORT) + strlen(pVolData->fileName) + 1;
   if (pattach->cbParm < len) {
      logMsg(L_ERR, "FS_ATTACH buffer too small, %d bytes", pattach->cbParm);
      pattach->cbParm = len;
      return ERROR_BUFFER_OVERFLOW;
   }

   pattach->cbParm = len;
   *((USHORT *) pServerData->pData) = len - sizeof(USHORT);
   strncpy(((char *) pServerData->pData) + sizeof(USHORT), pVolData->fileName,
        len - sizeof(USHORT));
   return NO_ERROR;
}


APIRET fsAttach(ServerData * pServerData, struct attach * pattach)
{
   APIRET rc;
   
   logMsg(L_DBG, "FS_ATTACH, flag=%hd, szDev=%s, cbParm=%d",
      pattach->fsFlag, pattach->szDev, pattach->cbParm);

   if (VERIFYFIXED(pattach->szDev) ||
       (strlen(pattach->szDev) != 2) ||
       (pattach->szDev[1] != ':'))
      return ERROR_INVALID_PARAMETER;

   switch (pattach->fsFlag) {

      case FSA_ATTACH:
         rc = attachVolume(pServerData, pattach);
         memset(pServerData->pData, 0, sizeof(IFS_ATTACH)); /* burn */
         return rc;
         
      case FSA_DETACH:
         return detachVolume(pServerData, pattach);

      case FSA_ATTACH_INFO:
         return queryAttachmentInfo(pServerData, pattach);

      default:
         logMsg(L_EVIL, "unknown FS_ATTACH flag: %d", pattach->fsFlag);
         return ERROR_NOT_SUPPORTED;
   }
}

 
static APIRET getSetAllocInfo(ServerData * pServerData,
   struct fsinfo * pfsinfo)
{
   PFSALLOCATE pfsalloc;

   if (pfsinfo->fsFlag == INFO_RETRIEVE) {
      
      if (pfsinfo->cbData < sizeof(FSALLOCATE))
         return ERROR_BUFFER_OVERFLOW;

      pfsinfo->cbData = sizeof(FSALLOCATE);
      pfsalloc = (PFSALLOCATE) pServerData->pData;
      pfsalloc->idFileSystem = 0;
      pfsalloc->cSectorUnit = 1;
      pfsalloc->cUnitAvail = 0;
      if (pfsinfo->pVolData->high_sierra)
      {
          struct hs_primary_descriptor *hpd = (struct hs_primary_descriptor *) &pfsinfo->pVolData->ipd;
          pfsalloc->cUnit = isonum_733((unsigned char *)&(hpd->volume_space_size));
          pfsalloc->cbSector = isonum_723(hpd->logical_block_size);
      }
      else
      {
          pfsalloc->cUnit = isonum_733((unsigned char *)&(pfsinfo->pVolData->ipd.volume_space_size));
          pfsalloc->cbSector = isonum_723(pfsinfo->pVolData->ipd.logical_block_size);
      }
     
      return NO_ERROR;
      
   } else {
      logMsg(L_EVIL, "cannot set FSALLOCATE");
      return ERROR_NOT_SUPPORTED;
   }
}



static APIRET getSetVolSer(ServerData * pServerData,
   struct fsinfo * pfsinfo)
{
   PFSINFO pinfo;
   PVOLUMELABEL pvollabel;
  
   if (pfsinfo->fsFlag == INFO_RETRIEVE) {
      if (pfsinfo->cbData < sizeof(FSINFO))
         return ERROR_BUFFER_OVERFLOW;
      else {
        pfsinfo->cbData = sizeof(FSINFO);
        pinfo = (PFSINFO) pServerData->pData;
        * (PULONG) &pinfo->fdateCreation = 0;
        * (PULONG) &pinfo->ftimeCreation = 0;

        /* Volume Label */
        memset(pinfo->vol.szVolLabel,0,sizeof(pinfo->vol.szVolLabel));

        if(strlen((pfsinfo->pVolData->chrCDName)))
          strncpy(pinfo->vol.szVolLabel,
                  (pfsinfo->pVolData->chrCDName),
                  sizeof(pinfo->vol.szVolLabel)-1);
        else
          strcpy(pinfo->vol.szVolLabel, IFS_NAME); 
        pinfo->vol.cch = strlen(pinfo->vol.szVolLabel);
        pinfo->fdateCreation.day=(USHORT)pfsinfo->pVolData->chDrive;
        pinfo->ftimeCreation.twosecs=(USHORT)pfsinfo->pVolData->chDrive;
        return NO_ERROR;
      } 
   }
   else { 
     pvollabel = (PVOLUMELABEL) pServerData->pData;
     if
       ((pfsinfo->cbData < sizeof(VOLUMELABEL)) || (pvollabel->cch > 11))
       return ERROR_INVALID_PARAMETER;
     else {
       return ERROR_WRITE_PROTECT;
     }
   }
}


APIRET fsFsInfo(ServerData * pServerData, struct fsinfo * pfsinfo)
{
   logMsg(L_DBG, "FS_FSINFO, flag=%hd, usLevel=%hd",
      pfsinfo->fsFlag, pfsinfo->usLevel);

   switch (pfsinfo->usLevel) {

      case FSIL_ALLOC: /* 1 */
         return getSetAllocInfo(pServerData, pfsinfo);

      case FSIL_VOLSER: /* 2 */
         return getSetVolSer(pServerData, pfsinfo);

      default:
         logMsg(L_EVIL, "unknown FS_INFO flag: %d", pfsinfo->fsFlag);
         return ERROR_NOT_SUPPORTED;
         
   }
}


APIRET fsFlushBuf(ServerData * pServerData,
   struct flushbuf * pflushbuf)
{
   logMsg(L_DBG, "FS_FLUSHBUF, flag=%hd");
   return NO_ERROR;
}


APIRET fsShutdown(ServerData * pServerData,
   struct shutdown * pshutdown)
{
   return NO_ERROR;
}

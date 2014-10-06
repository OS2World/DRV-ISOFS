/* mapiso.c -- ISOFS map program.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define INCL_ERRORS
#define INCL_DOSMISC
#include <os2.h>

#include "getopt.h"
#include "types.h"
#include "ifsdint.h"

char * pszProgramName;


static void printUsage(int status)
{
   if (status)
      fprintf(stderr,
         "\nTry `%s --help' for more information.\n",
         pszProgramName);
   else
   {
      printf("\
Usage: %s [OPTION]... [DRIVE-LETTER: [ISO-FILE]]\n\
Map an ISO file on to DRIVE-LETTER.\n\
\n\
  -h, --help           Display this help and exit\n\
  -v, --version        Output version information and exit\n\
  -d, --detach         Unmap ISO volume\n\
  -f, --force          Unmap even when the volume has open files\n\
  -o, --offset <nnn>   Offset of session on CD\n\
  -j, --jcharset <CP>  Translation codepage for unicode names\n\
                       Available codepages:\n", pszProgramName);
      printf("\
                         cp437 cp737 cp775 cp850 cp852 cp855 cp857 cp860\n\
                         cp861 cp862 cp863 cp864 cp865 cp866 cp869 cp874\n\
                         iso8859-1 iso8859-2 iso8859-3 iso8859-4\n\
                         iso8859-5 iso8859-6 iso8859-7 iso8859-8\n\
                         iso8859-9 iso8859-14 iso8859-15 koi8-r\n");
      printf("\
Examples:\n\
  Map the ISO image file in c:\\image.iso on to drive X:\n\
  %s X: c:\\isoimage.raw\n\n\
  Unmap drive X:\n\
  %s X: -d\n", pszProgramName, pszProgramName);
   }

   exit(status);
}


static void DisplayDrives(void)
{
    APIRET rc;
    ULONG ulLen, ulDrives = 0;
    unsigned char szDrive[3] = "?:";
    unsigned char buf[sizeof(FSQBUFFER) + 3*CCHMAXPATH];
    unsigned char *pszFSD, *pszBasePath;
    FSQBUFFER2 *fsq = (FSQBUFFER2 *) buf;

    for (szDrive[0] = 'C'; szDrive[0] <= 'Z'; szDrive[0]++)
    {
        ulLen = sizeof(buf);
        rc = DosQueryFSAttach(szDrive, 0, FSAIL_QUERYNAME, fsq, &ulLen);
        if (!rc)
        {
            pszFSD = fsq->szName + fsq->cbName + 1;
            pszBasePath = pszFSD + fsq->cbFSDName + 1;
            if (!strcmp((char *) pszFSD, IFS_NAME))
            {
                ulDrives++;
                printf("%s => %s\n", fsq->szName, pszBasePath);
            }
        }
    }

    if (!ulDrives)
        printf("No mapped drives.\n");
}


int main(int argc, char * * argv)
{
   int iOffset=0;
   int c;
   Bool fDetach = FALSE, fForce = FALSE;
   IFS_ATTACH attachparms;
   IFS_DETACH detachparms;
   APIRET rc;
   char  *pszDrive, *pszCharSet="";

   struct option const options[] = {
      { "help", no_argument, 0, 'h' },
      { "version", no_argument, 0, 'v' },
      { "detach", no_argument, 0, 'd' },
      { "force", no_argument, 0, 'f' },
      { "jcharset", required_argument, 0, 'j' },
      { "offset", required_argument, 0, 'o' },
      { 0, 0, 0, 0 } 
   };      

   DosError(FERR_DISABLEHARDERR | FERR_ENABLEEXCEPTION);
   /* Parse the arguments. */
   pszProgramName = argv[0];
   while ((c = getopt_long(argc, argv, "?hvdfj:o:", options, 0)) != EOF)
   {
      switch (c)
      {
         case 0:
            break;

         case 'h': /* --help */
         case '?':
            printUsage(0);
            break;

         case 'v': /* --version */
            printf("mapiso - %s\n", IFS_VERSION);
            exit(0);
            break;

         case 'd': /* --delete */
            fDetach = TRUE;
            break;

         case 'f': /* --force */
            fForce = TRUE;
            break;

         case 'j': /* --jcharset */
            pszCharSet = optarg;
            break;

         case 'o': /* --offset */
            iOffset=atoi(optarg);
            break;

         default:
            printUsage(1);
      }
   }

   if ((fDetach && (optind != argc - 1)) ||
       (!fDetach && (optind != argc - 2) && (argc != 1)))
   {
      fprintf(stderr, "%s: Invalid parameters\n", pszProgramName);
      printUsage(1);
   }

   if (argc == 1)
   {
      DisplayDrives();
      return(0);
   }

   pszDrive = argv[optind++];
   /* Drive okay? */
   if ((strlen(pszDrive) != 2) ||
       (!isalpha((int) pszDrive[0])) ||
       (pszDrive[1] != ':'))
   {
      fprintf(stderr, "%s: Drive specification is incorrect\n",
         pszProgramName);
      return 1;
   }

   if (pszDrive[0] > 'Z')
      pszDrive[0] -= 'a' - 'A';

   memset(&detachparms, 0, sizeof(detachparms));
   if (fDetach)
   {
      if (fForce)
         detachparms.flFlags = DP_FORCE;

      /* Unmap the volume attached to the specified drive. */
      rc = DosFSAttach((PSZ) pszDrive, (PSZ) IFS_NAME,
         &detachparms, sizeof(detachparms), FS_DETACH);
      if (!rc)
         printf ("Drive %s deleted\n", pszDrive);
   }
   else
   {
      memset(&attachparms, 0, sizeof(attachparms));
      if (DosQueryPathInfo ((PSZ) argv[optind], FIL_QUERYFULLNAME,
            attachparms.szBasePath, sizeof(attachparms.szBasePath)))
         strcpy (attachparms.szBasePath, argv[optind]);

      if (pszCharSet)
         strcpy(attachparms.szCharSet, pszCharSet);

      attachparms.iOffset=iOffset;
      /* Send the attachment request to the FSD. */
      rc = DosFSAttach((PSZ) pszDrive, (PSZ) IFS_NAME,
         &attachparms, sizeof(attachparms), FS_ATTACH);
      if (rc == ERROR_ALREADY_ASSIGNED)
      {
         DosFSAttach((PSZ) pszDrive, (PSZ) IFS_NAME,
            &detachparms, sizeof(detachparms), FS_DETACH);
         rc = DosFSAttach((PSZ) pszDrive, (PSZ) IFS_NAME,
            &attachparms, sizeof(attachparms), FS_ATTACH);
      }

      if (!rc)
         printf ("%s => %s\n", pszDrive, attachparms.szBasePath);
   }

   if (rc)
   {
      fprintf(stderr, "%s: Error %ld %smapping %s volume\n",
          pszProgramName, rc, fDetach ? "un" : "", IFS_NAME);
      switch(rc)
      {
         case ERROR_ISOFS_INVALIDOFFSET:
            fprintf(stderr, "Invalid offset.\n");
            break;
         case ERROR_ISOFS_FILEOPEN:
            fprintf(stderr, "Can't open ISO image file.\n");
            break;
         case ERROR_ISOFS_NOTISO:
            fprintf(stderr, "Image file is not in ISO format.\n");
            break;
         case ERROR_STUBFSD_DAEMON_NOT_RUN:
            fprintf(stderr, "The %s daemon is not running.\n", IFS_NAME);
            break;
         case ERROR_INVALID_DRIVE:
            fprintf(stderr, "The drive letter is not in use.\n");
            break;
         case ERROR_ALREADY_ASSIGNED:
            fprintf(stderr, "The drive letter is already in use.\n");
            break;
      }
   }

   return rc;
}

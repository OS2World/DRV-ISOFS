/* @(#)isoinfo.c	1.24 01/11/01 joerg */
#ifndef lint
static	char sccsid[] =
	"@(#)isoinfo.c	1.24 01/11/01 joerg";
#endif
/*
 * File isoinfo.c - dump iso9660 information.
 *

   Modifications for ISOFS:
   Copyright (C) 2005 Paul Ratcliffe

   Written by Eric Youngdale (1993).

   Copyright 1993 Yggdrasil Computing, Incorporated

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * Simple program to dump contents of iso9660 image in more usable format.
 *
 * Usage:
 * To list contents of image (with or without RR):
 *	isoinfo -l [-R] -i imagefile
 * To extract file from image:
 *	isoinfo -i imagefile -x xtractfile > outfile
 * To generate a "find" like list of files:
 *	isoinfo -f -i imagefile
 */

#include <stdio.h>
#include <malloc.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

#include <mconfig.h>
#include <strdefs.h>

#include "iso9660.h"
#include "sysdep.h"

#include <unls.h>

#ifndef __SVR4
extern int optind;
extern char *optarg;
extern int getopt (int __argc, char **__argv, char *__optstring);
#endif

#ifndef S_ISLNK
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISSOCK
# ifdef S_IFSOCK
#   define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
# else
#   define S_ISSOCK(m)	(0)
# endif
#endif

File *infile;
int use_rock = 0;
int use_joliet = 0;
int do_listing = 0;
int do_find = 0;
int do_sectors = 0;
int do_pathtab = 0;
int do_pvd = 0;
char * xtract = 0;
int su_version = 0;
int aa_version = 0;
int ucs_level = 0;

struct stat fstat_buf;
char name_buf[256];
char xname[256];
unsigned char date_buf[7];
unsigned int sector_offset = 0;

unsigned char buffer[2048];

struct nls_table *nls;

#define PAGE sizeof(buffer)

#define ISODCL(from, to) (to - from + 1)


int	isonum_721	__PR((char * p));
int	isonum_723	__PR((char * p));
int	isonum_731	__PR((char * p));
int	isonum_733	__PR((unsigned char * p));
void	printchars	__PR((char *s, int n));
void	dump_pathtab	__PR((int block, int size, int high_sierra));
int	parse_rr	__PR((unsigned char * pnt, int len, int cont_flag));
void	find_rr		__PR((struct iso_directory_record * idr, unsigned char **pntp, int *lenp));
int	dump_rr		__PR((struct iso_directory_record * idr));
void	dump_stat	__PR((int extent));
void	extract_file	__PR((struct iso_directory_record * idr));
void	parse_dir	__PR((char * rootname, int extent, int len, int high_sierra));
void	usage		__PR((int excode));
int	main		__PR((int argc, char *argv[]));

int
isonum_721 (p)
	char	*p;
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8));
}

int
isonum_723 (p)
	char * p;
{
#if 0
	if (p[0] != p[3] || p[1] != p[2]) {
#ifdef	USE_LIBSCHILY
		comerrno(EX_BAD, "invalid format 7.2.3 number\n");
#else
		fprintf (stderr, "invalid format 7.2.3 number\n");
		exit (1);
#endif
	}
#endif
	return (isonum_721 (p));
}

int
isonum_731 (p)
	char	*p;
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}


int
isonum_733 (p)
	unsigned char	*p;
{
	return (isonum_731 ((char *)p));
}

void
printchars(s, n)
	char	*s;
	int	n;
{
	int	i;
	char	*p;

	for (;n > 0 && *s; n--) {
		if (*s == ' ') {
			p = s;
			i = n;
			while (--i >= 0 && *p++ == ' ')
				;
			if (i <= 0)
				break;
		}
		putchar(*s++);
	}
}

void
dump_pathtab(block, size, high_sierra)
	int	block;
	int	size;
        int     high_sierra;
{
    unsigned char * buf;
    int    offset;
    int    idx;
    int    extent;
    int    pindex;
    int    j;
    int    len;
    int    jlen;
    char   namebuf[255];
    unsigned char uh, ul, uc, *up;
    

    printf("Path table starts at block %d, size %d\n", block, size);
    
    buf = (unsigned char *) malloc(size);
    
    lseek(infile->h, ((off_t)(block - sector_offset)) << 11, SEEK_SET);
    read(infile->h, buf, size);
    
    offset = 0;
    idx = 1;
    while(offset < size)
    {
        if (high_sierra) {
	    len    = buf[offset + 5];
	    extent = isonum_731((char *)buf + offset);
        } else {
	    len    = buf[offset];
	    extent = isonum_731((char *)buf + offset + 2);
        }

	pindex = isonum_721((char *)buf + offset + 6);
	switch( ucs_level )
	{
	case 3:
	case 2:
	case 1:
	    jlen = len/2;
	    namebuf[0] = '\0';
	    for(j=0; j < jlen; j++)
	    {
		uh = buf[offset + 8 + j*2];
		ul = buf[offset + 8 + j*2+1];

		up = nls->page_uni2charset[uh];

		if (up == NULL)
			uc = '\0';
		else
			uc = up[ul];

		namebuf[j] = uc ? uc : '_';
	    }
	    printf("%4d: %4d %x %.*s\n", idx, pindex, extent, jlen, namebuf);
	    break;
	case 0:
	    printf("%4d: %4d %x %.*s\n", idx, pindex, extent, len, buf + offset + 8);
	}

	idx++;
	offset += 8 + len;
	if( offset & 1) offset++;
    }

    free(buf);
}

int parse_rr(pnt, len, cont_flag)
	unsigned char	*pnt;
	int		len;
	int		cont_flag;
{
	int slen;
	int ncount;
	/*int extent;*/
	int cont_extent, cont_offset, cont_size;
	int /*flag1,*/ flag2;
	unsigned char *pnts;
	char symlinkname[1024];
	int goof;

	symlinkname[0] = 0;

	cont_extent = cont_offset = cont_size = 0;

	ncount = 0;
	/*flag1 =*/ flag2 = 0;
	while(len >= 4) {
		if(pnt[3] != 1 && pnt[3] != 2) {
		  printf("**BAD RRVERSION (%d)\n", pnt[3]);
		  return 0;	/* JS ??? Is this right ??? */
		}
		ncount++;
		/*if(pnt[0] == 'R' && pnt[1] == 'R') flag1 = pnt[4] & 0xff;*/
		if(strncmp((char *)pnt, "PX", 2) == 0) flag2 |= 1;	/* POSIX attributes */
		if(strncmp((char *)pnt, "PN", 2) == 0) flag2 |= 2;	/* POSIX device number */
		if(strncmp((char *)pnt, "SL", 2) == 0) flag2 |= 4;	/* Symlink */
		if(strncmp((char *)pnt, "NM", 2) == 0) flag2 |= 8;	/* Alternate Name */
		if(strncmp((char *)pnt, "CL", 2) == 0) flag2 |= 16;	/* Child link */
		if(strncmp((char *)pnt, "PL", 2) == 0) flag2 |= 32;	/* Parent link */
		if(strncmp((char *)pnt, "RE", 2) == 0) flag2 |= 64;	/* Relocated Direcotry */
		if(strncmp((char *)pnt, "TF", 2) == 0) flag2 |= 128;	/* Time stamp */
		if(strncmp((char *)pnt, "SP", 2) == 0){flag2 |= 1024;	/* SUSP record */
			su_version = pnt[3] & 0xff;
		}
		if(strncmp((char *)pnt, "AA", 2) == 0){flag2 |= 2048;	/* Apple Signature record */
			aa_version = pnt[3] & 0xff;
		}

		if(strncmp((char *)pnt, "PX", 2) == 0) {		/* POSIX attributes */
			fstat_buf.st_mode = isonum_733(pnt+4);
			fstat_buf.st_nlink = isonum_733(pnt+12);
			fstat_buf.st_uid = isonum_733(pnt+20);
			fstat_buf.st_gid = isonum_733(pnt+28);
		}

		if(strncmp((char *)pnt, "NM", 2) == 0) {		/* Alternate Name */
		  strncpy(name_buf, (char *)(pnt+5), pnt[2] - 5);
		  name_buf[pnt[2] - 5] = 0;
		}

		if(strncmp((char *)pnt, "CE", 2) == 0) {		/* Continuation Area */
			cont_extent = isonum_733(pnt+4);
			cont_offset = isonum_733(pnt+12);
			cont_size = isonum_733(pnt+20);
		}

		/*if(strncmp((char *)pnt, "PL", 2) == 0 || strncmp((char *)pnt, "CL", 2) == 0) {
			extent = isonum_733(pnt+4);
		}*/

		if(strncmp((char *)pnt, "SL", 2) == 0) {		/* Symlink */
		        /*int cflag;*/

			/*cflag = pnt[4];*/
			pnts = pnt+5;
			slen = pnt[2] - 5;
			while(slen >= 1){
				switch(pnts[0] & 0xfe){
				case 0:
					strncat(symlinkname, (char *)(pnts+2), pnts[1]);
					break;
				case 2:
					strcat (symlinkname, ".");
					break;
				case 4:
					strcat (symlinkname, "..");
					break;
				case 8:
					if((pnts[0] & 1) == 0)strcat (symlinkname, "\\");
					break;
				case 16:
					strcat(symlinkname,"/mnt");
					printf("Warning - mount point requested");
					break;
				case 32:
					strcat(symlinkname,"kafka");
					printf("Warning - host_name requested");
					break;
				default:
					printf("Reserved bit setting in symlink");
					goof++;
					break;
				}
				if((pnts[0] & 0xfe) && pnts[1] != 0) {
					printf("Incorrect length in symlink component");
				}
				if((pnts[0] & 1) == 0) strcat(symlinkname,"\\");

				slen -= (pnts[1] + 2);
				pnts += (pnts[1] + 2);
				if(xname[0] == 0) strcpy(xname, "-> ");
				strcat(xname, symlinkname);
		       }
			symlinkname[0] = 0;
		}

		len -= pnt[2];
		pnt += pnt[2];
		if(len <= 3 && cont_extent) {
		  unsigned char sector[2048];
		  lseek(infile->h, ((off_t)(cont_extent - sector_offset)) << 11, SEEK_SET);
		  read(infile->h, sector, sizeof(sector));
		  flag2 |= parse_rr(&sector[cont_offset], cont_size, 1);
		}
	}
	return flag2;
}

void
find_rr(idr, pntp, lenp)
	struct iso_directory_record *idr;
	unsigned char	**pntp;
	int	*lenp;
{
	struct iso_xa_dir_record *xadp;
	int len;
	unsigned char * pnt;

	len = idr->length[0] & 0xff;
	len -= sizeof(struct iso_directory_record);
	len += sizeof(idr->name);
	len -= idr->name_len[0];

	pnt = (unsigned char *) idr;
	pnt += sizeof(struct iso_directory_record);
	pnt -= sizeof(idr->name);
	pnt += idr->name_len[0];
	if((idr->name_len[0] & 1) == 0){
		pnt++;
		len--;
	}
	if (len >= 14) {
		xadp = (struct iso_xa_dir_record *)pnt;

		if (xadp->signature[0] == 'X' && xadp->signature[1] == 'A'
				&& xadp->reserved[0] == '\0') {
			len -= 14;
			pnt += 14;
		}
	}
	*pntp = pnt;
	*lenp = len;
}

int
dump_rr(idr)
	struct iso_directory_record *idr;
{
	int len;
	unsigned char * pnt;

	find_rr(idr, &pnt, &len);
	return (parse_rr(pnt, len, 0));
}

struct todo
{
  struct todo * next;
  char * name;
  int extent;
  int length;
};

struct todo * todo_idr = NULL;

char * months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		       "Aug", "Sep", "Oct", "Nov", "Dec"};

void
dump_stat(extent)
	int	extent;
{
  int i;
  char outline[80];

  memset(outline, ' ', sizeof(outline));

  if(S_ISREG(fstat_buf.st_mode))
    outline[0] = '-';
  else   if(S_ISDIR(fstat_buf.st_mode))
    outline[0] = 'd';
  else   if(S_ISLNK(fstat_buf.st_mode))
    outline[0] = 'l';
  else   if(S_ISCHR(fstat_buf.st_mode))
    outline[0] = 'c';
  else   if(S_ISBLK(fstat_buf.st_mode))
    outline[0] = 'b';
  else   if(S_ISFIFO(fstat_buf.st_mode))
    outline[0] = 'f';
  else   if(S_ISSOCK(fstat_buf.st_mode))
    outline[0] = 's';
  else
    outline[0] = '?';

  memset(outline+1, '-', 9);
  if( fstat_buf.st_mode & S_IRUSR )
    outline[1] = 'r';
  if( fstat_buf.st_mode & S_IWUSR )
    outline[2] = 'w';
  if( fstat_buf.st_mode & S_IXUSR )
    outline[3] = 'x';

  if( fstat_buf.st_mode & S_IRGRP )
    outline[4] = 'r';
  if( fstat_buf.st_mode & S_IWGRP )
    outline[5] = 'w';
  if( fstat_buf.st_mode & S_IXGRP )
    outline[6] = 'x';

  if( fstat_buf.st_mode & S_IROTH )
    outline[7] = 'r';
  if( fstat_buf.st_mode & S_IWOTH )
    outline[8] = 'w';
  if( fstat_buf.st_mode & S_IXOTH )
    outline[9] = 'x';

  /*
   * XXX This is totally ugly code from Eric.
   * XXX If one field is wider than expected then it is truncated.
   */
  sprintf(outline+11, "%3ld", (long)fstat_buf.st_nlink);
  sprintf(outline+15, "%4lo", (unsigned long)fstat_buf.st_uid);
  sprintf(outline+20, "%4lo", (unsigned long)fstat_buf.st_gid);
  sprintf(outline+30, "%10ld", (long)fstat_buf.st_size);

  if (do_sectors == 0) {
    sprintf(outline+30, "%10ld", (long)fstat_buf.st_size);
  } else {
    sprintf(outline+30, "%10ld", (long)((fstat_buf.st_size+PAGE-1)/PAGE));
  }

  if( date_buf[1] >= 1 && date_buf[1] <= 12 )
    {
      memcpy(outline+41, months[date_buf[1]-1], 3);
    }

  sprintf(outline+45, "%2d", date_buf[2]);
  sprintf(outline+48, "%4d", date_buf[0]+1900);

  sprintf(outline+53, "[%7d]", extent);	/* XXX up to 20 GB */

  for(i=0; i<63; i++)
    if(outline[i] == 0) outline[i] = ' ';
  outline[63] = 0;

  printf("%s %s %s\n", outline, name_buf, xname);
}

void
extract_file(idr)
	struct iso_directory_record *idr;
{
  int extent, len, tlen;
  unsigned char buff[2048];

  extent = isonum_733((unsigned char *)idr->extent);
  len = isonum_733((unsigned char *)idr->size);

  while(len > 0)
    {
      lseek(infile->h, ((off_t)(extent - sector_offset)) << 11, SEEK_SET);
      tlen = (len > sizeof(buff) ? sizeof(buff) : len);
      read(infile->h, buff, tlen);
      len -= tlen;
      extent++;
      write(1, buff, tlen);
    }
}

void
parse_dir(rootname, extent, len, high_sierra)
	char	*rootname;
	int	extent;
	int	len;
        int     high_sierra;
{
  char testname[256], flags;
  struct todo * td;
  int i;
  struct iso_directory_record * idr;
  struct hs_directory_record * hdr;
  unsigned char uh, ul, uc, *up;


  if( do_listing)
    printf("\nDirectory listing of %s\n", rootname);

  while(len > 0 )
    {
      lseek(infile->h, ((off_t)(extent - sector_offset)) << 11, SEEK_SET);
      read(infile->h, buffer, sizeof(buffer));
      len -= sizeof(buffer);
      extent++;
      i = 0;
      while(1==1){
	idr = (struct iso_directory_record *) &buffer[i];
	hdr = (struct hs_directory_record *) &buffer[i];
	if(idr->length[0] == 0) break;
	memset(&fstat_buf, 0, sizeof(fstat_buf));
	name_buf[0] = xname[0] = 0;
	fstat_buf.st_size = (off_t)isonum_733((unsigned char *)idr->size);
        flags = high_sierra ? hdr->flags[0] : idr->flags[0];
	if(flags & ISO_DIRECTORY)
	  fstat_buf.st_mode |= S_IFDIR;
	else
	  fstat_buf.st_mode |= S_IFREG;	
	if(idr->name_len[0] == 1 && idr->name[0] == 0)
	  strcpy(name_buf, ".");
	else if(idr->name_len[0] == 1 && idr->name[0] == 1)
	  strcpy(name_buf, "..");
	else {
	  switch(ucs_level)
	    {
	    case 3:
	    case 2:
	    case 1:
	      /*
	       * Unicode name.  Convert as best we can.
	       */
	      {
		int j;

		name_buf[0] = '\0';
		for(j=0; j < idr->name_len[0] / 2; j++)
		  {
		    uh = idr->name[j*2];
		    ul = idr->name[j*2+1];

		    up = nls->page_uni2charset[uh];

		    if (up == NULL)
			uc = '\0';
		    else
			uc = up[ul];

		    name_buf[j] = uc ? uc : '_';
		  }
		name_buf[idr->name_len[0]/2] = '\0';
	      }
	      break;
	    case 0:
	      /*
	       * Normal non-Unicode name.
	       */
	      strncpy(name_buf, idr->name, idr->name_len[0]);
	      name_buf[idr->name_len[0]] = 0;
	      break;
	    default:
	      /*
	       * Don't know how to do these yet.  Maybe they are the same
	       * as one of the above.
	       */
	      strcpy(name_buf, "_");
	    }
	}
	memcpy(date_buf, idr->date, sizeof(date_buf));
	if(use_rock) dump_rr(idr);
	if(   (flags & ISO_DIRECTORY) != 0
	   && (idr->name_len[0] != 1
	       || (idr->name[0] != 0 && idr->name[0] != 1)))
	  {
	    /*
	     * Add this directory to the todo list.
	     */
	    td = todo_idr;
	    if( td != NULL ) 
	      {
		while(td->next != NULL) td = td->next;
		td->next = (struct todo *) malloc(sizeof(*td));
		td = td->next;
	      }
	    else
	      {
		todo_idr = td = (struct todo *) malloc(sizeof(*td));
	      }
	    td->next = NULL;
	    td->extent = isonum_733((unsigned char *)idr->extent);
	    td->length = isonum_733((unsigned char *)idr->size);
	    td->name = (char *) malloc(strlen(rootname) 
				       + strlen(name_buf) + 2);
	    strcpy(td->name, rootname);
	    strcat(td->name, name_buf);
	    strcat(td->name, "\\");
	  }
	else
	  {
	    strcpy(testname, rootname);
	    strcat(testname, name_buf);
	    if (xtract && stricmp(xtract, testname + 1) == 0)
		extract_file(idr);
	  }
	if(   do_find
	   && (idr->name_len[0] != 1
	       || (idr->name[0] != 0 && idr->name[0] != 1)))
	  {
	    strcpy(testname, rootname);
	    strcat(testname, name_buf);
	    printf("%s\n", testname);
	  }
	if(do_listing)
	  dump_stat(isonum_733((unsigned char *)idr->extent));
	i += buffer[i];
	if (i > 2048 - (sizeof(*idr) - sizeof(idr->name) + 1)) break;
      }
    }
}

void
usage(excode)
	int	excode;
{
	printf("Usage: isoinfo [options]\n");

	printf("Options:\n");
	printf("  -h		Print this help\n");
	printf("  -d		Print information from the primary volume descriptor\n");
	printf("  -f		Generate output similar to 'find .  -print'\n");
	printf("  -J		Print information from Joliet extensions\n");
	printf("  -j charset	Use charset to display Joliet file names\n");
	printf("  -l		Generate output similar to 'ls -lR'\n");
	printf("  -p		Print Path Table\n");
	printf("  -R		Print information from Rock Ridge extensions\n");
	printf("  -s		Print file size info. in multiples of sector size (%ld bytes)\n", (long) PAGE);
	printf("  -N sector	Sector number where ISO image should start on CD\n");
	printf("  -T sector	Sector number where actual session starts on CD\n");
	printf("  -i filename	Filename to read ISO-9660 image from\n");
	printf("  -x pathname	Extract specified file to stdout\n");
	exit(excode);
}

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
  int c;
  char * filename = NULL;
  int toc_offset = 0;
  int extent;
  int high_sierra = 0;
  int got_joliet = 0;
  struct todo * td;
  struct iso_primary_descriptor ipd;
  struct iso_primary_descriptor jpd;
  struct hs_primary_descriptor *hpd = (struct hs_primary_descriptor *) &ipd;
  struct iso_directory_record * idr = 0;
  struct hs_directory_record * hdr = 0;
  char *charset = NULL;

  if(argc < 2)
	usage(-1);
  while ((c = getopt(argc, argv, "hdpi:JRlx:fsN:T:j:")) != EOF)
    switch (c)
      {
      case 'h':
	usage(0);
	break;

      case 'd':
	do_pvd++;
	break;

      case 'f':
	do_find++;
	break;

      case 's':
	do_sectors++;
	break;

      case 'p':
	do_pathtab++;
	break;

      case 'R':
	use_rock++;
	break;

      case 'J':
	use_joliet++;
	break;

      case 'j':
	use_joliet++;
	charset = optarg;
	break;

      case 'l':
	do_listing++;
	break;

      case 'T':
	/*
	 * This is used if we have a complete multi-session
	 * disc that we want/need to play with.
	 * Here we specify the offset where we want to
	 * start searching for the TOC.
	 */
	toc_offset = atol(optarg);
	break;

      case 'N':
	/*
	 * Use this if we have an image of a single session
	 * and we need to list the directory contents.
	 * This is the session block number of the start
	 * of the session.
	 */
	sector_offset = atol(optarg);
	break;

      case 'i':
	filename = optarg;
	break;

      case 'x':
	xtract = optarg;
	if (*xtract == '\\' || *xtract == '/')
	    xtract++;

	break;

      default:
	usage(-1);
      }

	init_nls();		/* Initialize UNICODE tables */
	if (charset == NULL) {
#if	(defined(__CYGWIN32__) || defined(__CYGWIN__)) && !defined(IS_CYGWIN_1)
		nls = load_nls("cp437");
#else
		nls = load_nls("iso8859-1");
#endif
	} else {
		if (strcmp(charset, "default") == 0)
			nls = load_nls_default();
		else
			nls = load_nls(charset);
	}
	if (nls == NULL) {	/* Unknown charset specified */
		fprintf(stderr, "Unknown charset: %s\nKnown charsets are:\n",
							charset);
		list_nls();	/* List all known charset names */
		exit(1);
	}

  if( filename == NULL )
  {
#ifdef	USE_LIBSCHILY
	comerrno(EX_BAD, "Error - file not specified\n");
#else
	fprintf(stderr, "Error - file not specified\n");
  	exit(1);
#endif
  }

    infile = sysOpenFile(filename, 
        SOF_FAIL_IF_NEW | SOF_OPEN_IF_EXISTS | SOF_DENYWRITE | SOF_READONLY,
        0);
  if(!infile)
  {
#ifdef	USE_LIBSCHILY
	comerr("Unable to open file %s\n", filename);
#else
	fprintf(stderr,"Unable to open file %s\n", filename);
	exit(1);
#endif
  }

  /*
   * Absolute sector offset, so don't subtract sector_offset here.
   */
  lseek(infile->h, ((off_t)(16 + toc_offset)) <<11, SEEK_SET);
  read(infile->h, &ipd, sizeof(ipd));
	/*
	 * High sierra:
	 *
	 *	DESC TYPE	== 1 (VD_SFS)	offset 8	len 1
	 *	STR ID		== "CDROM"	offset 9	len 5
	 *	STD_VER		== 1		offset 14	len 1
	 */
	if ((hpd->type[0] == ISO_VD_PRIMARY) &&
	    (strncmp(hpd->id, HIGH_SIERRA_ID, sizeof(hpd->id)) == 0) &&
	    (hpd->version[0] == 1)) {
                high_sierra = 1;
	}
        else
	/*
	 * ISO 9660:
	 *
	 *	DESC TYPE	== 1 (VD_PVD)	offset 0	len 1
	 *	STR ID		== "CD001"	offset 1	len 5
	 *	STD_VER		== 1		offset 6	len 1
	 */
	if ((ipd.type[0] != ISO_VD_PRIMARY) ||
	    (strncmp(ipd.id, ISO_STANDARD_ID, sizeof(ipd.id)) != 0) ||
	    (ipd.version[0] != 1)) {
		printf("CD-ROM is NOT in ISO 9660 format\n");
		exit(1);
	}

  if (high_sierra)
    hdr = (struct hs_directory_record *)hpd->root_directory_record;
  else
    idr = (struct iso_directory_record *)ipd.root_directory_record;

  if (do_pvd) {
    if (high_sierra) {
	printf("CD-ROM is in High Sierra format\n");
	printf("System id: ");
	printchars(hpd->system_id, 32);
	putchar('\n');
	printf("Volume id: ");
	printchars(hpd->volume_id, 40);
	putchar('\n');

	printf("Volume set id: ");
	printchars(hpd->volume_set_id, 128);
	putchar('\n');
	printf("Publisher id: ");
	printchars(hpd->publisher_id, 128);
	putchar('\n');
	printf("Data preparer id: ");
	printchars(hpd->preparer_id, 128);
	putchar('\n');
	printf("Application id: ");
	printchars(hpd->application_id, 128);
	putchar('\n');

	printf("Copyright File id: ");
	printchars(hpd->copyright_file_id, 32);
	putchar('\n');
	printf("Abstract File id: ");
	printchars(hpd->abstract_file_id, 32);
	putchar('\n');

	printf("Volume set size is: %d\n", isonum_723(hpd->volume_set_size));
	printf("Volume set sequence number is: %d\n", isonum_723(hpd->volume_sequence_number));
	printf("Logical block size is: %d\n", isonum_723(hpd->logical_block_size));
	printf("Volume size is: %d\n", isonum_733((unsigned char *)hpd->volume_space_size));
    }
    else {
	printf("CD-ROM is in ISO 9660 format\n");
	printf("System id: ");
	printchars(ipd.system_id, 32);
	putchar('\n');
	printf("Volume id: ");
	printchars(ipd.volume_id, 32);
	putchar('\n');

	printf("Volume set id: ");
	printchars(ipd.volume_set_id, 128);
	putchar('\n');
	printf("Publisher id: ");
	printchars(ipd.publisher_id, 128);
	putchar('\n');
	printf("Data preparer id: ");
	printchars(ipd.preparer_id, 128);
	putchar('\n');
	printf("Application id: ");
	printchars(ipd.application_id, 128);
	putchar('\n');

	printf("Copyright File id: ");
	printchars(ipd.copyright_file_id, 37);
	putchar('\n');
	printf("Abstract File id: ");
	printchars(ipd.abstract_file_id, 37);
	putchar('\n');
	printf("Bibliographic File id: ");
	printchars(ipd.bibliographic_file_id, 37);
	putchar('\n');

	printf("Volume set size is: %d\n", isonum_723(ipd.volume_set_size));
	printf("Volume set sequence number is: %d\n", isonum_723(ipd.volume_sequence_number));
	printf("Logical block size is: %d\n", isonum_723(ipd.logical_block_size));
	printf("Volume size is: %d\n", isonum_733((unsigned char *)ipd.volume_space_size));
    }
  }

    if (!high_sierra && (use_joliet || do_pvd))
    {
      int block = 16;
      memcpy(&jpd, &ipd, sizeof(ipd));
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
	      && (    jpd.escape_sequences[2] == '@'
		   || jpd.escape_sequences[2] == 'C'
		   || jpd.escape_sequences[2] == 'E') )
	    {
	      got_joliet = 1;
	      break;
	    }

	  block++;
	  lseek(infile->h, ((off_t)(block + toc_offset)) <<11, SEEK_SET);
	  read(infile->h, &jpd, sizeof(jpd));
	}

      if(use_joliet && !got_joliet)
	{
#ifdef	USE_LIBSCHILY
	  comerrno(EX_BAD, "Unable to find Joliet SVD\n");
#else
	  fprintf(stderr, "Unable to find Joliet SVD\n");
	  exit(1);
#endif
	}

      if (got_joliet)
        switch(jpd.escape_sequences[2])
	{
	case '@':
	  ucs_level = 1;
	  break;
	case 'C':
	  ucs_level = 2;
	  break;
	case 'E':
	  ucs_level = 3;
	  break;
	}

      if( ucs_level > 3 )
	{
#ifdef	USE_LIBSCHILY
	  comerrno(EX_BAD, "Don't know what ucs_level == %d means\n", ucs_level);
#else
	  fprintf(stderr, "Don't know what ucs_level == %d means\n", ucs_level);
	  exit(1);
#endif
	}
    }

	if (do_pvd) {
	    if (!high_sierra) {
		if (ucs_level > 0)
			printf("Joliet with UCS level %d found\n", ucs_level);
		else
			printf("NO Joliet present\n");

		extent = isonum_733((unsigned char *)idr->extent);

		lseek(infile->h, ((off_t)(extent - sector_offset)) <<11, SEEK_SET);
		read(infile->h, buffer, sizeof(buffer));
		idr = (struct iso_directory_record *) buffer;
		if ((c = dump_rr(idr)) != 0) {
/*			printf("RR %X %d\n", c, c);*/
			if (c & 1024)
				printf("Rock Ridge signatures version %d found\n", su_version);
			else
				printf("Bad Rock Ridge signatures found (SU record missing)\n");
			/*
			 * This is currently a no op!
			 * We need to check the first plain file instead of
			 * the '.' entry in the root directory.
			 */
			if (c & 2048)
				printf("Apple signatures version %d found\n", aa_version);
		} else {
			printf("NO Rock Ridge present\n");
		}
	    }
	    exit(0);
	}

	if (got_joliet)
		idr = (struct iso_directory_record *)jpd.root_directory_record;

  if (do_pathtab) {
	if (got_joliet) {
		dump_pathtab(isonum_731(jpd.type_l_path_table), 
		   isonum_733((unsigned char *)jpd.path_table_size), 0);
	} else if (high_sierra) {
		dump_pathtab(isonum_731(hpd->type_l_path_table), 
		   isonum_733((unsigned char *)hpd->path_table_size), 1);
	} else {
		dump_pathtab(isonum_731(ipd.type_l_path_table), 
		   isonum_733((unsigned char *)ipd.path_table_size), 0);
	}
    }

  if (high_sierra)
    parse_dir("\\", isonum_733((unsigned char *)hdr->extent), isonum_733((unsigned char *)hdr->size), 1);
  else
    parse_dir("\\", isonum_733((unsigned char *)idr->extent), isonum_733((unsigned char *)idr->size), 0);

  td = todo_idr;
  while(td)
    {
      struct todo *td2;

      parse_dir(td->name, td->extent, td->length, high_sierra);
      free(td->name);
      td2 = td->next;
      free(td);
      td = td2;
    }

  sysCloseFile(infile);
  return(0);
}

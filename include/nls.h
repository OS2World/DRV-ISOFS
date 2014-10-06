/* @(#)nls.h	1.2 00/04/26 2000 J. Schilling */
/*
 *	Modifications to make the code portable Copyright (c) 2000 J. Schilling
 *	Thanks to Georgy Salnikov <sge@nmr.nioch.nsc.ru>
 *
 *	Code taken from the Linux kernel.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _NLS_H
#define _NLS_H

#include <unls.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT

#define CONFIG_NLS_CODEPAGE_437
#define CONFIG_NLS_CODEPAGE_737
#define CONFIG_NLS_CODEPAGE_775
#define CONFIG_NLS_CODEPAGE_850
#define CONFIG_NLS_CODEPAGE_852
#define CONFIG_NLS_CODEPAGE_855
#define CONFIG_NLS_CODEPAGE_857
#define CONFIG_NLS_CODEPAGE_860
#define CONFIG_NLS_CODEPAGE_861
#define CONFIG_NLS_CODEPAGE_862
#define CONFIG_NLS_CODEPAGE_863
#define CONFIG_NLS_CODEPAGE_864
#define CONFIG_NLS_CODEPAGE_865
#define CONFIG_NLS_CODEPAGE_866
#define CONFIG_NLS_CODEPAGE_869
#define CONFIG_NLS_CODEPAGE_874
#define CONFIG_NLS_ISO8859_1
#define CONFIG_NLS_ISO8859_2
#define CONFIG_NLS_ISO8859_3
#define CONFIG_NLS_ISO8859_4
#define CONFIG_NLS_ISO8859_5
#define CONFIG_NLS_ISO8859_6
#define CONFIG_NLS_ISO8859_7
#define CONFIG_NLS_ISO8859_8
#define CONFIG_NLS_ISO8859_9
#define CONFIG_NLS_ISO8859_14
#define CONFIG_NLS_ISO8859_15
#define CONFIG_NLS_KOI8_R

#define CONFIG_NLS_MAC_ROMAN

extern int init_nls_iso8859_1	(void);
extern int init_nls_iso8859_2	(void);
extern int init_nls_iso8859_3	(void);
extern int init_nls_iso8859_4	(void);
extern int init_nls_iso8859_5	(void);
extern int init_nls_iso8859_6	(void);
extern int init_nls_iso8859_7	(void);
extern int init_nls_iso8859_8	(void);
extern int init_nls_iso8859_9	(void);
extern int init_nls_iso8859_14	(void);
extern int init_nls_iso8859_15	(void);
extern int init_nls_cp437	(void);
extern int init_nls_cp737	(void);
extern int init_nls_cp775	(void);
extern int init_nls_cp850	(void);
extern int init_nls_cp852	(void);
extern int init_nls_cp855	(void);
extern int init_nls_cp857	(void);
extern int init_nls_cp860	(void);
extern int init_nls_cp861	(void);
extern int init_nls_cp862	(void);
extern int init_nls_cp863	(void);
extern int init_nls_cp864	(void);
extern int init_nls_cp865	(void);
extern int init_nls_cp866	(void);
extern int init_nls_cp869	(void);
extern int init_nls_cp874	(void);
extern int init_nls_koi8_r	(void);

extern int init_nls_mac_roman	(void);

#endif	/* _NLS_H */

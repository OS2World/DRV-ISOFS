/* isodrive.h -- ISOFS GUI interface program.
   Copyright (C) 1999 Eelco Dolstra (edolstra@students.cs.uu.nl).

   Modifications for ISOFS:
   Copyright (C) 2000 Chris Wohlgemuth (chris.wohlgemuth@cityweb.de)
   Copyright (C) 2005-2006 Paul Ratcliffe
   Copyright (C) 2017 Rich Walsh

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

/* dialog IDs */
#define OK_PB           201
#define CANCEL_PB       202

#define MAIN_DLG        300
#define MAIN_TX         301
#define MAIN_LB         302
#define MARK_TX         303
#define OPTS_PB         304
#define PLUS_PB         305
#define MINUS_PB        306

#define MNT_DLG         400
#define FILE_TX         401
#define FILE_EF         402
#define FILE_PB         403
#define DRV_TX          404
#define DRV_CB          405
#define OFFS_TX         406
#define OFFS_EF         407
#define CP_TX           408
#define CP_CB           409
#define MORE_PB         410

#define UMNT_DLG        500
#define UMNT_LB         501
#define FORCE_CK        502

#define OPTS_DLG        600
#define OPTS_TX         601
#define AUTO_CK         602
#define CONF_CK         603
#define ERR_CK          604

/* NLS message IDs */

#define ISO_Title                1
#define ISO_Cancel               2
#define ISO_MountFailure         3
#define ISO_UnmountFailure       4
#define ISO_InvalidOffset        5
#define ISO_FileOpen             6
#define ISO_NotIso               7
#define ISO_DaemonNotRun         8
#define ISO_InvalidDrive         9
#define ISO_AlreadyAssigned     10
#define ISO_InvalidParameter    11
#define ISO_InvalidFsdName      12
#define ISO_UnknownError        13
#define ISO_AttachFailed        14
#define ISO_DetachFailed        15
#define ISO_CurrentDrives       16
#define ISO_OptionsButton       17
#define ISO_ConfirmMount        18
#define ISO_MountFile           19
#define ISO_Drive               20
#define ISO_Codepage            21
#define ISO_Offset              22
#define ISO_Mount               23
#define ISO_ConfirmUnmount      24
#define ISO_UnmountDrive        25
#define ISO_UnmountInvalid      26
#define ISO_EmergencyUnmount    27
#define ISO_Unmount             28
#define ISO_Options             29
#define ISO_ISOFSOptions        30
#define ISO_AutoMount           31
#define ISO_Confirm             32
#define ISO_Advise              33
#define ISO_Save                34


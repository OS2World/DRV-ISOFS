/* isodrive.c -- ISOFS GUI interface program.
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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/*********************************************************************
 *
 *  ISODrive is a GUI interface for ISOFS. It builds on mapiso
 *  and adds new features such as starting the daemon on demand.
 *  While it is scriptable, ISODrive is primarily for Desktop use
 *  with access through a program object or through that object's
 *  association with ISO files. It replaces 'automap.cmd' and the
 *  'mapiso' program objects found in previous versions.
 *
 *  ISODrive was written by Rich Walsh, October, 2017
 *
 *********************************************************************/

/* don't use signed character types */
#define OS2EMX_PLAIN_CHAR

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define INCL_ERRORS
#define INCL_DOS
#define INCL_WIN
#define INCL_WINWORKPLACE
#include <os2.h>

#include "getopt.h"
#include "types.h"
#include "ifsdint.h"

#include "isodrive.h"

/*********************************************************************/
/*  Structs / defines                                                */
/*********************************************************************/

/* ISOFS drive info */
typedef struct _DRIVES
{
    ULONG   ndx;        /* drive nbr     */
    char    *str;       /* ISO file name */
} DRIVES;

/* a bundle of things needed by the event thread */
typedef struct _ISOSEM
{
    HEV         term;
    HEV         ev;
    HMUX        mux;
    int         tid;
    HWND        hwnd;
    SEMRECORD   rec[2];
} ISOSEM;

typedef struct _ISONLS
{
  ULONG       msg;
  ULONG       ctl;
} ISONLS;

#define MP  MPARAM
#define UM_REFRESH  (WM_USER+27)

#define MSGFILE_FMT         "ISODRIVE_%s.MSG"
#define MSGFILE_DFLT        "ISODRIVE_EN.MSG"

/*********************************************************************/
/*  Prototypes                                                       */
/*********************************************************************/

void    Morph(void);
BOOL    IsDaemonRunning(void);
void    GetSettings(void);
BOOL    GetMountList(void);
void    GetArgs(int argc, char **argv);
void    DoAutoMountFixup(void);
ULONG   IsIsoMounted(char *iso);
char *  WhichIso(ULONG drv);

ULONG   CmdlineDetach(void);
ULONG   CmdlineAttach(void);
ULONG   DoDetach(void);
ULONG   DoAttach(void);
void    DoFailure(BOOL fAttach, ULONG rc);
BOOL    StartDaemon(void);

ULONG   DoUI(void);

MRESULT EXPENTRY MainDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void    InitMainDlg(HWND hwnd);
BOOL    InitMainList(HWND hwnd);
BOOL    RefreshMainList(HWND hwnd);
void    CenterDialog(HWND hwnd);
void    PositionPopup(HWND hwnd);

MRESULT EXPENTRY MountDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void    InitMountDlg(HWND hwnd);
BOOL    InitDriveList(HWND hwnd);
void    FileDlgCmd(HWND hwnd);
void    ToggleMountOptions(HWND hwnd);
BOOL    MountCmd(HWND hwnd);

MRESULT EXPENTRY UnmountDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void    InitUnmountDlg(HWND hwnd);
void    UnmountCmd(HWND hwnd);

MRESULT EXPENTRY OptsDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void    InitOptsDlg(HWND hwnd);
void    SaveOptions(HWND hwnd);

void    InitNLS(void);
ULONG   GetMsg(ULONG msg, char *buf, ULONG cb);

void    StartSem(HWND hwnd);
void    EndSem(HWND hwnd);
void    NotifyThread(void *pArg);

/*********************************************************************/
/*  Globals !!                                                       */
/*********************************************************************/

/* options */
BOOL        fNoConfirm  = FALSE;
BOOL        fNoErrMsg   = FALSE;
BOOL        fDetach     = FALSE;
BOOL        fForce      = FALSE;
BOOL        fNoAuto     = FALSE;
int         iOffset     = 0;
char        *pszCharSet = 0;

/* internals */
BOOL        fInUI       = FALSE;
LONG        lNLSInit    = 0;
ULONG       freeMap     = 0;
ULONG       nextDrive   = 0;
DRIVES      drives[26];
ISOSEM      is;
char        szDrive[4]  = "\0:\0";
char        szFile[CCHMAXPATH] = "";
char        msgFile[CCHMAXPATH];

/* buffer */
UCHAR       fsqbuf[1024];
FSQBUFFER2  *fsq = (FSQBUFFER2*)fsqbuf;

/* INI strings */
char        szISOFS[]      = IFS_NAME;
char        szNoAutoOpen[] = "NoAutoOpen";
char        szNoErrMsg[]   = "NoErrMsg";
char        szNoConfirm[]  = "NoConfirm";

/* codepages supported */
char   *cpList[] =
{
    "default",
    "cp437", "cp737", "cp775", "cp850",
    "cp852", "cp855", "cp857", "cp860",
    "cp861", "cp862", "cp863", "cp864",
    "cp865", "cp866", "cp869", "cp874",
    "iso8859-1", "iso8859-2", "iso8859-3",
    "iso8859-4", "iso8859-5", "iso8859-6",
    "iso8859-7", "iso8859-8", "iso8859-9",
    "iso8859-14", "iso8859-15",
    "koi8-r", "mac-roman"
};

#define CNT_CPLIST      (sizeof(cpList) / sizeof(char*))

ISONLS  nlsMain[] =
{
    {ISO_CurrentDrives,     MAIN_TX},
    {ISO_OptionsButton,     OPTS_PB},
    {0,                     0}
};

ISONLS  nlsMount[] =
{
    {ISO_MountFile,         FILE_TX},
    {ISO_Drive,             DRV_TX},
    {ISO_Codepage,          CP_TX},
    {ISO_Offset,            OFFS_TX},
    {ISO_Mount,             OK_PB},
    {ISO_Cancel,            CANCEL_PB},
    {0,                     0}
};

ISONLS  nlsUnmount[] =
{
    {ISO_EmergencyUnmount,  FORCE_CK},
    {ISO_Unmount,           OK_PB},
    {ISO_Cancel,            CANCEL_PB},
    {0,                     0}
};

ISONLS  nlsOptions[] =
{
    {ISO_ISOFSOptions,      OPTS_TX},
    {ISO_AutoMount,         AUTO_CK},
    {ISO_Confirm,           CONF_CK},
    {ISO_Advise,            ERR_CK},
    {ISO_Save,              OK_PB},
    {ISO_Cancel,            CANCEL_PB},
    {0,                     0}
};

/*********************************************************************/

/* these are the default NLS message strings to be used
 * if a message file can't be found or if there's an error
 * retrieving a message - note: these are needed because
 * gcc appears not to support having a message file bound
 * to the exe (it crashes during runtime initialization)
 */

char    *dfltMsg[] =
{
    "???",                                              /* filler  */
    "IsoDrive",                                         /* ISO0001 */
    "~Cancel",                                          /* ISO0002 */
    "ISOFS Mount Failure",                              /* ISO0003 */
    "ISOFS Unmount Failure",                            /* ISO0004 */
    "Invalid offset.",                                  /* ISO0005 */
    "Can't open ISO image file.",                       /* ISO0006 */
    "Image file is not in ISO format.",                 /* ISO0007 */
    "The ISOFS daemon is not running.",                 /* ISO0008 */
    "The drive letter is not in use.",                  /* ISO0009 */
    "The drive letter is already in use.",              /* ISO0010 */
    "Invalid parameter.",                               /* ISO0011 */
    "'stubfsd.ifs' is missing (check config.sys).",     /* ISO0012 */
    "Unknown error.",                                   /* ISO0013 */
    "Failed to attach drive %c: - return code= %ld",    /* ISO0014 */
    "Failed to detach drive %c: - return code= %ld",    /* ISO0015 */
    "~Current ISOFS Drives",                            /* ISO0016 */
    "~Options...",                                      /* ISO0017 */
    "Confirm ISOFS Mount",                              /* ISO0018 */
    "Mount ~ISO file",                                  /* ISO0019 */
    "~Drive",                                           /* ISO0020 */
    "Code~page",                                        /* ISO0021 */
    "~Offset",                                          /* ISO0022 */
    "~Mount",                                           /* ISO0023 */
    "Confirm ISOFS Unmount",                            /* ISO0024 */
    "Unmount Drive ?:",                                 /* ISO0025 */
    "<invalid drive>",                                  /* ISO0026 */
    "Emergency unmount",                                /* ISO0027 */
    "~Unmount",                                         /* ISO0028 */
    "Options",                                          /* ISO0029 */
    "ISOFS Options",                                    /* ISO0030 */
    "~AutoMount:  double-click to mount/unmount",       /* ISO0031 */
    "Confirm before ~mount/unmount",                    /* ISO0032 */
    "Advise if mount/unmount ~failed",                  /* ISO0033 */
    "~Save",                                            /* ISO0034 */
};

/*********************************************************************/
/*  Main                                                             */
/*********************************************************************/

int main(int argc, char **argv)
{
    int     ret = 1;
    HAB     hab = 0;
    HMQ     hmq = 0;

do {
    Morph();
    DosError(FERR_DISABLEHARDERR | FERR_ENABLEEXCEPTION);

    if (!(hab = WinInitialize(0)) ||
        !(hmq = WinCreateMsgQueue(hab, 0)))
        break;

    /* get input */
    GetSettings();
    GetArgs(argc, argv);

    /* init list of mounted drives */
    memset(drives, 0, sizeof(drives));
    GetMountList();

    /* if AutoMount is on, try to supply a missing drive letter */
    DoAutoMountFixup();

    /* if there's enough info, attach/detach,
     * otherwise, show the UI
     */
    if (fDetach && *szDrive)
        ret = CmdlineDetach();
    else
    if (!fDetach && *szDrive && *szFile)
        ret = CmdlineAttach();
    else
        ret = DoUI();

} while (0);

    if (hmq)
        WinDestroyMsgQueue(hmq);
    if (hab)
        WinTerminate(hab);

    return ret;
}

/*********************************************************************/
/*  Init                                                             */
/*********************************************************************/

/* enable this program to use PM functions
 * even if it was linked or started as a VIO app
 */

void    Morph(void)
{
  PPIB    ppib;
  PTIB    ptib;

  DosGetInfoBlocks( &ptib, &ppib);
  ppib->pib_ultype = 3;
  return;
}

/*********************************************************************/

/* probe the IFS to see if the daemon is running */

BOOL    IsDaemonRunning(void)
{
    ULONG       rc;
    EASIZEBUF   data;
    ULONG       cbData = sizeof(data);
    ULONG       cbParms = 0;

    /* ask for the max EA size supported */
    rc = DosFSCtl(&data, cbData,  &cbData,
                  0,     cbParms, &cbParms,
                  FSCTL_MAX_EASIZE, szISOFS, -1,
                  FSCTL_FSDNAME);

    fprintf(stderr, "Daemon is %srunning\n", rc ? "not " : "");

    return rc != ERROR_STUBFSD_DAEMON_NOT_RUN;
}

/*********************************************************************/

/* read os2.ini */

void    GetSettings(void)
{
    char    text[8];

    if (PrfQueryProfileString(HINI_USERPROFILE, szISOFS, szNoAutoOpen,
                              "0", text, sizeof(text)) &&
        text[0] == '1')
        fNoAuto = TRUE;

    if (PrfQueryProfileString(HINI_USERPROFILE, szISOFS, szNoErrMsg,
                              "0", text, sizeof(text)) &&
        text[0] == '1')
        fNoErrMsg = TRUE;

    if (PrfQueryProfileString(HINI_USERPROFILE, szISOFS, szNoConfirm,
                              "0", text, sizeof(text)) &&
        text[0] == '1')
        fNoConfirm = TRUE;
}

/*********************************************************************/

/* query every mapped drive to see if it's attached to ISOFS */

BOOL    GetMountList(void)
{
    ULONG   rc;
    ULONG   dummy;
    ULONG   map;
    ULONG   lth;
    ULONG   ctr;
    ULONG   mask;
    char    drv[4] = "A:\0";

    /* clear existing entries */
    for (ctr = 0; ctr < nextDrive; ctr++)
    {
        free(drives[ctr].str);
        drives[ctr].str = 0;
    }

    nextDrive = 0;
    DosQueryCurrentDisk(&dummy, &map);

    /* the map of free drive letters */
    freeMap = (~map) & 0x03ffffff;

    /* no daemon? no mounted drives... */
    if (!IsDaemonRunning())
        return FALSE;

    for (ctr = 2, mask = 1 << 2; mask <= map; ctr++, mask <<= 1)
    {
        if (!(map & mask))
            continue;

        drv[0] = ctr + 'A';
        lth = sizeof(fsqbuf);
        rc = DosQueryFSAttach(drv, 0, FSAIL_QUERYNAME, fsq, &lth);

        /* look for a remote filesystem drive
         * whose filesystem drive name is ISOFS
         */
        if (rc ||
            fsq->iType != FSAT_REMOTEDRV ||
            !fsq->cbFSDName ||
            stricmp(szISOFS, (char*)&(fsq->szFSDName[fsq->cbName])))
            continue;

        /* ISO drive - save the ISO file's path */
        drives[nextDrive].ndx = ctr;
        if (fsq->cbFSAData)
            drives[nextDrive].str = strdup((char*)&(fsq->rgFSAData[fsq->cbName+fsq->cbFSDName]));
        else
            drives[nextDrive].str = strdup("");
        nextDrive++;
    }

    /* did we find anything? */
    return (nextDrive != 0);
}

/*********************************************************************/

void    GetArgs(int argc, char **argv)
{
    int     c;
    char    *ptr;

    struct option const options[] = {
       { "quiet",    no_argument, 0, 'q' },
       { "detach",   no_argument, 0, 'd' },
       { "force",    no_argument, 0, 'f' },
       { "auto",     no_argument, 0, 'a' },
       { "noauto",   no_argument, 0, 'n' },
       { "jcharset", required_argument, 0, 'j' },
       { "offset",   required_argument, 0, 'o' },
       { 0, 0, 0, 0 } 
    };      

    while ((c = getopt_long(argc, argv, "ndfj:o:", options, 0)) != EOF)
    {
        switch (c)
        {
            case 0:
                break;

            case 'q': /* --quiet */
                fNoConfirm = TRUE;
                fNoErrMsg = TRUE;
                break;

            case 'd': /* --delete */
                fDetach = TRUE;
                break;

            case 'f': /* --force */
                fForce = TRUE;
                break;

            case 'a': /* --auto */
                fNoAuto = FALSE;
                break;

            case 'n': /* --noauto */
                fNoAuto = TRUE;
                break;

            case 'j': /* --jcharset */
                pszCharSet = optarg;
                break;

            case 'o': /* --offset */
                iOffset = atoi(optarg);
                break;
        }
    }

    /* identify drive and filename - ignore detritus */
    for (ptr = argv[optind]; optind < argc; ptr = argv[++optind])
    {
        if (!*szDrive && ptr[2] == 0 && ptr[1] == ':' && isalpha((int)ptr[0]))
            *szDrive = ptr[0] & ~0x20;
        else
        if (!*szFile && *ptr)
            DosQueryPathInfo(ptr, FIL_QUERYFULLNAME, szFile, sizeof(szFile));
    }

    return;
}

/*********************************************************************/

/* if automount is on, see if we have a filename but no drive.
 * if the file is already mounted, set up for detachment;
 * otherwise, find the first free drive letter for attachment.
 */

void    DoAutoMountFixup(void)
{
    ULONG   ctr;
    ULONG   mask;

    if (fNoAuto || !*szFile || *szDrive)
        return;

    *szDrive = (char)IsIsoMounted(szFile);
    if (*szDrive) {
        fDetach = TRUE;
        return;
    }

    for (ctr = 2, mask = 1 << 2; mask <= freeMap; ctr++, mask <<= 1)
    {
        if (freeMap & mask) {
            *szDrive = (char)(ctr + 'A');
            break;
        }
    }
    return;
}

/*********************************************************************/

/* in: iso filename - out: drive letter or null */

ULONG   IsIsoMounted(char *iso)
{
    ULONG   ctr;

    for (ctr = 0; ctr < nextDrive; ctr++)
    {
        if (!stricmp(iso, drives[ctr].str))
            return drives[ctr].ndx + 'A';
    }

    return 0;
}

/*********************************************************************/

/* in: drive letter - out: iso filename */

char *  WhichIso(ULONG drv)
{
    ULONG   ctr;

    drv -= 'A';
    for (ctr = 0; ctr < nextDrive; ctr++)
    {
        if (drives[ctr].ndx == drv)
            return drives[ctr].str;
    }

    return 0;
}

/*********************************************************************/
/*  Attach / Detach Operations                                       */
/*********************************************************************/

/* enough info was on the command line to detach;
 * either do it immediately or show the confirmation dialog
 */

ULONG   CmdlineDetach(void)
{
    if (fNoConfirm)
        return DoDetach();

    return WinDlgBox(HWND_DESKTOP, 0, UnmountDlgProc, 0, UMNT_DLG, 0);
}

/*********************************************************************/

/* enough info was on the command line to atttach;
 * either do it immediately or show the confirmation dialog
 */

ULONG   CmdlineAttach(void)
{
    if (fNoConfirm)
        return DoAttach();

    return WinDlgBox(HWND_DESKTOP, 0, MountDlgProc, 0, MNT_DLG, 0);
}

/*********************************************************************/

/* detach a drive */

ULONG   DoDetach(void)
{
    ULONG       rc;
    IFS_DETACH  detachparms;

    memset(&detachparms, 0, sizeof(detachparms));

    if (fForce)
        detachparms.flFlags = DP_FORCE;

    /* detach */
    rc = DosFSAttach(szDrive, IFS_NAME, &detachparms,
                     sizeof(detachparms), FS_DETACH);
    if (rc)
        DoFailure(FALSE, rc);

    return rc;
}

/*********************************************************************/

/* attach a drive */

ULONG   DoAttach(void)
{
    ULONG       rc;
    IFS_ATTACH  attachparms;

    /* start the daemon if it isn't running */
    if (!IsDaemonRunning()) {
        StartDaemon();
        DosSleep(100);
    }

    memset(&attachparms, 0, sizeof(attachparms));
    strcpy (attachparms.szBasePath, szFile);
    attachparms.iOffset = iOffset;
    if (pszCharSet)
        strcpy(attachparms.szCharSet, pszCharSet);

    /* attach */
    rc = DosFSAttach(szDrive, IFS_NAME, &attachparms,
                     sizeof(attachparms), FS_ATTACH);

    /* like 'mapiso', this lets you replace the ISO file
     * attached to a drive without an explicit unmount
     */
    if (rc == ERROR_ALREADY_ASSIGNED)
    {
        IFS_DETACH  detachparms;

        memset(&detachparms, 0, sizeof(detachparms));
        if (fForce)
            detachparms.flFlags = DP_FORCE;
        DosFSAttach(szDrive, IFS_NAME, &detachparms,
                    sizeof(detachparms), FS_DETACH);

        rc = DosFSAttach(szDrive, IFS_NAME, &attachparms,
                         sizeof(attachparms), FS_ATTACH);
    }

    if (rc)
        DoFailure(TRUE, rc);

    return rc;
}

/*********************************************************************/

/* advise that the last attach/detach failed */

void    DoFailure(BOOL fAttach, ULONG rc)
{
    ULONG   error;
    ULONG   lth;
    char    fmt[128];
    char    err[128];
    char    out[256];
    char    title[64];

    if (fNoErrMsg)
        return;

    switch(rc)
    {
        case ERROR_ISOFS_INVALIDOFFSET:
            error = ISO_InvalidOffset;
            break;
        case ERROR_ISOFS_FILEOPEN:
            error = ISO_FileOpen;
            break;
        case ERROR_ISOFS_NOTISO:
            error = ISO_NotIso;
            break;
        case ERROR_STUBFSD_DAEMON_NOT_RUN:
            error = ISO_DaemonNotRun;
            break;
        case ERROR_INVALID_DRIVE:
            error = ISO_InvalidDrive;
            break;
        case ERROR_ALREADY_ASSIGNED:
            error = ISO_AlreadyAssigned;
            break;
        case ERROR_INVALID_PARAMETER:
            error = ISO_InvalidParameter;
            break;
        case ERROR_INVALID_FSD_NAME:
            error = ISO_InvalidFsdName;
            break;
        default:
            error = ISO_UnknownError;
            break;
    }

    GetMsg((fAttach ? ISO_MountFailure : ISO_UnmountFailure),
           title, sizeof(title));

    lth = GetMsg((fAttach ? ISO_AttachFailed : ISO_DetachFailed),
                 fmt, sizeof(fmt));
    strcpy(&fmt[lth], "\n%s");
    GetMsg(error, err, sizeof(err));
    sprintf(out, fmt, szDrive[0], rc, err);

    WinMessageBox(HWND_DESKTOP, 0, out, title,
                  1, MB_CANCEL | MB_ICONEXCLAMATION | MB_MOVEABLE);

    return;
}

/*********************************************************************/

/* detach 'isofsdmn.exe' */

BOOL    StartDaemon(void)
{
    ULONG       rc;
    ULONG       ul;
    BOOL        fDaemon = FALSE;
    PID         pidDummy;
    STARTDATA   sd;
    char        szErr[16];

    memset(&sd, 0, sizeof(sd));
    sd.Length = sizeof(sd);
    sd.FgBg = SSF_FGBG_BACK;
    sd.PgmTitle = (PSZ)"isofsdmn";
    sd.PgmName = (PSZ)"CMD.EXE";
    sd.PgmInputs = (PSZ)"/C DETACH ISOFSDMN.EXE";
    sd.InheritOpt = SSF_INHERTOPT_PARENT;
    sd.PgmControl = SSF_CONTROL_INVISIBLE | SSF_CONTROL_MINIMIZE;
    sd.ObjectBuffer = (PSZ)szErr;
    sd.ObjectBuffLen = sizeof(szErr);

    rc = DosStartSession(&sd, &ul, &pidDummy);
    if (rc) {
        fprintf(stderr, "StartDaemon: DosStartSession - rc= %lu\n", rc);
        return FALSE;
    }

    /* wait up to 3 seconds for the daemon to be ready */
    ul = 12;
    do {
        ul--;
        DosSleep(250);
        fDaemon = IsDaemonRunning();
    } while (ul && !fDaemon);

    fprintf(stderr, "StartDaemon: DosStartSession - rc= %lu fDaemon= %ld\n",
            rc, fDaemon);
    return fDaemon;
}

/*********************************************************************/
/*  UI                                                               */
/*********************************************************************/

/* start the UI as a modal dialog */

ULONG   DoUI(void)
{
    fInUI = TRUE;
    fNoErrMsg = FALSE;

    WinDlgBox(HWND_DESKTOP, 0, MainDlgProc, 0, MAIN_DLG, 0);

    return 0;
}

/*********************************************************************/
/*  Main Dialog                                                      */
/*********************************************************************/

/* main window proc - terminates when dismissed */

MRESULT EXPENTRY MainDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch (msg)
    {
        case WM_INITDLG:
            InitMainDlg(hwnd);
            return 0;

        case WM_CLOSE:
            WinDismissDlg(hwnd, 0);
            return 0;

        case WM_COMMAND:
            switch ((ULONG)mp1)
            {
                /* show Detach dialog */
                case MINUS_PB:
                {
                    LONG sel = (LONG)WinSendDlgItemMsg(hwnd, MAIN_LB,
                                                       LM_QUERYSELECTION,
                                                       (MP)LIT_FIRST, 0);
                    if (sel < 0) {
                        WinAlarm(HWND_DESKTOP, WA_ERROR);
                        break;
                    }

                    /* fill in the drive & file of the current selection */
                    *szDrive = (char)drives[sel].ndx + 'A';
                    strcpy(szFile, drives[sel].str);
                    WinLoadDlg(HWND_DESKTOP, hwnd, UnmountDlgProc, 0, UMNT_DLG, 0);
                    break;
                }

                /* show Attach dialog */
                case PLUS_PB:
                    WinLoadDlg(HWND_DESKTOP, hwnd, MountDlgProc, 0, MNT_DLG, 0);
                    break;

                /* show Options dialog */
                case OPTS_PB:
                    WinLoadDlg(HWND_DESKTOP, hwnd, OptsDlgProc, 0, OPTS_DLG, 0);
                    break;

            }
            return 0;

        /* shutdown mount/unmount events */
        case WM_DESTROY:
            EndSem(hwnd);
            break;

        /* posted by the event thread on mount/unmount events
         * and by the mount/unmount dialogs when they close
         */
        case UM_REFRESH:
            RefreshMainList(hwnd);
            return 0;
    }

    return (WinDefDlgProc(hwnd, msg, mp1, mp2));
}

/*********************************************************************/

/* get it going */

void    InitMainDlg(HWND hwnd)
{
    ISONLS  *nls;
    char    buf[128];

    /* setup to get mount/unmount events */
    StartSem(hwnd);

    /* set window title */
    GetMsg(ISO_Title, buf, sizeof(buf));
    WinSetWindowText(hwnd, buf);

    /* set dialog item strings */
    for (nls = nlsMain; nls->ctl; nls++)
    {
        GetMsg(nls->msg, buf, sizeof(buf));
        WinSetDlgItemText(hwnd, nls->ctl, buf);
    }

    /* setup mounted drives list */
    InitMainList(hwnd);
    WinEnableWindow(WinWindowFromID(hwnd, MINUS_PB), (nextDrive > 0));

    /* display sub-dialog if appropriate */
    if (*szFile && !*szDrive)
        WinPostMsg(hwnd, WM_COMMAND,
                  (MP)(IsIsoMounted(szFile) ? MINUS_PB : PLUS_PB), 0);

    /* show the dialog */
    CenterDialog(hwnd);
}

/*********************************************************************/

/* fill listbox with mounted drive info */

BOOL    InitMainList(HWND hwnd)
{
    ULONG   ctr;
    ULONG   sel;
    HWND    hCtl;
    char    text[512];

    hCtl = WinWindowFromID(hwnd, MAIN_LB);
    if (!hCtl)
        return FALSE;

    WinSendMsg(hCtl, LM_DELETEALL, 0, 0);

    for (ctr = 0, sel = 0; ctr < nextDrive; ctr++)
    {
        sprintf(text, "[ %c: ]    %s",
                (char)(drives[ctr].ndx + 'A'),
                drives[ctr].str);

        WinSendMsg(hCtl, LM_INSERTITEM, (MP)LIT_END, (MP)text);

        if (*szFile && !stricmp(szFile, drives[ctr].str))
            sel = ctr;
    }

    WinSendMsg(hCtl, LM_SELECTITEM, (MP)sel, (MP)TRUE);

    return TRUE;
}

/*********************************************************************/

/* update list of mounted drives, then display it */

BOOL    RefreshMainList(HWND hwnd)
{
    GetMountList();
    InitMainList(hwnd);
    WinEnableWindow(WinWindowFromID(hwnd, MINUS_PB), (nextDrive > 0));
    return TRUE;
}

/*****************************************************************************/

/* center and show the main dialog */

void    CenterDialog(HWND hwnd)
{
    RECTL   rclDlg;
    RECTL   rclOwner;

    WinQueryWindowRect(hwnd, &rclDlg);
    rclOwner.xLeft = 0;
    rclOwner.yBottom = 0;
    rclOwner.xRight = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
    rclOwner.yTop = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);

    WinSetWindowPos(hwnd, 0,
                    rclOwner.xLeft + (rclOwner.xRight - rclDlg.xRight) / 2,
                    rclOwner.yBottom + (rclOwner.yTop - rclDlg.yTop) / 2,
                    0, 0, SWP_MOVE | SWP_ACTIVATE | SWP_SHOW);

    return;
}

/*********************************************************************/

/* put the Mount/Unmount/Options dialogs in the right place */

void    PositionPopup(HWND hwnd)
{
    HWND    hOwner;
    HWND    hMark;
    RECTL   rclMark;
    RECTL   rclDlg;

    /* free-standing confirmation dialog? */
    if (!fInUI) {
        CenterDialog(hwnd);
        return;
    }

    /* dropdown from main dialog */
    hOwner = WinQueryWindow(hwnd, QW_OWNER);
    hMark = WinWindowFromID(hOwner, MARK_TX);
    if (!hOwner || !hMark)
        return;

    /* remove titlebar and sysmenu after caclculating new window size */
    WinQueryWindowRect(hwnd, &rclDlg);
    WinQueryWindowRect(WinWindowFromID(hwnd, FID_TITLEBAR), &rclMark);
    rclDlg.yTop -= rclMark.yTop;

    WinDestroyWindow(WinWindowFromID(hwnd, FID_TITLEBAR));
    WinDestroyWindow(WinWindowFromID(hwnd, FID_SYSMENU));
    WinSendMsg(hwnd, WM_UPDATEFRAME, (MP)(FCF_TITLEBAR | FCF_SYSMENU), 0);

    /* the dialog's top-left aligns with the top-left of MARK_TX */
    WinQueryWindowRect(hMark, &rclMark);
    WinMapWindowPoints(hMark, HWND_DESKTOP, (POINTL*)&rclMark, 2);

    WinSetWindowPos(hwnd, 0, rclMark.xLeft, rclMark.yTop - rclDlg.yTop,
                    rclDlg.xRight, rclDlg.yTop, SWP_SIZE | SWP_MOVE | SWP_SHOW);
}

/*********************************************************************/
/*  Mount Dialog                                                     */
/*********************************************************************/

/* Mount an ISO file
 * used as both a confirmation dialog
 * and as a sub-dialog of the main UI
 */

MRESULT EXPENTRY MountDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch (msg)
    {
        case WM_INITDLG:
            InitMountDlg(hwnd);
            return 0;

        case WM_CLOSE:
            /* started as a confirmation dialog */
            if (!fInUI) {
                WinDismissDlg(hwnd, 0);
                return 0;
            }

            /* started as a sub-dialog */
            *szDrive = 0;
            *szFile = 0;
            WinPostMsg(WinQueryWindow(hwnd, QW_OWNER), UM_REFRESH, 0, 0);
            WinDestroyWindow(hwnd);
            return 0;

        case WM_COMMAND:
            switch ((ULONG)mp1)
            {
                /* show/hide additional mount options */
                case MORE_PB:
                    ToggleMountOptions(hwnd);
                    break;

                /* show file chooser dialog */
                case FILE_PB:
                    FileDlgCmd(hwnd);
                    break;

                /* do mount - exit if no error */
                case OK_PB:
                    if (MountCmd(hwnd))
                        WinSendMsg(hwnd, WM_CLOSE, 0, 0);
                    break;

                /* return to main dialog */
                case CANCEL_PB:
                    WinSendMsg(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            return 0;

        case WM_CHAR:
            if ((SHORT1FROMMP(mp1) & (KC_VIRTUALKEY | KC_KEYUP)) == KC_VIRTUALKEY &&
                SHORT2FROMMP(mp2) == VK_ESC) {
                WinPostMsg(hwnd, WM_CLOSE, 0, 0);
                return (MRESULT)TRUE;
            }
            break;
    }

    return (WinDefDlgProc(hwnd, msg, mp1, mp2));
}

/*********************************************************************/

/* like the name says... init the mount dialog */

void    InitMountDlg(HWND hwnd)
{
    ULONG   ctr;
    HWND    hCtl;
    HWND    hEdit;
    LONG    sel = 0;
    ISONLS  *nls;
    char    buf[128];

    /* set window title */
    GetMsg(ISO_ConfirmMount, buf, sizeof(buf));
    WinSetWindowText(hwnd, buf);

    /* set dialog item strings */
    for (nls = nlsMount; nls->ctl; nls++)
    {
        GetMsg(nls->msg, buf, sizeof(buf));
        WinSetDlgItemText(hwnd, nls->ctl, buf);
    }

    /* list of free drive letters */
    InitDriveList(hwnd);

    /* try to select the correct letter */
    hCtl = WinWindowFromID(hwnd, DRV_CB);
    if (*szDrive) {
        sel = (LONG)WinSendMsg(hCtl, LM_SEARCHSTRING,
                MPFROM2SHORT((LSS_CASESENSITIVE | LSS_PREFIX), LIT_FIRST),
                (MP)&(szDrive));
        if (sel < 0)
            sel = 0;
    }
    WinSendMsg(hCtl, LM_SELECTITEM, (MP)sel, (MP)TRUE);

    /* set the filename if any */
    WinSendDlgItemMsg(hwnd, FILE_EF, EM_SETTEXTLIMIT, (MP)CCHMAXPATH, 0);
    WinSetDlgItemText(hwnd, FILE_EF, szFile);

    /* session offset */
    sprintf(buf, "%d", iOffset);
    WinSetDlgItemText(hwnd, OFFS_EF, buf);

    /* codepage list */
    hCtl = WinWindowFromID(hwnd, CP_CB);
    hEdit = WinWindowFromID(hCtl, CBID_EDIT);
    if (hEdit)
        WinSetWindowBits(hEdit, QWL_STYLE, ES_CENTER, ES_CENTER);

    for (ctr = 0; ctr < CNT_CPLIST; ctr++)
        WinSendMsg(hCtl, LM_INSERTITEM, (MP)LIT_END, (MP)cpList[ctr]);

    WinSetWindowText(hCtl, (pszCharSet ? pszCharSet : cpList[0]));

    /* show window */
    WinSetWindowULong(WinWindowFromID(hwnd, MORE_PB), QWL_USER, 1);
    ToggleMountOptions(hwnd);
    PositionPopup(hwnd);
    return;
}

/*********************************************************************/

/* construct a list of free drive letters */

BOOL    InitDriveList(HWND hwnd)
{
    ULONG   ctr;
    ULONG   mask;
    ULONG   map;
    HWND    hCtl;
    HWND    hEdit;
    char    text[16];

    hCtl = WinWindowFromID(hwnd, DRV_CB);
    if (!hCtl)
        return FALSE;

    WinSendMsg(hCtl, LM_DELETEALL, 0, 0);

    /* center the contents of the entryfield */
    hEdit = WinWindowFromID(hCtl, CBID_EDIT);
    if (hEdit)
        WinSetWindowBits(hEdit, QWL_STYLE, ES_CENTER, ES_CENTER);

    /* get the current map of free drives */
    DosQueryCurrentDisk(&ctr, &map);
    freeMap = (~map) & 0x03ffffff;
    map = freeMap;

    /* add each free drive letter to the list */
    for (ctr = 2, mask = 1 << 2; mask <= map; ctr++, mask <<= 1)
    {
        if (map & mask) {
            sprintf(text, "%c:", (char)(ctr + 'A'));
            WinSendMsg(hCtl, LM_INSERTITEM, (MP)LIT_END, (MP)text);
        }
    }

    return TRUE;
}

/*********************************************************************/

/* show/hide codepage & offset fields */

void    ToggleMountOptions(HWND hwnd)
{
    ULONG   flag;
    HWND    hCtl;

    hCtl = WinWindowFromID(hwnd, MORE_PB);
    if (!hCtl)
        return;

    flag = WinQueryWindowULong(hCtl, QWL_USER);
    flag = !flag;
    WinSetWindowULong(hCtl, QWL_USER, flag);
    WinSetWindowText(hCtl, flag ? ">" : "<");

    WinShowWindow(WinWindowFromID(hwnd, CP_TX), flag);
    WinShowWindow(WinWindowFromID(hwnd, CP_CB), flag);
    WinShowWindow(WinWindowFromID(hwnd, OFFS_TX), flag);
    WinShowWindow(WinWindowFromID(hwnd, OFFS_EF), flag);
}

/*********************************************************************/

/* show the file chooser dialog */

void    FileDlgCmd(HWND hwnd)
{
    FILEDLG fd;

    memset(&fd, 0, sizeof(fd));
    fd.cbSize = sizeof(fd);
    fd.fl = FDS_OPEN_DIALOG | FDS_CENTER;
    strcpy(fd.szFullFile, "*.ISO");

    if (!WinFileDlg(HWND_DESKTOP, hwnd, &fd) ||
        fd.lReturn != DID_OK)
        return;

    strcpy(szFile, fd.szFullFile);
    WinSetDlgItemText(hwnd, FILE_EF, szFile);

    return;
}

/*********************************************************************/

/* GUI attach command */

BOOL    MountCmd(HWND hwnd)
{
    LONG    sel = 0;
    char    text[CCHMAXPATH];

    /* filename */
    if (!WinQueryDlgItemText(hwnd, FILE_EF, sizeof(text), (PSZ)text)) {
        WinAlarm(HWND_DESKTOP, WA_ERROR);
        return FALSE;
    }
    strcpy(szFile, text);

    /* drive */
    if (!WinQueryDlgItemText(hwnd, DRV_CB, sizeof(text), (PSZ)text)) {
        WinAlarm(HWND_DESKTOP, WA_ERROR);
        return FALSE;
    }
    *szDrive = *text;

    /* offset */
    if (WinQueryDlgItemText(hwnd, OFFS_EF, sizeof(text), (PSZ)text))
        iOffset = atoi(text);

    /* codepage */
    sel = (LONG)WinSendDlgItemMsg(hwnd, CP_CB, LM_QUERYSELECTION,
                                  (MP)LIT_FIRST, 0);
    if (sel >= 0)
        pszCharSet = sel ? cpList[sel] : 0;

    /* attach */
    DoAttach();

    return TRUE;
}

/*********************************************************************/
/*  Unmount Dialog                                                   */
/*********************************************************************/

/* Unmount an ISO file
 * used as both a confirmation dialog
 * and as a sub-dialog of the main UI
 */

MRESULT EXPENTRY UnmountDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch (msg)
    {
        case WM_INITDLG:
            InitUnmountDlg(hwnd);
            return 0;

        case WM_CLOSE:
            /* started as a confirmation dialog */
            if (!fInUI) {
                WinDismissDlg(hwnd, 0);
                return 0;
            }

            /* started as a sub-dialog */
            *szDrive = 0;
            *szFile = 0;
            WinPostMsg(WinQueryWindow(hwnd, QW_OWNER), UM_REFRESH, 0, 0);
            WinDestroyWindow(hwnd);
            return 0;

        case WM_COMMAND:
            switch ((ULONG)mp1)
            {
                /* do unmount, exit */
                case OK_PB:
                    UnmountCmd(hwnd);
                    WinSendMsg(hwnd, WM_CLOSE, 0, 0);
                    break;

                /* return to main dialog */
                case CANCEL_PB:
                    WinSendMsg(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            return 0;

        case WM_CHAR:
            if ((SHORT1FROMMP(mp1) & (KC_VIRTUALKEY | KC_KEYUP)) == KC_VIRTUALKEY &&
                SHORT2FROMMP(mp2) == VK_ESC) {
                WinPostMsg(hwnd, WM_CLOSE, 0, 0);
                return (MRESULT)TRUE;
            }
            break;
    }

    return (WinDefDlgProc(hwnd, msg, mp1, mp2));
}

/*********************************************************************/

/* fill in Unmount dialog with confirmation info */

void    InitUnmountDlg(HWND hwnd)
{
    HWND    hCtl;
    LONG    clr = 0;
    char    *ptr;
    ISONLS  *nls;
    char    buf[128];

    /* set window title */
    GetMsg(ISO_ConfirmMount, buf, sizeof(buf));
    WinSetWindowText(hwnd, buf);

    /* set dialog item strings */
    for (nls = nlsUnmount; nls->ctl; nls++)
    {
        GetMsg(nls->msg, buf, sizeof(buf));
        WinSetDlgItemText(hwnd, nls->ctl, buf);
    }

    /* insert the correct drive letter in the confirmation text */
    GetMsg(ISO_UnmountDrive, buf, sizeof(buf));
    ptr = strchr(buf, '?');
    if (ptr)
        *ptr = *szDrive;
    WinSetDlgItemText(hwnd, FILE_TX, buf);

    WinSendDlgItemMsg(hwnd, FORCE_CK, BM_SETCHECK,
                      (MP)(fForce ? 1 : 0), 0);

    /* give this read-only EF a gray background */
    hCtl = WinWindowFromID(hwnd, FILE_EF);
    if (!WinQueryPresParam(hwnd, PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                           0, sizeof(clr), &clr, QPF_ID2COLORINDEX))
        clr = WinQuerySysColor(HWND_DESKTOP, SYSCLR_DIALOGBACKGROUND, 0);
    WinSetPresParam(hCtl, PP_BACKGROUNDCOLOR, sizeof(clr), &clr);
    WinSendMsg(hCtl, EM_SETTEXTLIMIT, (MP)CCHMAXPATH, 0);

    /* get the ISO file if it's missing - this should only
     * be needed when started as a confirmation dialog
     */
    ptr = szFile;
    if (!*szFile) {
        ptr = WhichIso(*szDrive);
        if (!ptr) {
            GetMsg(ISO_UnmountInvalid, buf, sizeof(buf));
            ptr = buf;
            WinEnableWindow(WinWindowFromID(hwnd, OK_PB), FALSE);
        }
    }
    WinSetWindowText(hCtl, ptr);

    PositionPopup(hwnd);
}

/*********************************************************************/

/* GUI detach command */

void    UnmountCmd(HWND hwnd)
{
    LONG    sel;

    sel = (LONG)WinSendDlgItemMsg(hwnd, FORCE_CK, BM_QUERYCHECK, 0, 0);
    fForce = (sel == 1);

    DoDetach();
}

/*********************************************************************/
/*  Options Dialog                                                   */
/*********************************************************************/

/* nuthin' but a few checkboxes here */

MRESULT EXPENTRY OptsDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch (msg)
    {
        case WM_INITDLG:
            InitOptsDlg(hwnd);
            return 0;

        case WM_CLOSE:
            WinDismissDlg(hwnd, 0);
            return 0;

        case WM_COMMAND:
            switch ((ULONG)mp1)
            {
                case OK_PB:
                    SaveOptions(hwnd);
                    WinDismissDlg(hwnd, 0);
                    break;

                case CANCEL_PB:
                    WinDismissDlg(hwnd, 0);
                    break;
            }
            return 0;

        case WM_CHAR:
            if ((SHORT1FROMMP(mp1) & (KC_VIRTUALKEY | KC_KEYUP)) == KC_VIRTUALKEY &&
                SHORT2FROMMP(mp2) == VK_ESC) {
                WinPostMsg(hwnd, WM_CLOSE, 0, 0);
                return (MRESULT)TRUE;
            }
            break;
    }

    return (WinDefDlgProc(hwnd, msg, mp1, mp2));
}

/*********************************************************************/

/* setup options */

void    InitOptsDlg(HWND hwnd)
{
    ISONLS  *nls;
    char    buf[128];

    /* set window title */
    GetMsg(ISO_Options, buf, sizeof(buf));
    WinSetWindowText(hwnd, buf);

    /* set dialog item strings */
    for (nls = nlsOptions; nls->ctl; nls++)
    {
        GetMsg(nls->msg, buf, sizeof(buf));
        WinSetDlgItemText(hwnd, nls->ctl, buf);
    }

    WinCheckButton(hwnd, AUTO_CK, !fNoAuto);
    WinCheckButton(hwnd, CONF_CK, !fNoConfirm);
    WinCheckButton(hwnd, ERR_CK,  !fNoErrMsg);
    PositionPopup(hwnd);
}

/*********************************************************************/

/* store options */

void    SaveOptions(HWND hwnd)
{
    fNoAuto    = !WinQueryButtonCheckstate(hwnd, AUTO_CK);
    fNoConfirm = !WinQueryButtonCheckstate(hwnd, CONF_CK);
    fNoErrMsg  = !WinQueryButtonCheckstate(hwnd, ERR_CK);

    PrfWriteProfileString(HINI_USERPROFILE, szISOFS, szNoAutoOpen,
                          fNoAuto ? "1" : 0);
    PrfWriteProfileString(HINI_USERPROFILE, szISOFS, szNoConfirm,
                          fNoConfirm ? "1" : 0);
    PrfWriteProfileString(HINI_USERPROFILE, szISOFS, szNoErrMsg,
                          fNoErrMsg ? "1" : 0);
}

/*********************************************************************/
/*  NLS Support                                                      */
/*********************************************************************/

/* look for a valid message file;
 * if not found, use the built-in English strings 
 */

void    InitNLS(void)
{
    char        *pEnv;
    char        szLang[4];
    char        szFile[24];

    /* default to using built-in strings */
    lNLSInit = -1;

    /* if there's no LANG environment variable
     * or it looks malformed, use the default
     */
    if (DosScanEnv("LANG", &pEnv) ||
        !pEnv[0] || !pEnv[1] || pEnv[2] != '_') {
      return;
    }
  
    /* extract the language code, then
     * compose the name of the NLS file
     */
    memcpy(szLang, pEnv, 2);
    szLang[2] = 0;
    strupr(szLang);
    sprintf(szFile, MSGFILE_FMT, szLang);

    /* perform the same search that DosGetMessage() does;
     * if the file can be found, set flag to use the msg file
     */
    if (!DosSearchPath(SEARCH_IGNORENETERRS | SEARCH_ENVIRONMENT | SEARCH_CUR_DIRECTORY,
                       "DPATH", szFile, msgFile, sizeof(msgFile)))
        lNLSInit = 1;

    if (lNLSInit == -1)
        fprintf(stderr, "Using built-in message strings\n");
    else
        fprintf(stderr, "Using msg file '%s'\n", msgFile);

    return;
}

/*********************************************************************/

ULONG   GetMsg(ULONG msg, char *buf, ULONG cb)
{
    ULONG   rtn = 0;

    if (!lNLSInit)
        InitNLS();

    if (lNLSInit == 1 &&
        !DosGetMessage(0, 0, buf, cb, msg, msgFile, &rtn) &&
        rtn >= 2) {
        rtn -= 2;
        buf[rtn] = 0;
        return rtn;
    }

    strcpy(buf, dfltMsg[msg]);
    return strlen(buf);
}

/*********************************************************************/
/*  Event Notification                                               */
/*********************************************************************/

/* Create an event sem that 'isofsdmn' will post on mount/unmount
 * events. Create another that we'll post to signal termination.
 * Combine them in a muxwait sem, then start a thread to wait for them.
 */

void    StartSem(HWND hwnd)
{
    memset(&is, 0, sizeof(is));

    if (DosCreateEventSem(0, &is.term, 0, 0) ||
        (DosCreateEventSem(ISOFS_MOUNT_SEM, &is.ev, DC_SEM_SHARED, 0) &&
         DosOpenEventSem(ISOFS_MOUNT_SEM, &is.ev))) {
        return;
    }

    is.rec[0].ulUser  = 0;
    is.rec[0].hsemCur = (HSEM)is.ev;
    is.rec[1].ulUser  = 1;
    is.rec[1].hsemCur = (HSEM)is.term;

    if (DosCreateMuxWaitSem(0, &is.mux, 2, is.rec, DCMW_WAIT_ANY))
        return;

    is.hwnd = hwnd;
    is.tid = _beginthread(NotifyThread, 0, 0x4000, (void*)&is);
}

/*********************************************************************/

/* event semaphore cleanup */

void    EndSem(HWND hwnd)
{
    if (is.term)
        DosPostEventSem(is.term);

    /* if the notify thread is running, it will do cleanup */
    if (is.tid >= 0)
        return;

    /* no notify thread, so we do cleanup */
    if (is.mux) {
        DosCloseMuxWaitSem(is.mux);
        is.mux = 0;
    }
    if (is.ev) {
        DosCloseEventSem(is.ev);
        is.ev = 0;
    }
    if (is.term) {
        DosCloseEventSem(is.term);
        is.term = 0;
    }
}

/*********************************************************************/

/* waits for mount/unmount events posted by 'isofsdmn',
 * then posts a "refresh" message to the main dialog.
 */

void    NotifyThread(void *pArg)
{
    ISOSEM  *pi = (ISOSEM*)pArg;

    // loop until the termination sem is posted or an error occurs
    while (1) {
        ULONG   rc;
        ULONG   whichSem;
        ULONG   postCnt;

        // wait for something to happen
        if (DosWaitMuxWaitSem(pi->mux, (ULONG)-1, &whichSem))
            break;

        // exit the loop if the sem wasn't Isofs
        if (whichSem != 0)
            break;

        rc = DosResetEventSem(pi->ev, &postCnt);
        if (rc && rc != ERROR_ALREADY_RESET)
            break;

        WinPostMsg(is.hwnd, UM_REFRESH, 0, 0);
    }

    /* cleanup */
    if (pi->mux) {
        DosCloseMuxWaitSem(pi->mux);
        pi->mux = 0;
    }
    if (pi->ev) {
        DosCloseEventSem(pi->ev);
        pi->ev = 0;
    }
    if (pi->term) {
        DosCloseEventSem(pi->term);
        pi->term = 0;
    }

    // signal that this thread has terminated    
    pi->tid = -1;

    return;
}

/*********************************************************************/


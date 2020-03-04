/* isofsdmn.c -- Daemon main program.  Gets requests from the ring 0
   IFS and dispatches them to the various worker routines in the other
   C files in this directory.
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
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_DOSMISC
#define INCL_DOSERRORS
#define INCL_DOSEXCEPTIONS
#include <os2.h>

#include "getopt.h"
#include "ifsdmn.h"
#include "sysdep.h"
#include "nls.h"
#include "unls.h"

char    *pszProgramName;
int     loglevel = L_INFO;
BOOL    fOutput = FALSE;

/* Format and write a message to stderr if level <= loglevel. */
void logMsg(int level, char * pszMsg, ...)
{
    time_t now;
    char szTime[128];
    va_list args;

    if (!fOutput || level > loglevel)
        return;

    va_start(args, pszMsg);
    time(&now);
    strftime(szTime, sizeof(szTime),
       "%a, %d %b %Y %H:%M:%S", localtime(&now));
    fprintf(stderr, "[%s] %d: ", szTime, level);
    vfprintf(stderr, pszMsg, args);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(args);
}


static void DetachLostDrives(void)
{
    APIRET rc;
    ULONG len;
    unsigned char szDrive[3] = "?:";
    char buf[sizeof(FSQBUFFER) + 3*CCHMAXPATH];

    for (szDrive[0] = 'C'; szDrive[0] <= 'Z'; szDrive[0]++)
    {
        len = sizeof(buf);
        rc = DosQueryFSAttach(szDrive, 0, FSAIL_QUERYNAME,
                              (FSQBUFFER2 *) buf, &len);
        if (rc == ERROR_STUBFSD_DAEMON_NOT_RUN)
        {
            logMsg (L_INFO, "Detaching drive %s\n", szDrive);
            DosFSAttach(szDrive, (PSZ) IFS_NAME, NULL, 0, FS_DETACH);
        }
    }
}


static void printDaemonStats(ServerData * pServerData)
{
    VolData * pVolData;

    if (!fOutput)
        return;

    fprintf(stderr, "*** INFO ***\n");
    for (pVolData = pServerData->pFirstVolume;
         pVolData;
         pVolData = pVolData->pNext)
    {
        fprintf(stderr, "Drive %c:", pVolData->chDrive);
        fprintf(stderr, " Open Files = %d", pVolData->cOpenFiles);
        fprintf(stderr, " Open Searches = %d", pVolData->cSearches);
        fprintf(stderr, " %s\n", pVolData->fileName);
    }

    fflush(stderr);
}


static void printUsage(int status)
{
    if (!fOutput)
        exit(status);

    if (status)
        fprintf(stderr, "\nTry `%s --help' for more information.\n",
                pszProgramName);
    else
        printf("\
Usage: %s [OPTION]...\n\
Start the %s daemon.\n\
\n\
  -h, --help          Display this help and exit\n\
  -v, --version       Output version information and exit\n\
  -q, --quit          Kill the running daemon\n\
      --logfile=FILE  Log messages to FILE\n\
      --loglevel=N    Set severity level for logging messages\n",
         pszProgramName, IFS_NAME);

    exit(status);
}


/* Set the parameters of the already running daemon */
static int setDaemonParams(ServerData * pServerData, char *command)
{
    APIRET rc;
    IFS_SETPARAMS params;
    ULONG cbParm;

    memset (&params, 0, sizeof(params));   
    strcpy (params.szParams, command);
    cbParm = sizeof(params);
    rc = DosFSCtl(NULL, 0, NULL, &params, sizeof(params), &cbParm,
                  FSCTL_IFS_SETPARAMS, (PSZ)IFS_NAME, (HFILE)-1, FSCTL_FSDNAME);
    if (rc)
    {
        logMsg(L_EVIL, "Error setting daemon parameters, rc=%d", rc);
        return 1;
    }

    return 0;
}


/* Process program arguments. */
int processArgs(ServerData * pServerData, int argc, char **argv, int startup)
{
    PTIB ptib;
    PPIB ppib;
    int  c;

    struct option options[] =
    {
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { "quit", no_argument, 0, 'q' },
        { "info", no_argument, 0, 'i' },
        { "logfile", required_argument, 0, 1 },
        { "loglevel", required_argument, 0, 2 },
        { 0, 0, 0, 0 } 
    };

    /* If the daemon was started from a RUN= statement in config.sys,
     * writing to stdout or stderr will cause it to hang unless they
     * are redirected to a file. Set a flag to signal whether it's safe
     * and/or worthwhile to output any error or informational messages.
     * ('pib_ultype == 4' is a detached process)
     */
    DosGetInfoBlocks(&ptib, &ppib);
    if (ppib->pib_ultype != 4)
        fOutput = TRUE;

    optind = 0;
    while ((c = getopt_long(argc, argv, "hvqi", options, 0)) != EOF)
    {
        switch(c)
        {
            case 0:
                break;

            case 'h': /* --help */
            case '?':
                /* printUsage() doesn't return */
                if (startup)
                    printUsage(0);
                return 1;

            case 'v': /* --version */
                logMsg(0, "isofsdmn - %s", IFS_VERSION);
                return 1;

            case 'q': /* --quit */
                if (pServerData->fRunning)
                {
                    pServerData->fQuit = TRUE;
                    return 0;
                }
                setDaemonParams (pServerData, "-q");
                return 1;

            case 'i': /* --info */
                if (pServerData->fRunning)
                {
                    printDaemonStats(pServerData);
                    return 0;
                }
                setDaemonParams (pServerData, "-i");
                return 1;

            case 1: /* --logfile */
                fclose(stderr);
                if (freopen(optarg, "w", stderr) == 0)
                {
                    logMsg(L_ERR, "Cannot open %s as stderr", optarg);
                    return 1;
                }
                fOutput = TRUE;
                break;

            case 2: /* --loglevel */
                loglevel = atoi(optarg);
                if (loglevel < 1)
                    loglevel = 1;

                break;

            default:
                if (startup)
                    printUsage(1);
                return 1;
        }
    }

    return 0;
}


/* Allocate the buffers that will be used to pass data between the FSD
   and the daemon. */
static int allocExchangeBuffers(ServerData * pServerData)
{
    APIRET rc;

    rc = DosAllocMem((PVOID) &pServerData->pRequest, sizeof(FSREQUEST),
                     PAG_COMMIT | OBJ_TILE | PAG_READ | PAG_WRITE);
    if (rc)
    {
        logMsg(L_FATAL, "Error %d allocating request buffer", rc);
        return 1;
    }

    rc = DosAllocMem((PVOID) &pServerData->pData, sizeof(FSDATA),
                     PAG_COMMIT | OBJ_TILE | PAG_READ | PAG_WRITE);
    if (rc)
    {
        logMsg(L_FATAL, "Error %d allocating data buffer", rc);
        return 1;
    }

    return 0;
}


/* Free the exchange buffers. */
static int freeExchangeBuffers(ServerData * pServerData)
{
    APIRET rc;
    int res = 0;

    rc = DosFreeMem(pServerData->pData);
    if (rc)
    {
        logMsg(L_EVIL, "Error %d freeing data buffer", rc);
        res = 1;
    }

    rc = DosFreeMem(pServerData->pRequest);
    if (rc)
    {
        logMsg(L_EVIL, "Error %d freeing request buffer", rc);
        res = 1;
    }

    return res;
}


/* Announce to the FSD that the daemon is up and running.  The FSD
   will lock the exchange buffers. */
static int announceDaemon(ServerData * pServerData)
{
    APIRET rc;
    SETXCHGBUFFERS buffers;
    ULONG cbParm;
   
    buffers.pRequest = pServerData->pRequest;
    buffers.pData = pServerData->pData;
    cbParm = sizeof(buffers);
    rc = DosFSCtl(NULL, 0, NULL, &buffers, sizeof(buffers), &cbParm,
                  FSCTL_STUBFSD_DAEMON_STARTED, (PSZ)IFS_NAME,
                  (HFILE)-1, FSCTL_FSDNAME);
    switch(rc)
    {
        case 0:
            break;

        case ERROR_STUBFSD_DAEMON_RUNNING:
            if (fOutput)
                fprintf(stderr, "The %s daemon is already running.\n", IFS_NAME);
            return 1;

        default:
            logMsg(L_FATAL, "Error %d announcing daemon to FSD", rc);
            return 1;
    }
   
    return 0;
}


/* Tell the FSD that the daemon is going down.  This unlocks the
   exchange buffers. */
static int detachDaemon(ServerData * pServerData)
{
    APIRET rc;
   
    rc = DosFSCtl(NULL, 0, NULL, NULL, 0, NULL, FSCTL_STUBFSD_DAEMON_STOPPED,
                  (PSZ) IFS_NAME, (HFILE)-1, FSCTL_FSDNAME);
    if (rc)
    {
        logMsg(L_EVIL, "Error %d detaching daemon from FSD", rc);
        return 1;
    }
   
    return 0;
}


/* Initialize the daemon. */
static int initDaemon(ServerData * pServerData)
{
    DosSetPriority(PRTYS_PROCESS, PRTYC_FOREGROUNDSERVER, 16, 0);
    DosSetMaxFH(50);
    DosSetCurrentDir((PSZ) "\\");
    if (!allocExchangeBuffers(pServerData))
    {
        if (!announceDaemon(pServerData))
        {
            if (!init_nls())
                return 0;

            detachDaemon(pServerData);
        }

        freeExchangeBuffers(pServerData);
    }

    return 1;
}


/* Deinitialize the daemon. */
static int doneDaemon(ServerData * pServerData)
{
    detachDaemon(pServerData);
    freeExchangeBuffers(pServerData);
    return 0;
}


/* Block until the FSD places a request in the exchange buffers. */
static int getNextRequest(ServerData * pServerData)
{
    APIRET rc;

    rc = DosFSCtl(NULL, 0, NULL, NULL, 0, NULL, FSCTL_STUBFSD_GET_REQUEST,
                  (PSZ) IFS_NAME, (HFILE)-1, FSCTL_FSDNAME);
    if (rc)
    {
//        if (rc != ERROR_INTERRUPT)
        logMsg(L_FATAL, "Error %d getting next request from FSD", rc);
        return 1;
    }

    return 0;
}


/* Signal to the FSD that the request has completed. */
static int signalRequestDone(ServerData * pServerData)
{
    APIRET rc;

    rc = DosFSCtl(NULL, 0, NULL, NULL, 0, NULL, FSCTL_STUBFSD_DONE_REQUEST,
                  (PSZ)IFS_NAME, (HFILE)-1, FSCTL_FSDNAME);
    if (rc)
    {
        logMsg(L_FATAL, "Error %d signalling request completion to FSD", rc);
        return 1;
    }

    return 0;
}


/* Execute a request from the FSD. */
static void handleRequest(ServerData * pServerData)
{
    APIRET rc;
    PFSREQUEST pRequest = pServerData->pRequest;

    logMsg(L_DBG, "/----- FSD request, code = %d", pRequest->rq);
    switch (pRequest->rq)
    {
        case FSRQ_FSCTL:
            rc = fsFsCtl(pServerData, &pRequest->data.fsctl);
            break;

        case FSRQ_ATTACH:
            rc = fsAttach(pServerData, &pRequest->data.attach);
            break;

        case FSRQ_IOCTL:
            rc = fsIOCtl(pServerData, &pRequest->data.ioctl);
            break;

        case FSRQ_FSINFO:
            rc = fsFsInfo(pServerData, &pRequest->data.fsinfo);
            break;

        case FSRQ_FLUSHBUF:
            rc = fsFlushBuf(pServerData, &pRequest->data.flushbuf);
            break;

        case FSRQ_SHUTDOWN:
            rc = fsShutdown(pServerData, &pRequest->data.shutdown);
            break;

        case FSRQ_OPENCREATE:
            rc = fsOpenCreate(pServerData, &pRequest->data.opencreate);
            break;

        case FSRQ_CLOSE:
            rc = fsClose(pServerData, &pRequest->data.close);
            break;

        case FSRQ_READ:
            rc = fsRead(pServerData, &pRequest->data.read);
            break;

        case FSRQ_WRITE:
            rc = fsWrite(pServerData, &pRequest->data.write);
            break;

        case FSRQ_CHGFILEPTR:
            rc = fsChgFilePtr(pServerData, &pRequest->data.chgfileptr);
            break;

        case FSRQ_NEWSIZE:
            rc = fsNewSize(pServerData, &pRequest->data.newsize);
            break;

        case FSRQ_FILEATTRIBUTE:
            rc = fsFileAttribute(pServerData, &pRequest->data.fileattribute);
            break;

        case FSRQ_FILEINFO:
            rc = fsFileInfo(pServerData, &pRequest->data.fileinfo);
            break;

        case FSRQ_COMMIT:
            rc = fsCommit(pServerData, &pRequest->data.commit);
            break;

        case FSRQ_PATHINFO:
            rc = fsPathInfo(pServerData, &pRequest->data.pathinfo);
            break;

        case FSRQ_DELETE:
            rc = fsDelete(pServerData, &pRequest->data.delete);
            break;

        case FSRQ_MOVE:
            rc = fsMove(pServerData, &pRequest->data.move);
            break;

        case FSRQ_COPY:
            rc = ERROR_CANNOT_COPY;
            break;

        case FSRQ_CHDIR:
            rc = fsChDir(pServerData, &pRequest->data.chdir);
            break;

        case FSRQ_MKDIR:
            rc = fsMkDir(pServerData, &pRequest->data.mkdir);
            break;

        case FSRQ_RMDIR:
            rc = fsRmDir(pServerData, &pRequest->data.rmdir);
            break;

        case FSRQ_FINDFIRST:
            rc = fsFindFirst(pServerData, &pRequest->data.findfirst);
            break;

        case FSRQ_FINDNEXT:
            rc = fsFindNext(pServerData, &pRequest->data.findnext);
            break;

        case FSRQ_FINDFROMNAME:
            rc = fsFindFromName(pServerData, &pRequest->data.findfromname);
            break;

        case FSRQ_FINDCLOSE:
            rc = fsFindClose(pServerData, &pRequest->data.findclose);
            break;

        case FSRQ_PROCESSNAME:
            rc = fsProcessName(pServerData, &pRequest->data.processname);
            break;

        default:
            logMsg(L_EVIL, "Unknown FSD request: %d", pRequest->rq);
            rc = ERROR_NOT_SUPPORTED;
    }

    pRequest->rc = rc;
    logMsg(L_DBG, "\\----- FSD request done, result = %d\n", rc);
}


static int runDaemon(ServerData * pServerData)
{
    int res;

    pServerData->fRunning = TRUE;
    while (!pServerData->fQuit)
    {
        if (getNextRequest(pServerData))
            break;

        handleRequest(pServerData);

        if (signalRequestDone(pServerData))
            break;
    }

    res = !pServerData->fQuit;
    pServerData->fRunning = FALSE;
    DosSleep(100);
    return res;
}


static void DetachDrives(ServerData * pServerData)
{
    VolData *pVolData;
    unsigned char szDrive[3] = "?:";

    for (pVolData = pServerData->pFirstVolume; pVolData; pVolData = pVolData->pNext)
    {
        szDrive[0] = pVolData->chDrive;
        logMsg (L_INFO, "Detaching drive %s\n", szDrive);
        DosFSAttach (szDrive, (PSZ) IFS_NAME, NULL, 0, FS_DETACH);
    }
}


static void cleanup(int sig)
{
    signal (sig, SIG_IGN);
}


int main(int argc, char **argv)
{
    ServerData serverData;
    int res = 0;

    pszProgramName = argv[0];
    memset(&serverData, 0, sizeof(serverData));
    serverData.pFirstVolume = 0;
    signal (SIGINT, cleanup);
    signal (SIGBREAK, cleanup);
    signal (SIGTERM, cleanup);
    DosError(FERR_DISABLEHARDERR | FERR_ENABLEEXCEPTION);
    DetachLostDrives();

    if (processArgs(&serverData, argc, argv, 1))
        return 2;

    if (initDaemon(&serverData))
        return 1;

    if (runDaemon(&serverData))
        res = 1;

    if (doneDaemon(&serverData))
        res = 1;

    DetachDrives(&serverData);
    return res;
}

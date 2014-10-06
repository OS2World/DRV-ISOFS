/* xconfig.h.  Generated automatically by configure.  */
/* @(#)xconfig.h.in	1.8 00/06/03 Copyright 1998 J. Schilling */
/*
 *	Dynamic autoconf C-include code.
 *
 *	Copyright (c) 1998 J. Schilling
 */

/*
 * Header Files
 */
#define PROTOTYPES 1	/* if Compiler supports ANSI C prototypes */
#define HAVE_STDARG_H 1	/* to use stdarg.h, else use varargs.h NOTE: SaberC on a Sun has prototypes but no stdarg.h */
#define HAVE_VARARGS_H 1	/* to use use varargs.h NOTE: The free HP-UX C-compiler has stdarg.h but no PROTOTYPES */
#define HAVE_STDLIB_H 1	/* to use general utility defines (malloc(), size_t ...) and general C library prototypes */
#define HAVE_STRING_H 1	/* to get NULL and ANSI C string function prototypes */
#define HAVE_STRINGS_H 1	/* to get BSD string function prototypes */
#define STDC_HEADERS 1	/* if ANSI compliant stdlib.h, stdarg.h, string.h, float.h are present */
#define HAVE_UNISTD_H 1	/* to get POSIX syscall prototypes XXX sys/file.h fcntl.h (unixstd/fctl)XXX*/
#define HAVE_GETOPT_H 1	/* to get getopt() prototype from getopt.h instead of unistd.h */
#define HAVE_LIMITS_H 1	/* to get POSIX numeric limits constants */
/* #undef HAVE_A_OUT_H */	/* if a.out.h is present (may be a system using a.out format) */
/* #undef HAVE_AOUTHDR_H */	/* if aouthdr.h is present. This is a COFF system */
/* #undef HAVE_ELF_H */	/* if elf.h is present. This is an ELF system */
#define HAVE_FCNTL_H 1	/* to access, O_XXX constants for open(), otherwise use sys/file.h */
/* #undef HAVE_INTTYPES_H */	/* to use UNIX-98 inttypes.h */
#define HAVE_DIRENT_H 1	/* to use POSIX dirent.h */
/* #undef HAVE_SYS_DIR_H */	/* to use BSD sys/dir.h */
/* #undef HAVE_NDIR_H */	/* to use ndir.h */
/* #undef HAVE_SYS_NDIR_H */	/* to use sys/ndir.h */
#define HAVE_MALLOC_H 1	/* if malloc.h exists */
#define HAVE_TERMIOS_H 1	/* to use POSIX termios.h */
#define HAVE_TERMIO_H 1	/* to use SVR4 termio.h */
/* #undef HAVE_SHADOW_H */	/* if shadow.h exists */
/* #undef HAVE_SYSLOG_H */	/* if syslog.h exists */
#define HAVE_SYS_TIME_H 1	/* may include sys/time.h for struct timeval */
/* #undef TIME_WITH_SYS_TIME_H */	/* may include both time.h and sys/time.h */
#define HAVE_UTIMES 1		/* to use BSD utimes() and sys/time.h */
#define HAVE_UTIME_H 1		/* to use utime.h for the utimbuf structure declaration, else declare struct utimbuf yourself */
#define HAVE_SYS_IOCTL_H 1		/* if sys/ioctl.h is present */
#define HAVE_SYS_PARAM_H 1		/* if sys/param.h is present */
/* #undef HAVE_MNTENT_H */		/* if mntent.h is present */
/* #undef HAVE_SYS_MNTENT_H */	/* if sys/mntent.h is present */
/* #undef HAVE_SYS_MNTTAB_H */	/* if sys/mnttab.h is present */
/* #undef HAVE_SYS_MOUNT_H */		/* if sys/mount.h is present */
/* #undef HAVE_WAIT_H */		/* to use wait.h for prototypes and union wait */
#define HAVE_SYS_WAIT_H 1		/* else use sys/wait.h */
#define HAVE_SYS_RESOURCE_H 1	/* to use sys/resource.h for rlimit() and wait3() */
/* #undef HAVE_SYS_PROCFS_H */	/* to use sys/procfs.h for wait3() emulation */
/* #undef HAVE_SYS_SYSTEMINFO_H */	/* to use SVr4 sysinfo() */
#define HAVE_SYS_UTSNAME_H 1	/* to use uname() */
/* #undef HAVE_SYS_PRIOCNTL_H */	/* to use SVr4 priocntl() instead of nice()/setpriority() */
/* #undef HAVE_SYS_RTPRIOCNTL_H */	/* if the system supports SVr4 real time classes */
/* #undef HAVE_SYS_MTIO_H */		/* to use mtio definitions from sys/mtio.h */
/* #undef HAVE_SYS_MMAN_H */		/* to use definitions for mmap()/madvise()... from sys/mman.h */
/* #undef MAJOR_IN_MKDEV */		/* if we should include sys/mkdev.h to get major()/minor()/makedev() */
/* #undef MAJOR_IN_SYSMACROS */	/* if we should include sys/sysmacros.h to get major()/minor()/makedev() */
/* #undef HAVE_SYS_DKIO_H */		/* if we may include sys/dkio.h for disk ioctls */
/* #undef HAVE_SYS_DKLABEL_H */	/* if we may include sys/dklabel.h for disk label */
/* #undef HAVE_SUN_DKIO_H */		/* if we may include sun/dkio.h for disk ioctls */
/* #undef HAVE_SUN_DKLABEL_H */	/* if we may include sun/dklabel.h for disk label */
/* #undef HAVE_POLL_H */		/* if we may include poll.h to use poll() */
/* #undef HAVE_SYS_POLL_H */		/* if we may include sys/poll.h to use poll() */
#define HAVE_SYS_SELECT_H 1	/* if we may have sys/select.h nonstandard use for select() on some systems*/
/* #undef NEED_SYS_SELECT_H */	/* if we need sys/select.h to use select() (this is nonstandard) */
/* #undef HAVE_LINUX_PG_H */		/* if we may include linux/pg.h for PP ATAPI sypport */
/* #undef HAVE_CAMLIB_H */		/* if we may include camlib.h for CAM SCSI transport definitions */
/* #undef HAVE_IEEEFP_H */		/* if we may include ieeefp.h for finite()/isnand() */
/* #undef HAVE_FP_H */		/* if we may include fp.h for FINITE()/IS_INF()/IS_NAN() */
/* #undef HAVE_VALUES_H */		/* if we may include values.h for MAXFLOAT */
#define HAVE_FLOAT_H 1		/* if we may include float.h for FLT_MAX */
/* #undef HAVE_USG_STDIO */		/* if we have USG derived STDIO */
#define HAVE_ERRNO_DEF 1		/* if we have errno definition in <errno.h> */
/* #undef HAVE_VFORK_H */		/* if we should include vfork.h for vfork() definitions */

/*
 * Convert to SCHILY name
 */
#ifdef	STDC_HEADERS
#	ifndef	HAVE_STDC_HEADERS
#		define	HAVE_STDC_HEADERS
#	endif
#endif

#ifdef	HAVE_ELF_H
#define	HAVE_ELF			/* This system uses ELF */
#else
#	ifdef	HAVE_AOUTHDR_H
#	define	HAVE_COFF		/* This system uses COFF */
#	else
#		ifdef HAVE_A_OUT_H
#		define HAVE_AOUT	/* This system uses AOUT */
#		endif
#	endif
#endif

/*
 * Library Functions
 */
#define HAVE_STRERROR 1		/* strerror() is present in libc */
#define HAVE_MEMMOVE 1		/* memmove() is present in libc */
/* #undef HAVE_MLOCKALL */		/* working mlockall() is present in libc */
/* #undef HAVE_MMAP */		/* working mmap() is present in libc */
#define HAVE_FLOCK 1		/* flock() is present in libc */
/* #undef HAVE_FCHDIR */		/* fchdir() is present in libc */
/* #undef HAVE_STATVFS */		/* statvfs() is present in libc */
/* #undef HAVE_QUOTACTL */		/* quotactl() is present in libc */
/* #undef HAVE_QUOTAIOCTL */		/* use ioctl(f, Q_QUOTACTL, &q) instead of quotactl() */
/* #undef HAVE_SETREUID */		/* setreuid() is present in libc */
/* #undef HAVE_SETRESUID */		/* setresuid() is present in libc */
/* #undef HAVE_SETEUID */		/* seteuid() is present in libc */
#define HAVE_SETUID 1		/* setuid() is present in libc */
/* #undef HAVE_SETREGID */		/* setregid() is present in libc */
/* #undef HAVE_SETRESGID */		/* setresgid() is present in libc */
/* #undef HAVE_SETEGID */		/* setegid() is present in libc */
#define HAVE_SETGID 1		/* setgid() is present in libc */
/* #undef HAVE_GETPGID */		/* getpgid() is present in libc (POSIX) */
#define HAVE_SETPGID 1		/* setpgid() is present in libc (POSIX) */
#define HAVE_GETPGRP 1		/* getpgrp() is present in libc (ANY) */
/* #undef HAVE_SETPGRP */		/* setpgrp() is present in libc (ANY) */
/* #undef HAVE_BSD_GETPGRP */		/* getpgrp() in libc is BSD-4.2 compliant */
/* #undef HAVE_BSD_SETPGRP */		/* setpgrp() in libc is BSD-4.2 compliant */
/* #undef HAVE_SYNC */		/* sync() is present in libc */
#define HAVE_FSYNC 1		/* fsync() is present in libc */
/* #undef HAVE_WAIT3 */		/* working wait3() is present in libc */
/* #undef HAVE_WAIT4 */		/* wait4() is present in libc */
/* #undef HAVE_WAITID */		/* waitid() is present in libc */
#define HAVE_WAITPID 1		/* waitpid() is present in libc */
/* #undef HAVE_GETHOSTID */		/* gethostid() is present in libc */
/* #undef HAVE_GETHOSTNAME */		/* gethostname() is present in libc */
/* #undef HAVE_GETDOMAINNAME */	/* getdomainname() is present in libc */
#define HAVE_GETPAGESIZE 1		/* getpagesize() is present in libc */
/* #undef HAVE_GETRUSAGE */		/* getrusage() is present in libc */
/* #undef HAVE_POLL */		/* poll() is present in libc */
#define HAVE_SELECT 1		/* select() is present in libc */
/* #undef HAVE_LCHOWN */		/* lchown() is present in libc */
#define HAVE_BRK 1			/* brk() is present in libc */
#define HAVE_SBRK 1		/* sbrk() is present in libc */
/* #undef HAVE_VA_COPY */		/* va_copy() is present in varargs.h/stdarg.h */
/* #undef HAVE__VA_COPY */		/* __va_copy() is present in varargs.h/stdarg.h */
/* #undef HAVE_DTOA */		/* BSD-4.4 __dtoa() is present in libc */
#define HAVE_GETCWD 1		/* POSIX getcwd() is present in libc */
/* #undef HAVE_SMMAP */		/* may map anonymous memory to get shared mem */
/* #undef HAVE_SHMAT */		/* shmat() is present in libc */
/* #undef HAVE_SEMGET */		/* semget() is present in libc */
/* #undef HAVE_LSTAT */		/* lstat() is present in libc */
/* #undef HAVE_READLINK */		/* readlink() is present in libc */
/* #undef HAVE_LINK */		/* link() is present in libc */
#define HAVE_RENAME 1		/* rename() is present in libc */
/* #undef HAVE_MKFIFO */		/* mkfifo() is present in libc */
/* #undef HAVE_MKNOD */		/* mknod() is present in libc */
/* #undef HAVE_ECVT */		/* ecvt() is present in libc */
/* #undef HAVE_FCVT */		/* fcvt() is present in libc */
#define HAVE_GCVT 1		/* gcvt() is present in libc */
/* #undef HAVE_ECVT_R */		/* ecvt_r() is present in libc */
/* #undef HAVE_FCVT_R */		/* fcvt_r() is present in libc */
/* #undef HAVE_GCVT_R */		/* gcvt_r() is present in libc */
/* #undef HAVE_ECONVERT */		/* econvert() is present in libc */
/* #undef HAVE_FCONVERT */		/* fconvert() is present in libc */
/* #undef HAVE_GCONVERT */		/* gconvert() is present in libc */
/* #undef HAVE_ISINF */		/* isinf() is present in libc */
/* #undef HAVE_ISNAN */		/* isnan() is present in libc */
#define HAVE_RAND 1		/* rand() is present in libc */
/* #undef HAVE_DRAND48 */		/* drand48() is present in libc */
/* #undef HAVE_SETPRIORITY */		/* setpriority() is present in libc */
/* #undef HAVE_NICE */		/* nice() is present in libc */
#define HAVE_DOSSETPRIORITY 1	/* DosSetPriority() is present in libc */
#define HAVE_DOSALLOCSHAREDMEM 1	/* DosAllocSharedMem() is present in libc */
#define HAVE_SEEKDIR 1		/* seekdir() is present in libc */
#define HAVE_PUTENV 1		/* putenv() is present in libc (preferred function) */
/* #undef HAVE_SETENV */		/* setenv() is present in libc (use instead of putenv()) */
#define HAVE_UNAME 1		/* uname() is present in libc */
#define HAVE_SNPRINTF 1		/* snprintf() is present in libc */
/* #undef HAVE_STRCASECMP */		/* strcasecmp() is present in libc */
/* #undef HAVE_STRSIGNAL */		/* strsignal() is present in libc */
/* #undef HAVE_STR2SIG */		/* str2sig() is present in libc */
/* #undef HAVE_SIG2STR */		/* sig2str() is present in libc */
/* #undef HAVE_KILLPG */		/* killpg() is present in libc */
/* #undef HAVE_SIGRELSE */		/* sigrelse() is present in libc */
/* #undef HAVE_SYS_SIGLIST */		/* char *sys_siglist[] is present in libc */
/* #undef HAVE_NANOSLEEP */		/* nanosleep() is present in libc */
/* #undef HAVE_USLEEP */		/* usleep() is present in libc */
#define HAVE_FORK 1		/* fork() is present in libc */
#define vfork fork

#if	defined(HAVE_QUOTACTL) || defined(HAVE_QUOTAIOCTL)
#	define HAVE_QUOTA	/* The system inludes quota */
#endif

#ifdef HAVE_SHMAT
#	define	HAVE_USGSHM	/* USG shared memory is present */
#endif
#ifdef HAVE_SEMGET
#	define	HAVE_USGSEM	/* USG semaphores are present */
#endif

#if	defined(HAVE_GETPGRP) && !defined(HAVE_BSD_GETPGRP)
#define	HAVE_POSIX_GETPGRP 1	/* getpgrp() in libc is POSIX compliant */
#endif
#if	defined(HAVE_SETPGRP) && !defined(HAVE_BSD_SETPGRP)
#define	HAVE_POSIX_SETPGRP 1	/* setpgrp() in libc is POSIX compliant */
#endif

/*
 * Structures
 */
/* #undef HAVE_MTGET_DSREG */		/* if struct mtget contains mt_dsreg (drive status) */
/* #undef HAVE_MTGET_RESID */		/* if struct mtget contains mt_resid (residual count) */
/* #undef HAVE_MTGET_FILENO */	/* if struct mtget contains mt_fileno (file #) */
/* #undef HAVE_MTGET_BLKNO */		/* if struct mtget contains mt_blkno (block #0) */
/* #undef HAVE_STRUCT_RUSAGE */	/* have struct rusage in sys/resource.h */
/* #undef HAVE_UNION_SEMUN */		/* have an illegal definition for union semun in sys/sem.h */
/* #undef HAVE_UNION_WAIT */		/* have union wait in wait.h */
/* #undef HAVE_ST_SPARE1 */		/* if struct stat contains st_spare1 (usecs) */
/* #undef HAVE_ST_NSEC */		/* if struct stat contains st_atim.st_nsec (nanosecs */
/* #undef HAVE_ST_BLKSIZE */		/* if struct stat contains st_blksize */
/* #undef HAVE_ST_BLOCKS */		/* if struct stat contains st_blocks */
#define HAVE_ST_RDEV 1		/* if struct stat contains st_rdev */
/* #undef STAT_MACROS_BROKEN */	/* if the macros S_ISDIR, S_ISREG .. don't work */

#define DEV_MINOR_BITS 8		/* # if bits needed to hold minor device number */
/* #undef DEV_MINOR_NONCONTIG */	/* if bits in minor device number are noncontiguous */


/*
 * Byteorder/Bitorder
 */
#define	HAVE_C_BIGENDIAN	/* Flag that WORDS_BIGENDIAN test was done */
/* #undef WORDS_BIGENDIAN */		/* If using network byte order             */
#define	HAVE_C_BITFIELDS	/* Flag that BITFIELDS_HTOL test was done  */
/* #undef BITFIELDS_HTOL */		/* If high bits come first in structures   */

/*
 * Types/Keywords
 */
#define SIZEOF_CHAR 1
#define SIZEOF_SHORT_INT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG_INT 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_CHAR_P 4
#define SIZEOF_UNSIGNED_CHAR 1
#define SIZEOF_UNSIGNED_SHORT_INT 2
#define SIZEOF_UNSIGNED_INT 4
#define SIZEOF_UNSIGNED_LONG_INT 4
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_UNSIGNED_CHAR_P 4

#define HAVE_LONGLONG 1		/* Compiler defines long long type */
/* #undef CHAR_IS_UNSIGNED */		/* Compiler defines char to be unsigned */

/* #undef const */			/* Define to empty if const doesn't work */
/* #undef uid_t */			/* To be used if uid_t is not present  */
/* #undef size_t */			/* To be used if size_t is not present */
/* #undef pid_t */			/* To be used if pid_t is not present  */
/* #undef off_t */			/* To be used if off_t is not present  */
/* #undef mode_t */			/* To be used if mode_t is not present */

/*#undef HAVE_SIZE_T*/
/*#undef NO_SIZE_T*/
/* #undef VA_LIST_IS_ARRAY */		/* va_list is an array */
#define GETGROUPS_T gid_t
#define GID_T		GETGROUPS_T

/*
 * Define as the return type of signal handlers (int or void).
 */
#define RETSIGTYPE void

/*
 * Misc CC / LD related stuff
 */
/* #undef NO_USER_MALLOC */		/* If we cannot define our own malloc()	*/

/*
 * Strings that help to maintain OS/platform id's in C-programs
 */
#define HOST_ALIAS "i386-pc-os2_emx"		/* Output from config.guess (orig)	*/
#define HOST_SUB "i386-pc-os2_emx"			/* Output from config.sub (modified)	*/
#define HOST_CPU "i386"			/* CPU part from HOST_SUB		*/
#define HOST_VENDOR "pc"		/* VENDOR part from HOST_SUB		*/
#define HOST_OS "os2_emx"			/* CPU part from HOST_SUB		*/

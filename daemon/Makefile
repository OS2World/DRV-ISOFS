BASE = ..
include $(BASE)\Makefile.inc

SRCS = isofsdmn.c isoacces.c attach.c find.c misc.c dir.c openclos.c \
 fileio.c fileinfo.c fileop.c ea.c utils.c

SRCS2 = mapiso.c

SRCS3 = isoinfo.c

LIB =  $(BASE)\misc\getopt.a $(BASE)\system\$(SYSTEM)\sysdep.a \
 $(BASE)\libunls\unls.a

all: isofsdmn.exe mapiso.exe isoinfo.exe

isofsdmn.exe: $(SRCS:.c=.o) $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS:.c=.o) $(LIB) -o $@

mapiso.exe: $(SRCS2:.c=.o) $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS2:.c=.o) $(LIB) -o $@

isoinfo.exe: $(SRCS3:.c=.o) $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS3:.c=.o) $(LIB) -o $@

clean:
	$(RM) isofsdmn.exe
	$(RM) mapiso.exe
	$(RM) isoinfo.exe

ifneq ($(MAKECMDGOALS),clean)
include $(SRCS:.c=.d)
include $(SRCS2:.c=.d)
endif

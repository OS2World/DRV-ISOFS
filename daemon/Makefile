BASE = ..
include $(BASE)\Makefile.inc

OBJS1 = isofsdmn.o isoacces.o attach.o find.o misc.o dir.o openclos.o \
 fileio.o fileinfo.o fileop.o ea.o utils.o

OBJS2 = mapiso.o

OBJS3 = isoinfo.o

OBJS4 = isodrive.o

LIB = $(BASE)\system\$(SYSTEM)\sysdep.a $(BASE)\libunls\unls.a

all: isofsdmn.exe mapiso.exe isoinfo.exe isodrive.exe

isofsdmn.exe: $(OBJS1) $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS1) $(LIB) -o $@

mapiso.exe: $(OBJS2) $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS2) $(LIB) -o $@

isoinfo.exe: $(OBJS3) $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS3) $(LIB) -o $@

isodrive.exe: $(OBJS4) $(LIB) isodrive.res nls
	$(CC) $(CFLAGS) $(LDFLAGS)  -Zlinker /PMTYPE:PM $(OBJS4) $(LIB) -o $@
	$(RC) -x2 -n isodrive.res $@

nls: $(patsubst %.nls,%.msg,$(wildcard *.nls)) isodrive.h

clean: clean-default
	$(RM) *.exe *.map *.res *.msg

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS1:.o=.d)
-include $(OBJS2:.o=.d)
-include $(OBJS3:.o=.d)
-include $(OBJS4:.o=.d)
endif

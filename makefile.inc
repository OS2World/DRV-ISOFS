DISTNAME = isofs103
WARPINNAME = $(DISTNAME).wpi
SYSTEM = os2

INCL = -I$(BASE)\misc -I$(BASE)\include -I$(BASE)\driver \
 -I$(BASE)\system -I$(BASE)\system\$(SYSTEM) -I$(BASE)\daemon
OPTFLAGS = -O2 -fomit-frame-pointer
CFLAGS = -Wall -Wno-parentheses -std=gnu99 -pedantic -DSYSTEM=$(SYSTEM) \
 $(INCL) $(OPTFLAGS)
LDFLAGS = -s -static -Zomf -lc_omf386

DDK = f:\ddk\base
CC16 = $(DDK)\tools\cl
C16FLAGS = /c /Zp /G2s /Anlw /Onir
C16INC = -I$(DDK)\h
LINK16 = $(DDK)\tools\link
L16FLAGS = /map /nod /exepack /packd /a:16 /far
L16LIBS = $(DDK)\lib\dhcalls.lib fshelper.lib

CP = copy
RM = -del
MD = -md

%.d: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

default: all

clean-default:
	$(RM) *.d *.o *.a *.exe

clean: clean-default

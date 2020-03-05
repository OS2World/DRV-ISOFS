DISTNAME = isofs112
WARPINNAME = $(DISTNAME).wpi
SYSTEM = os2

INCL = -I$(BASE)\include -I$(BASE)\driver \
 -I$(BASE)\system -I$(BASE)\system\$(SYSTEM) -I$(BASE)\daemon
OPTFLAGS = -O2 -fomit-frame-pointer
CFLAGS = -Wall -Wno-parentheses -std=gnu99 -pedantic -DSYSTEM=$(SYSTEM) -Dlint \
 $(INCL) $(OPTFLAGS)

LD = gcc
LDFLAGS = -s -static -Zomf -Zmap -Zno-fork -llibc_omf386

RC = rc
RCFLAGS = -r -n

DDK = D:\tk\ddk\base
CC16 = $(DDK)\tools\cl
C16FLAGS = /c /Zp /G2s /Anlw /Onir
C16INC = -I$(DDK)\h
LINK16 = $(DDK)\tools\link
L16FLAGS = /map /nod /exepack /packd /a:16 /far
L16LIBS = $(DDK)\lib\dhcalls.lib $(DDK)\lib\os2286.lib fshelper.lib

LXLITE = H:\UT\lxlite\lxLite.exe

SHELL = cmd
CP = copy
RM = -del
MD = -md

%.d: %.c
	$(CC) $(CFLAGS) -MM $< >$@

%.res: %.rc
	$(RC) $(RCFLAGS) $<

%.msg: %.nls
	mkmsgf $< $@

default: all

clean-default:
	$(RM) *.d *.o

BASE = ..
include $(BASE)\Makefile.inc

all: stubfsd.ifs

stubfsd.ifs: stubfsd.obj stubfsd.def 
	$(LINK16) $(L16FLAGS) stubfsd.obj,$@,stubfsd.map,$(L16LIBS),stubfsd.def

stubfsd.obj: stubfsd.c fsd.h stubfsd.h
	$(CC16) $(C16FLAGS) $(C16INC) $<

clean:
	$(RM) *.obj *.ifs *.map

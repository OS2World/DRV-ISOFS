BASE = ..\..
include $(BASE)\Makefile.inc

OBJS = os2.o

all: sysdep.a 

sysdep.a: $(OBJS)
	ar crs $@ $?

clean: clean-default
	$(RM) *.a

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif

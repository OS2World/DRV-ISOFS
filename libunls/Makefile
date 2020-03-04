BASE = ..
include $(BASE)\Makefile.inc

OBJS =	nls_base.o nls_cp437.o nls_cp737.o nls_cp775.o nls_cp850.o \
	nls_cp852.o nls_cp855.o nls_cp857.o nls_cp860.o nls_cp861.o \
	nls_cp862.o nls_cp863.o nls_cp864.o nls_cp865.o nls_cp866.o \
	nls_cp869.o nls_cp874.o nls_iso8859-1.o nls_iso8859-14.o \
	nls_iso8859-15.o nls_iso8859-2.o nls_iso8859-3.o nls_iso8859-4.o \
	nls_iso8859-5.o nls_iso8859-6.o nls_iso8859-7.o nls_iso8859-8.o \
	nls_iso8859-9.o nls_koi8-r.o nls_mac-roman.o

all:	unls.a 

unls.a:	$(OBJS)
	ar crs unls.a $?

clean: clean-default
	$(RM) *.a

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif

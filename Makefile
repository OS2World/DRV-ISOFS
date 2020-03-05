BASE = .
include $(BASE)\Makefile.inc

all: docs
	cd system\$(SYSTEM) && $(MAKE) -w $@ && cd ..\..
	cd libunls && $(MAKE) -w $@ && cd ..
	cd daemon && $(MAKE) -w $@ && cd ..
	cd driver && $(MAKE) -w $@ && cd ..

clean:
	$(RM) isofs_*.inf
	cd system\$(SYSTEM) && $(MAKE) -w $@ && cd ..\..
	cd libunls && $(MAKE) -w $@ && cd ..
	cd daemon && $(MAKE) -w $@ && cd ..
	cd driver && $(MAKE) -w $@ && cd ..
	$(RM) ..\$(DISTNAME).zip
	$(RM) ..\$(WARPINNAME)

docs: $(patsubst readme_%.src,isofs_%.inf,$(wildcard readme_??.src))

isofs_%.inf: readme_%.src
	emxdoc -I -o $*.ipf $<
	ipfc $*.ipf $@
	$(RM) $*.ipf

dist: Makefile
	make all
	$(MD) ..\bin
	$(CP) daemon\*.exe ..\bin
	$(CP) daemon\*.msg ..\bin
	$(CP) driver\stubfsd.ifs ..\bin
	$(CP) isofs*.inf ..\bin
	make clean
	cd ..\bin && lxlite *.exe && cd ..\source
	makewpi.cmd ..
	zip -9Xoj ..\$(DISTNAME) ..\$(WARPINNAME) file_id.diz

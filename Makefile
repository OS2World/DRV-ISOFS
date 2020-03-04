BASE = .
include $(BASE)\Makefile.inc

all: docs
	cd system\$(SYSTEM) && $(MAKE) -w $@ && cd ..\..
	cd libunls && $(MAKE) -w $@ && cd ..
	cd daemon && $(MAKE) -w $@ && cd ..
	cd driver && $(MAKE) -w $@ && cd ..

clean:
	$(RM) isofs.txt
	$(RM) isofs.inf
	cd system\$(SYSTEM) && $(MAKE) -w $@ && cd ..\..
	cd libunls && $(MAKE) -w $@ && cd ..
	cd daemon && $(MAKE) -w $@ && cd ..
	cd driver && $(MAKE) -w $@ && cd ..
	$(RM) ..\$(DISTNAME).zip
	$(RM) ..\$(WARPINNAME)

docs: isofs.txt isofs.inf

isofs.txt: readme.src
	emxdoc -T -o $@ $<

isofs.inf: readme.src
	emxdoc -I -o isofs.ipf $<
	ipfc isofs.ipf $@
	$(RM) isofs.ipf

dist: Makefile
	make all
	$(MD) ..\bin
	$(CP) daemon\*.exe ..\bin
	$(CP) daemon\*.ico ..\bin
	$(CP) daemon\*.msg ..\bin
	$(CP) driver\stubfsd.ifs ..\bin
	$(CP) isofs.inf ..\bin
	$(CP) isofs.txt ..\bin
	make clean
	cd ..\bin && $(LXLITE) *.exe && cd ..\source
	makewpi.cmd ..
	zip -9Xoj ..\$(DISTNAME) ..\$(WARPINNAME) file_id.diz ..\bin\isofs.txt

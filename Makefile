BASE = .
include $(BASE)\Makefile.inc

all: docs
	cd misc && $(MAKE) -w $@ && cd ..
	cd system\$(SYSTEM) && $(MAKE) -w $@ && cd ..\..
	cd libunls && $(MAKE) -w $@ && cd ..
	cd daemon && $(MAKE) -w $@ && cd ..
	cd driver && $(MAKE) -w $@ && cd ..

clean:
	$(RM) readme.txt
	$(RM) readme.htm
	$(RM) readme.inf
	cd misc && $(MAKE) -w $@ && cd ..
	cd system\$(SYSTEM) && $(MAKE) -w $@ && cd ..\..
	cd libunls && $(MAKE) -w $@ && cd ..
	cd daemon && $(MAKE) -w $@ && cd ..
	cd driver && $(MAKE) -w $@ && cd ..
	$(RM) ..\$(DISTNAME).zip
	$(RM) ..\$(WARPINNAME)

docs: readme.txt readme.inf readme.htm

readme.txt: readme.src
	emxdoc -T -o $@ $<

readme.htm: readme.src
	emxdoc -H -o $@ $<

readme.inf: readme.src
	emxdoc -I -o readme.ipf $<
	ipfc readme.ipf $@
	$(RM) readme.ipf

dist: Makefile
	make all
	$(MD) ..\bin
	$(CP) daemon\*.exe ..\bin
	$(CP) driver\stubfsd.ifs ..\bin
	$(CP) readme.inf ..\bin
	$(CP) automap.* ..\bin
	make clean
	cd ..\bin && lxlite *.exe && cd ..\source
	makewpi.cmd ..
	zip -9oj ..\$(DISTNAME) ..\$(WARPINNAME) file_id.diz

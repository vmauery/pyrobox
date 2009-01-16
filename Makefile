SHELL=/bin/sh
MAKE = make
SUBDIRS=fcgi

all: config.fcgi

subdirs:
	@for i in $(SUBDIRS); do \
	(cd $$i; $(MAKE) $(MFLAGS) $$@ ); done

config.fcgi: subdirs
	cp -a fcgi/config config.fcgi.tmp
	rm -f config.fcgi
	mv config.fcgi.tmp config.fcgi

clean:
	@for i in $(SUBDIRS); do \
	(cd $$i; $(MAKE) $(MFLAGS) clean ); done
	rm -f config.fcgi

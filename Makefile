VERSION=6.1
PREFIX ?= /usr/
DESTDIR ?= 
BINPREFIX = $(PREFIX)

BINDIR = ${BINPREFIX}bin/
INSTALLDIR = ${PREFIX}share/games/atanks

export VERSION
export PREFIX
export INSTALLDIR

FILENAME=atanks-${VERSION}
INSTALL=$(PREFIX)bin/install -c
DISTCOMMON=atanks/*.dat atanks/COPYING atanks/README atanks/TODO atanks/Changelog atanks/BUGS atanks/*.txt
INCOMMON=COPYING README TODO Changelog *.txt unicode.dat

all:
	FLAGS=-DLINUX $(MAKE) -C src

install: 
	mkdir -p $(DESTDIR)${BINDIR}
	$(INSTALL) -m 755 atanks $(DESTDIR)${BINDIR}
	mkdir -p $(DESTDIR)/usr/share/applications
	$(INSTALL) -m 644 atanks.desktop $(DESTDIR)/usr/share/applications
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/48x48/apps
	$(INSTALL) -m 644 atanks.png $(DESTDIR)/usr/share/icons/hicolor/48x48/apps
	mkdir -p $(DESTDIR)${INSTALLDIR}
	mkdir -p $(DESTDIR)${INSTALLDIR}/button
	mkdir -p $(DESTDIR)${INSTALLDIR}/misc
	mkdir -p $(DESTDIR)${INSTALLDIR}/missile
	mkdir -p $(DESTDIR)${INSTALLDIR}/sound
	mkdir -p $(DESTDIR)${INSTALLDIR}/stock
	mkdir -p $(DESTDIR)${INSTALLDIR}/tank
	mkdir -p $(DESTDIR)${INSTALLDIR}/tankgun
	mkdir -p $(DESTDIR)${INSTALLDIR}/title
	mkdir -p $(DESTDIR)${INSTALLDIR}/text
	$(INSTALL) -m 644 $(INCOMMON) $(DESTDIR)${INSTALLDIR}
	$(INSTALL) -m 644 button/* $(DESTDIR)${INSTALLDIR}/button
	$(INSTALL) -m 644 misc/* $(DESTDIR)${INSTALLDIR}/misc
	$(INSTALL) -m 644 missile/* $(DESTDIR)${INSTALLDIR}/missile
	$(INSTALL) -m 644 sound/* $(DESTDIR)${INSTALLDIR}/sound
	$(INSTALL) -m 644 stock/* $(DESTDIR)${INSTALLDIR}/stock
	$(INSTALL) -m 644 tank/* $(DESTDIR)${INSTALLDIR}/tank
	$(INSTALL) -m 644 tankgun/* $(DESTDIR)${INSTALLDIR}/tankgun
	$(INSTALL) -m 644 title/* $(DESTDIR)${INSTALLDIR}/title
	$(INSTALL) -m 644 text/* $(DESTDIR)${INSTALLDIR}/text

user:
	INSTALLDIR=./ FLAGS=-DLINUX $(MAKE) -C src

winuser:
	INSTALLDIR=./ FLAGS=-DWIN32 $(MAKE) -C src -f Makefile.windows

osxuser:
	INSTALLDIR=./ FLAGS="-DMACOSX" $(MAKE) -C src -f Makefile.bsd

ubuntu:
	FLAGS="-DLINUX -DUBUNTU" $(MAKE) -C src

clean:
	rm -f atanks atanks.exe
	$(MAKE) -C src clean

dist: source-dist i686-dist win32-dist

tarball: clean
	cd .. && tar czf atanks-$(VERSION).tar.gz atanks-$(VERSION)

zipfile: clean
	cd .. && zip -r atanks-$(VERSION)-source.zip atanks-$(VERSION)

source-dist:
	cd ../; \
	rm -f ${FILENAME}.tar.gz; \
	tar cvf ${FILENAME}.tar atanks/src/*.cpp atanks/src/*.h atanks/src/Makefile atanks/src/Makefile.windows atanks/Makefile ${DISTCOMMON}; \
	gzip ${FILENAME}.tar

i686-dist:
	cd ../; \
	rm -f ${FILENAME}-i686-dist.tar; \
	rm -f ${FILENAME}-i686-dist.tar.gz; \
	strip atanks/atanks; \
	tar cvf ${FILENAME}-i686-dist.tar atanks/atanks ${DISTCOMMON}; \
	gzip ${FILENAME}-i686-dist.tar;

win32-dist:
	cd ../; \
	rm -f ${FILENAME}-win32-dist.zip; \
	zip ${FILENAME}-win32-dist.zip atanks/Atanks.exe atanks/alleg40.dll ${DISTCOMMON};

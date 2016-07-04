.PHONY: all install clean veryclean user winuser osxuser ubuntu \
dist tarball zipfile source-dist i686-dist win32-dist

VERSION := 6.5


DEBUG   ?= NO
# Note: Submit as "YES" to enable debugging
# Note: If any flag starting with -g is found in the CXXFLAGS, DEBUG is
#       switched to YES no matter whether set otherwise or not.

# The following switches can be used to fine-tune the debugging output:
# Note: DEBUG_AICORE can be used to enable both DEBUG_AIMING and DEBUG_EMOTION
#       together with a single flag.
DEBUG_AICORE  ?= NO
DEBUG_AIMING  ?= NO
DEBUG_EMOTION ?= NO
DEBUG_FINANCE ?= NO
DEBUG_OBJECTS ?= NO
DEBUG_PHYSICS ?= NO

# If the debug output shall be written to atanks.log, set this to YES
DEBUG_LOG_TO_FILE ?= NO

# These three are mutually exclusive. If all are set to yes,
# address-sanitizing has priority, followed by leak, thread
# is last.
SANITIZE_ADDRESS ?= NO
SANITIZE_LEAK    ?= NO
SANITIZE_THREAD  ?= NO

# The following is only used on gcc-4.9+ and only without debugging enabled.
USE_LTO ?= NO

# ------------------------------------
# Install and target directories
# ------------------------------------
PREFIX     ?= /usr
DESTDIR    ?=
BINPREFIX  ?= $(PREFIX)
BINDIR     ?= ${BINPREFIX}/bin
INSTALLDIR ?= ${PREFIX}/share/games/atanks


# ------------------------------------
# Source files and objects
# ------------------------------------
SOURCES := $(wildcard src/*.cpp)
MODULES := $(addprefix obj/,$(notdir $(SOURCES:.cpp=.o)))
DEPENDS := $(addprefix dep/,$(notdir $(SOURCES:.cpp=.d)))


# -------------------------------------
# Platform to build for (Can be forced)
# -------------------------------------
PLATFORM ?= none
ifeq (none,$(PLATFORM))
  # The easiest way is to go through our make goals
  # I know this looks weird, but the following simply means:
  # "If the search for "win" in MAKECMDGOALS does not return an empty string"
  ifneq (,$(findstring win,$(MAKECMDGOALS)))
    PLATFORM := WIN32
  else ifneq (,$(findstring osx,$(MAKECMDGOALS)))
    PLATFORM := MACOSX
  else
    PLATFORM := LINUX
  endif
endif

# If this is a user make goal, the install directory is forced to be local:
ifneq (,$(findstring user,$(MAKECMDGOALS)))
  INSTALLDIR := .
endif


# --------------------------------------------
# Target executable and distribution file name
# --------------------------------------------
TARGET   := atanks
FILENAME := $(TARGET)-$(VERSION)


# ------------------------------------
# Tools to use
# ------------------------------------
INSTALL := $(shell which install)
RM      := $(shell which rm) -f
CXX     ?= g++
SED     := $(shell which sed)
WINDRES :=

ifeq (,$(findstring /,$(CXX)))
  CXX   := $(shell which $(CXX))
endif


# if this is a Windows target, prefer mingw32-g++ over g++
# Further more the WIN32 Platform needs windres.exe to create src/atanks.res
ifeq (WIN32,$(PLATFORM))
  ifneq (,$(findstring /g++,$(CXX)))
    CXX   := $(shell which mingw32-g++)
  endif
  WINDRES := $(shell which windres.exe)
  MODULES := ${MODULES} obj/atanks.res
  TARGET  := ${TARGET}.exe
  RM      := del /q
endif

# Use the compiler as the linker.
LD := $(CXX)

# --------------------------------------------------------------------------
# Determine proper C++11 standard flag, and if and how stack protector works
# --------------------------------------------------------------------------
GCCVERSGTEQ47 := 0
GCCVERSGTEQ49 := 0
GCCUSESGOLD   := 0
GCC_STACKPROT :=
GCC_CXXSTD    := 0x
PEDANDIC_FLAG := -pedantic

# Note: It has to be evaluated which versions of clang and mingw
#       start using c++11 instead of c++0x.
ifneq (,$(findstring /g++,$(CXX)))
  GCCVERSGTEQ47 := $(shell expr `$(CXX) -dumpversion | cut -f1,2 -d. | tr -d '.'` \>= 47)
  GCCVERSGTEQ49 := $(shell expr `$(CXX) -dumpversion | cut -f1,2 -d. | tr -d '.'` \>= 49)
endif

ifeq "$(GCCVERSGTEQ47)" "1"
  GCC_CXXSTD    := 11
  PEDANDIC_FLAG := -Wpedantic
  ifeq "$(GCCVERSGTEQ49)" "1"
    GCC_STACKPROT := -fstack-protector-strong
    GCCUSESGOLD   := $(shell ld --version | head -n 1 | grep -c "GNU gold")
  else
    GCC_STACKPROT := -fstack-protector
  endif
endif


# ------------------------------------
# Flags for compiler and linker
# ------------------------------------
CPPFLAGS += -DDATA_DIR=\"${INSTALLDIR}\" -D$(PLATFORM) -DVERSION=\"${VERSION}\"
CXXFLAGS += -Wall -Wextra $(PEDANDIC_FLAG) -std=c++$(GCC_CXXSTD)
LDFLAGS  +=

# Depending on the platform, some values have to be appended:
ifeq (MACOSX,$(PLATFORM))
  CPPFLAGS := ${CPPFLAGS} -I/usr/local/include $(shell allegro-config --cppflags)
  LDFLAGS  := ${LDFLAGS} $(shell allegro-config --libs)
else ifeq (WIN32,$(PLATFORM))
  CPPFLAGS := ${CPPFLAGS} -I/usr/local/include
  CXXFLAGS := ${CXXFLAGS} -mwindows
  LDFLAGS  := ${LDFLAGS} -mwindows -L. -lalleg44
else
  ifneq (,$(findstring bsd,$(MAKECMDGOALS)))
    C_INCLUDE_PATH     := /usr/local/include
    CPLUS_INCLUDE_PATH := /usr/local/include
    CXXFLAGS           := ${CXXFLAGS} -Wno-c99-extensions
    export C_INCLUDE_PATH
    export CPLUS_INCLUDE_PATH
  endif
  CPPFLAGS := ${CPPFLAGS} -DNETWORK $(shell allegro-config --cppflags)
  CXXFLAGS := ${CXXFLAGS} -pthread
  LDFLAGS  := ${LDFLAGS} $(shell allegro-config --libs) -lm -lpthread
endif


# If the make goal is "ubuntu", a special define is to be added:
ifeq (UBUNTU,$(MAKECMDGOALS))
  CPPFLAGS := ${CPPFLAGS} -DUBUNTU
endif


# ------------------------------------
# Debug Mode settings
# ------------------------------------
HAS_DEBUG_FLAG := NO
ifneq (,$(findstring -g,$(CXXFLAGS)))
  ifneq (,$(findstring -ggdb,$(CXXFLAGS)))
    HAS_DEBUG_FLAG := YES
  endif
  DEBUG := YES
endif

ifeq (YES,$(DEBUG))
  ifeq (NO,$(HAS_DEBUG_FLAG))
    CXXFLAGS := -ggdb ${CXXFLAGS} -O0
  endif

  CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG
  CXXFLAGS := ${CXXFLAGS} ${GCC_STACKPROT} -Wunused

  # LTO is hard blocked now:
  USE_LTO := NO

  # address / thread sanitizer activation
  ifeq (YES,$(SANITIZE_ADDRESS))
    CXXFLAGS := ${CXXFLAGS} -fsanitize=address
    LDFLAGS  := ${LDFLAGS} -fsanitize=address
  else ifeq (YES,$(SANITIZE_LEAK))
    CXXFLAGS := ${CXXFLAGS} -fsanitize=leak
    LDFLAGS  := ${LDFLAGS} -fsanitize=leak
  else ifeq (YES,$(SANITIZE_THREAD))
    CPPFLAGS := ${CPPFLAGS} -DUSE_MUTEX_INSTEAD_OF_SPINLOCK
    CXXFLAGS := ${CXXFLAGS} -fsanitize=thread -fPIC -O2 -ggdb
    LDFLAGS  := ${LDFLAGS} -fsanitize=thread -pie -O2 -ggdb
  endif

  # Add specific debug message flavours
  ifeq (YES,$(DEBUG_AICORE))
    CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG_AIMING -DATANKS_DEBUG_EMOTIONS
  endif
  ifeq (YES,$(DEBUG_AIMING))
    CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG_AIMING
  endif
  ifeq (YES,$(DEBUG_EMOTION))
    CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG_EMOTIONS
  endif
  ifeq (YES,$(DEBUG_FINANCE))
    CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG_FINANCE
  endif
  ifeq (YES,$(DEBUG_OBJECTS))
    CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG_OBJECTS
  endif
  ifeq (YES,$(DEBUG_PHYSICS))
    CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG_PHYSICS
  endif
  ifeq (YES,$(DEBUG_LOG_TO_FILE))
    CPPFLAGS := ${CPPFLAGS} -DATANKS_DEBUG_LOGTOFILE
  endif

else
  CXXFLAGS := -march=native ${CXXFLAGS} -O2
endif


# Potentially enable LTO if this is gcc-4.9 and greater
ifeq (YES,$(USE_LTO))
  ifeq "$(GCCVERSGTEQ49)" "1"
    CXXFLAGS := ${CXXFLAGS} -flto
  endif
  ifeq "$(GCCUSESGOLD)" "1"
    CXXFLAGS := ${CXXFLAGS} -fuse-linker-plugin
  endif
endif



# ------------------------------------
# Distribution file lists
# ------------------------------------
DISTCOMMON := \
atanks/*.dat atanks/COPYING atanks/README atanks/TODO \
atanks/Changelog atanks/BUGS atanks/*.txt

INCOMMON   := COPYING README TODO Changelog *.txt unicode.dat

# ------------------------------------
# Default target
# ------------------------------------

all: $(TARGET)


# ------------------------------------
# Create dependencies
# This is the standard as described
# on the GNU make info manual.
# (See Chapter 4.14)
# ------------------------------------
dep/%.d: src/%.cpp
	@set -e; $(RM) $@; \
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,obj/\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$


# ------------------------------------
# Compile modules
# ------------------------------------
obj/%.o: src/%.cpp
	@echo "Compiling $@"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<


# ------------------------------------
# Build windows res file
# ------------------------------------
obj/atanks.res:
ifeq (WIN32,$(PLATFORM))
	$(WINDRES) -i src/atanks.rc --input-format=rc -o obj/atanks.res -O coff
else
	@echo "This is no WIN32 platform, so why?"
endif


# ------------------------------------
# Regular targets
# ------------------------------------
install: $(TARGET)
	$(INSTALL) -d $(DESTDIR)${BINDIR}
	$(INSTALL) -m 755 atanks $(DESTDIR)${BINDIR}
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 atanks.desktop $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps
	$(INSTALL) -m 644 atanks.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/button
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/misc
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/missile
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/sound
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/stock
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/tank
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/tankgun
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/title
	$(INSTALL) -d $(DESTDIR)${INSTALLDIR}/text
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

$(TARGET): $(MODULES)
	$(LD) -o $@ $(MODULES) $(CPPFLAGS) $(LDFLAGS) $(CXXFLAGS)

clean:
	$(RM) obj/*

veryclean: clean
ifeq (WIN32,$(PLATFORM))
	$(RM) $(TARGET).exe
else
	$(RM) $(TARGET)
endif


# ------------------------------------
# User (local) targets
# ------------------------------------

user: $(TARGET)
winuser: $(TARGET)
osxuser: $(TARGET)
bsduser: $(TARGET)
ubuntu: $(TARGET)

# ------------------------------------
# Distribution targets
# ------------------------------------

dist: source-dist i686-dist win32-dist

tarball: veryclean
	cd .. && tar czf $(FILENAME).tar.gz $(FILENAME) --exclude=.git

zipfile: veryclean
	cd .. && zip -r $(FILENAME)-source.zip $(FILENAME) -x *.git*

source-dist: $(TARGET)
	cd ../; \
	$(RM) $(FILENAME).tar.gz; \
	tar czf $(FILENAME).tar.gz atanks/src/*.cpp atanks/src/*.h atanks/Makefile $(DISTCOMMON)

i686-dist: $(TARGET)
	cd ../; \
	$(RM) $(FILENAME)-i686-dist.tar.gz; \
	strip atanks/atanks; \
	tar czf $(FILENAME)-i686-dist.tar atanks/atanks $(DISTCOMMON)

win32-dist: $(TARGET)
	cd ../; \
	$(RM) $(FILENAME)-win32-dist.zip; \
	zip -r $(FILENAME)-win32-dist.zip atanks/atanks.exe atanks/alleg40.dll $(DISTCOMMON)

# ------------------------------------
# Include all dependency files
# ------------------------------------
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
  -include $(DEPENDS)
endif

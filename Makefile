#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

PLUGIN = restfulapi

USE_LIBMAGICKPLUSPLUS ?= 1

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).cpp | awk '{ print $$6 }' | sed -e 's/[";]//g')

PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
TMPDIR ?= /tmp

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags) -std=c++17

APIVERSION = $(call PKGCFG,apiversion)

-include $(PLGCFG)

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)
SOFILE = libvdr-$(PLUGIN).so

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

LIBS    += $(shell pkg-config --libs cxxtools-http 2>/dev/null || { cxxtools-config --libs; echo -lcxxtools-http; })
CONFDIR  = $(call PKGCFG,configdir)
PLGCONFDIR = $(CONFDIR)/plugins/$(PLUGIN)

ifeq ($(USE_LIBMAGICKPLUSPLUS), 1)
INCLUDES += $(shell pkg-config --cflags Magick++)
LIBS += $(shell pkg-config --libs Magick++)
CXXFLAGS += -DUSE_LIBMAGICKPLUSPLUS
endif

OBJS = $(PLUGIN).o serverthread.o tools.o info.o searchtimers.o channels.o events.o recordings.o recordingmutation.o recordinganalysis.o recordingmoveanalysis.o recordingpreflight.o recordingmovepreflight.o recordingpreview.o recordingmovepreview.o recordingexecution.o recordingmoveexecution.o recordingvalidate.o recordingmovevalidate.o recordingtrashexecutor.o recordingtrash.o remote.o timers.o changestate.o eventsstreamthread.o changestatetracker.o scraper2vdr.o statusmonitor.o osd.o jsonparser.o epgsearch.o wirbelscan.o webapp.o femon.o
CFGS = API.html

all: $(SOFILE) i18n

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file,$(I18Npo),$(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file,$(I18Npo),$(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.cpp)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n test-recording-move-plan test-recording-move-analysis test-recording-move-preflight test-recording-move-execution-gate
i18n: $(I18Nmo) $(I18Npot)

test-recording-move-plan:
	$(CXX) -std=c++17 -Wall -Wextra \
		recordingmutation.cpp \
		tests/test_recording_move_plan.cpp \
		-o /tmp/test_recording_move_plan
	/tmp/test_recording_move_plan

test-recording-move-analysis:
	$(CXX) -std=c++17 -Wall -Wextra \
		recordingmutation.cpp \
		recordingmoveanalysis.cpp \
		tests/test_recording_move_analysis.cpp \
		-o /tmp/test_recording_move_analysis
	/tmp/test_recording_move_analysis

test-recording-move-preflight:
	$(CXX) -std=c++17 -Wall -Wextra \
		recordingmutation.cpp \
		recordingmoveanalysis.cpp \
		recordingmovepreflight.cpp \
		tests/test_recording_move_preflight.cpp \
		-o /tmp/test_recording_move_preflight
	/tmp/test_recording_move_preflight

test-recording-move-execution-gate:
	$(CXX) -std=c++17 -Wall -Wextra \
		recordingmutation.cpp \
		recordingmoveanalysis.cpp \
		recordingmoveexecution.cpp \
		tests/test_recording_move_execution_gate.cpp \
		-o /tmp/test_recording_move_execution_gate
	/tmp/test_recording_move_execution_gate

install-i18n: $(I18Nmsgs)

$(SOFILE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) -o $@ -Wl,--no-whole-archive $(LIBS)

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install-cfg: $(CFGS)
	install -D $^ $(DESTDIR)$(PLGCONFDIR)/$^

install: install-lib install-i18n install-cfg

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)/debian
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~ ._*
	
archive:
	git archive --format=tar.gz --prefix=vdr-plugin-restfulapi-${VERSION}/ --output=../vdr-plugin-restfulapi-${VERSION}.tar.gz master

DEFS = @DEFS@ @sound_flag@ @dl_flag@ @inst_flag@

bin_PROGRAMS = lgeneral

INCLUDES = -I$(top_srcdir)/util $(INTLINCLUDES)
lgeneral_SOURCES = main.c \
config.c date.c \
file.c map.c nation.c \
player.c scenario.c sdl.c misc.c unit.c \
audio.c parser.c list.c unit_lib.c \
terrain.c image.c slot.c gui.c purchase_dlg.c \
event.c windows.c action.c campaign.c \
strat_map.c ai.c engine.c ai_group.c ai_tools.c \
lgeneral.6

AM_CFLAGS = @SDL_CFLAGS@
lgeneral_LDFLAGS = @export_flag@
lgeneral_LDADD   = @mixer_flag@ @dl_lib_flag@ $(top_builddir)/util/libutil.a $(INTLLIBS) @SDL_LIBS@
man_MANS = lgeneral.6

EXTRA_DIST = lgeneral.h \
config.h date.h engine.h \
file.h map.h nation.h player.h \
player.h scenario.h sdl.h misc.h unit.h unit.h \
audio.h parser.h list.h unit_lib.h \
terrain.h image.h slot.h gui.h purchase_dlg.h \
event.h windows.h action.h campaign.h \
strat_map.h ai.h ai_group.h ai_tools.h

SUBDIRS = gfx themes campaigns maps nations sounds music units \
scenarios ai_modules terrain

check_PROGRAMS = testfile

testfile_CFLAGS = -DTESTFILE @SDL_CFLAGS@
testfile_SOURCES = file.c list.c misc.c
testfile_LDADD = $(top_builddir)/util/libutil.a

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(inst_dir)

uninstall-local:
	rm -rf $(DESTDIR)$(inst_dir)

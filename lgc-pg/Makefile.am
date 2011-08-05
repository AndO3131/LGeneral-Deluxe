bin_PROGRAMS = lgc-pg shptool
lgc_pg_SOURCES = main.c units.c shp.c nations.c terrain.c maps.c scenarios.c \
list.c misc.c parser.c lgc-pg.1
lgc_pg_CFLAGS = @SDL_CFLAGS@
LIBS = $(top_builddir)/util/libutil.a @SDL_LIBS@
DEFS = @DEFS@ @inst_flag@
INCLUDES = -I$(top_srcdir)

shptool_SOURCES = shptool.c misc.c shp.c
shptool_CFLAGS = -DSHPTOOL @SDL_CFLAGS@

man_MANS = lgc-pg.1

EXTRA_DIST = units.h shp.h nations.h terrain.h maps.h scenarios.h \
list.h misc.h parser.h shptool.c

SUBDIRS = convdata


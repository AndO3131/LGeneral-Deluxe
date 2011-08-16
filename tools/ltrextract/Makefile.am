LGENERAL_PATH = $(top_srcdir)/src
INTL_PATH = $(top_srcdir)/intl

bin_PROGRAMS = ltrextract

AM_CFLAGS = -DLTREXTRACT -I$(LGENERAL_PATH) -I$(top_srcdir) -I$(top_srcdir)/util $(INTLINCLUDES)
ltrextract_LDADD = $(top_builddir)/util/libutil.a $(INTLLIBS)
ltrextract_SOURCES = ltrextract.c util.c util.h \
	terrain.c terrain.h map.c map.h nation.c nation.h \
	unit_lib.c unit_lib.h scenario.c scenario.h campaign.c campaign.h \
	$(LGENERAL_PATH)/parser.c \
	$(LGENERAL_PATH)/list.c


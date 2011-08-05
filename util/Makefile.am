noinst_LIBRARIES = libutil.a
libutil_a_SOURCES = localize.c localize.h \
	hashtable.c hashtable.h hashtable_private.h \
	hashtable_itr.c hashtable_itr.h \
	hashtable_utility.c hashtable_utility.h \
	paths.c paths.h portability.c $(PATHS_PLATFORM)

if compile_paths_linux
  PATHS_PLATFORM = paths-linux.c
endif

INCLUDES = $(INTLINCLUDES) -I$(top_srcdir)
DEFS = -DHAVE_CONFIG_H -DDATADIR=\"@datadir@\" -DPREFIX=\"@prefix@\"


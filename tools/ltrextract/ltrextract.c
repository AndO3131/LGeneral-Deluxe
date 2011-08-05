/*
    A command line tool for extracting translations from lgeneral-resources.
    Copyright (C) 2005 Leo Savernik  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define _GNU_SOURCE
#include <getopt.h>

#include "list.h"
#include "parser.h"
// #include "misc.h"

#include "campaign.h"
#include "map.h"
#include "nation.h"
#include "scenario.h"
#include "terrain.h"
#include "unit_lib.h"
#include "util.h"

#include "util/localize.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: duplicated here a second time -> consolidate! */
#ifdef __GNUC__
#  define NORETURN __attribute__ ((noreturn))
#  define PRINTF_STYLE(fmtidx, firstoptidx) __attribute__ ((format(printf,fmtidx,firstoptidx)))
#else
#  define NORETURN
#  define PRINTF_STYLE(x,y)
#endif

#define LTREXTRACT_MAJOR 0
#define LTREXTRACT_MINOR 1
#define LTREXTRACT_PATCHLVL 0

typedef int Extractor(struct PData *, struct Translateables *);
typedef int Detector(struct PData *);

/**
 * Structure for registering handlers.
 *
 * They will be detected in the order they're registered here.
 */
static struct ResourceHandler {
  const char *name;
  Extractor *extract;
  Detector *detect;
}
resource_handlers[] = {
  { TR_NOOP("scenario"), scen_extract, scen_detect },
  { TR_NOOP("campaign"), camp_extract, camp_detect },
  { TR_NOOP("map"), map_extract, map_detect },
  { TR_NOOP("nation"), nation_extract, nation_detect },
  { TR_NOOP("terrain"), terrain_extract, terrain_detect },
  { TR_NOOP("unit-library"), unit_lib_extract, unit_lib_detect },
};

enum Options { OPT_VERSION = 256 };

static void abortf(const char *fmt, ...) NORETURN PRINTF_STYLE(1,2);
static void verbosef(int lvl, const char *fmt, ...) PRINTF_STYLE(2,3);

static void syntax(int argc, char **argv);

static const char *domain;
static int verbosity;
static int show_help;
static const char *output_file;
static char **input_files;
static int input_file_count;

static struct Translateables *xl;

static struct option long_options[] =
{
  {"domain", 1, 0, 'd'},
  {"help", 0, &show_help, 1},
  {"output", 1, 0, 'o'},
  {"verbose", 0, 0, 'v'},
  {"version", 0, 0, OPT_VERSION},
  {0, 0, 0, 0}
};

/* aborts with the given error message and exit code 1 */
static void abortf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  exit(1);
}

/* prints a message if 'level' is at least equal to the current verbosity level */
static void verbosef(int level, const char *fmt, ...) {
  va_list ap;
  if (level > verbosity) return;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
}

/* determines the basename */
static const char *get_basename(const char *filename) {
  const char *retval = strrchr(filename, '/');
  return retval ? retval + 1 : filename;
}

/* determines the length of the given filename without extension */
static unsigned get_basename_without_extension_len(const char *basename) {
  const char *retval = strchr(basename, '.');
  return retval ? retval - basename : strlen(basename);
}

/* handle the command line options */
static void process_cmdline(int argc, char **argv) {
  for (;;) {
    int result = getopt_long(argc, argv, "d:o:v", long_options, 0);
    if (result == -1) break;

    switch (result) {
      case 'd': domain = optarg; break;
      case 'o': output_file = optarg; break;
      case 'v': verbosity++; break;
      case OPT_VERSION: printf("%d.%d.%d\n", LTREXTRACT_MAJOR, LTREXTRACT_MINOR, LTREXTRACT_PATCHLVL); exit(0);
    }
  }

  if (show_help) {
    syntax(argc, argv);
    exit(1);
  }

  if (optind >= argc)
    abortf(tr("Input file expected\n"));

  input_files = argv + optind;
  input_file_count = argc - optind;
}

/** initialises tool */
static void init_tool(int argc, char **argv) {
  xl = translateables_create();
  if (domain) translateables_set_domain(xl, domain);
}

static void syntax(int argc, char **argv) {
  printf(tr("Translation extractor for LGeneral resource-files.\n"
         "\n"
         "Syntax: %s [options] [files]\n"), "ltrextract");
  printf(tr("\nOptions:\n"
         "-d, --domain=<domain>\n"
         "\t\tExtract only translations for files matching <domain>\n"
         "-o, --output=<file>\n"
         "\t\tWrite changes to <file> instead of the standard output.\n"
         "-v, --verbose\tIncrease verbosity.\n"
         "    --version\tDisplay version information and exit.\n"
         ));
  printf(tr("\nIf no domain is set, the domain will be read from the first input file\n"
         "If this file has no domain entry, the domain name will be derived from\n"
         "the filename\n"
         ));
}

static void print_prolog(FILE *out) {
  fprintf(out, "/* Automatically extracted translations for domain '%s'. DO NOT EDIT. */\n", translateables_get_domain(xl));
  fprintf(out, "int main(int argc, int argv) {\n"
		"auto const char *translations[] = {\n");
}

static void print_epilog(FILE *out) {
  fprintf(out, "};\n"
  		"}\n"
		);
}

static void print_output_file(const char *output_file) {
  FILE *output;
  if (output_file) {
    output = fopen(output_file, "w");
    if (!output) abortf(tr("Could not create output file %s\n"), output_file);
  } else
    output = stdout;

  verbosef(1, tr("Writing output to %s\n"), output_file);
  print_prolog(output);

  {
    struct TranslateablesIterator *it = translateables_iterator(xl);
    for (; translateables_iterator_advance(it); ) {
      fprintf(output, "/*%s */\n", translateables_iterator_comment(it));
      fprintf(output, "tr(\"%s\"),\n", translateables_iterator_key(it));
    }
    translateables_iterator_delete(it);
  }
  
  print_epilog(output);
}

/** sets translation domain to \c domain if not already set */
static void imply_domain(const char *domain) {
  if (!translateables_get_domain(xl)) {
    verbosef(2, tr("Filtering by implied domain '%s'\n"), domain);
    translateables_set_domain(xl, domain);
  }
}

static void process_input(const char *filename) {
  PData *pd;
  char *domain;
  Extractor *extract = 0;
  unsigned i;

  verbosef(1, tr("Processing %s...\n"), filename);

  pd = parser_read_file("tree", filename);
  if (!pd) abortf("%s\n", parser_get_error());

  /* autodetect file-type */
  for (i = 0; i < sizeof resource_handlers/sizeof resource_handlers[0]; i++) {
    if (resource_handlers[i].detect(pd)) {
      verbosef(2, tr("%s-resource detected\n"), resource_handlers[i].name);
      extract = resource_handlers[i].extract;
      break;
    }
  }

  if (!extract) abortf(tr("Unsupported/invalid lgeneral-resource: %s\n"), filename);

  /* insert domain if none given */
  if (!parser_get_value(pd, "domain", &domain, 0)) {
    PData *pdom;
    char *buffer;
    const char *basename = get_basename(filename);
    unsigned raw_basename_len = get_basename_without_extension_len(basename);
    buffer = alloca(raw_basename_len + 1);
    memcpy(buffer, basename, raw_basename_len);
    buffer[raw_basename_len] = 0;

    /* append raw basename as domain */
    pdom = parser_insert_new_pdata(pd, "domain", list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK ));
    list_add(pdom->values, strdup(buffer));

    verbosef(2, tr("No domain specified. Implying '%s'.\n"), buffer);

    /* make it the criteria if no domain is yet set */
    imply_domain(buffer);
  } else
    imply_domain(domain);

  if (!resource_matches_domain(pd, xl))
    fprintf(stderr, tr("*** Warning: '%s' does not match domain '%s' -> ignored\n"), filename, translateables_get_domain(xl));

  if (!extract(pd, xl))
    fprintf(stderr, tr("*** Warning: Extraction from %s may be incomplete\n"), filename);

  parser_free(&pd);
}

static void process_files(const char **file_list, int file_count) {
  int i;
  for (i = 0; i < file_count; i++) {
    process_input(file_list[i]);
  }
}

int main(int argc, char **argv) {

  process_cmdline(argc, argv);

  init_tool(argc, argv);

  process_files((const char **)input_files, input_file_count);

  print_output_file(output_file);
  return 0;
}

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */

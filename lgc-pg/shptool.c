/*
    A command line tool for converting pg-shp into pixmaps.
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

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <getopt.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "list.h"
#include "misc.h"
#include "shp.h"

#ifdef __GNUC__
#  define NORETURN __attribute__ ((noreturn))
#  define PRINTF_STYLE(fmtidx, firstoptidx) __attribute__ ((format(printf,fmtidx,firstoptidx)))
#else
#  define NORETURN
#  define PRINTF_STYLE(x,y)
#endif

#define SHPTOOL_MAJOR 0
#define SHPTOOL_MINOR 1
#define SHPTOOL_PATCHLVL 0

enum Options { OPT_VERSION = 256 };

static void abortf(const char *fmt, ...) NORETURN PRINTF_STYLE(1,2);
static void verbosef(int lvl, const char *fmt, ...) PRINTF_STYLE(2,3);

static void syntax(int argc, char **argv);

static int verbosity;
static int show_help;
static const char *output_file;
static const char *input_file;
int use_def_pal;

// fake externals for shp.c
const char *source_path;
const char *dest_path;

static struct option long_options[] =
{
  {"defpal", 0, 0, 'd'},
  {"help", 0, &show_help, 1},
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

/* handle the command line options */
static void process_cmdline(int argc, char **argv) {
  for (;;) {
    int result = getopt_long(argc, argv, "v", long_options, 0);
    if (result == -1) break;

    switch (result) {
      case 'd': use_def_pal = 1; break;
      case 'v': verbosity++; break;
      case OPT_VERSION: printf("%d.%d.%d\n", SHPTOOL_MAJOR, SHPTOOL_MINOR, SHPTOOL_PATCHLVL); exit(0);
    }
  }

  if (show_help) {
    syntax(argc, argv);
    exit(1);
  }
  
  /* first parameter is input file */
  if (optind >= argc) abortf("Input file missing\n");
  input_file = argv[optind++];
  
  /* second parameter is output file */  
  if (optind >= argc) abortf("Output file missing\n");
  output_file = argv[optind++];
  
  if (optind < argc) abortf("Excess parameters: %s\n", argv[optind]);
  
  verbosef(2, "use_def_pal:\t\t%d\n"
  		"verbosity:\t\t%d\n"
  	, use_def_pal, verbosity);
}

static void syntax(int argc, char **argv) {
  printf("LGeneral Shp-format extraction tool.\n"
         "\n"
         "Syntax: %s [options] infile outfile\n", "shptool");
  printf("\nOptions:\n"
         "-d, --defpal\tUse default palette.\n"
         "-v, --verbose\tIncrease verbosity.\n"
         "    --version\tDisplay version information and exit.\n"
         );
}

static void convert_shp_to_bmp() {
  PG_Shp *shp;
  verbosef(1, "Reading from %s\n", input_file);
  shp = shp_load(input_file);
  if (!shp) abortf("Input file '%s' could not be loaded\n", input_file);
  
  verbosef(1, "Writing to %s\n", output_file);
  if (SDL_SaveBMP(shp->surf, output_file) < 0)
    abortf("Could not write to '%s'\n", output_file);
}

static void init_sdl() {
  verbosef(2, "Initialising SDL...\n");
  verbosef(3, "SDL_Init\n");
  /* SDL required for graphical conversion */
  SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
  verbosef(3, "SDL_SetVideoMode\n");
  SDL_SetVideoMode( 20, 20, 16, SDL_SWSURFACE );
  verbosef(3, "Registering exit-handler\n");
  atexit( SDL_Quit );
}

int main(int argc, char **argv) {

  process_cmdline(argc, argv);
  init_sdl();
  convert_shp_to_bmp();
  return 0;
}

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */

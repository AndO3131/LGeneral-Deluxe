/*
    A command line tool for viewing unit properties and
    manipulating reinforcements.
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "list.h"
#include "misc.h"

extern const char *scenarios[];
extern const char *unit_classes[];
extern const int num_unit_classes;
extern const char *target_types[];
extern const int num_target_types;
extern const char *nation_table[];
extern const int num_nation_table;
extern const char *sides[];
extern const int num_sides;
extern List *reinf[2][SCEN_COUNT];
static int num_units;

static int within_source_tree;	/* 1 if running within source tree */
static const char *rt_path;	/* start of runtime path of executable */
static int rt_path_len;		/* length of runtime path including trailing slash */

#ifdef __GNUC__
#  define NORETURN __attribute__ ((noreturn))
#  define PRINTF_STYLE(fmtidx, firstoptidx) __attribute__ ((format(printf,fmtidx,firstoptidx)))
#else
#  define NORETURN
#  define PRINTF_STYLE(x,y)
#endif

#define LGED_MAJOR 0
#define LGED_MINOR 1

enum Options { OPT_LIST_SCENARIOS = 256, OPT_UNITDB_FILE, OPT_VERSION };
enum Command { COMMAND_NONE,
	REINF_INSERT, REINF_MODIFY, REINF_REMOVE, REINF_LIST, REINF_BUILD,
	LIST_UNITDB, LIST_SCENARIOS, LIST_NATIONS, LIST_SIDES,
        LIST_TARGET_TYPES, LIST_CLASSES };

typedef int CommandHandler(int argc, char **argv, int opt_index);
typedef void CommandHelp(int argc, char **argv);

static void abortf(const char *fmt, ...) NORETURN PRINTF_STYLE(1,2);
static void verbosef(int lvl, const char *fmt, ...) PRINTF_STYLE(2,3);

static void syntax(int argc, char **argv);
static void insert_reinf_help(int argc, char **argv);
static void modify_reinf_help(int argc, char **argv);
static void remove_reinf_help(int argc, char **argv);
static void rebuild_reinf_help(int argc, char **argv);
static void list_reinf_help(int argc, char **argv);
static void list_unitdb_help(int argc, char **argv);
static void list_scenarios_help(int argc, char **argv);
static void list_nations_help(int argc, char **argv);
static void list_sides_help(int argc, char **argv);
static void list_target_types_help(int argc, char **argv);
static void list_classes_help(int argc, char **argv);

static int insert_reinf(int argc, char **argv, int idx);
static int modify_reinf(int argc, char **argv, int idx);
static int remove_reinf(int argc, char **argv, int idx);
static int rebuild_reinf(int argc, char **argv, int idx);
static int list_reinf(int argc, char **argv, int idx);
static int list_unitdb(int argc, char **argv, int idx);
static int list_scenarios(int argc, char **argv, int idx);
static int list_nations(int argc, char **argv, int idx);
static int list_sides(int argc, char **argv, int idx);
static int list_target_types(int argc, char **argv, int idx);
static int list_classes(int argc, char **argv, int idx);

static struct command {
  const char name[20];
  CommandHandler *run;
  enum Command alias_for;
  CommandHelp *help;
} commands[] = {
  { "", 0, 0, syntax },
  { "insert", insert_reinf, 0, insert_reinf_help },
  { "modify", modify_reinf, 0, modify_reinf_help },
  { "remove", remove_reinf, 0, remove_reinf_help },
  { "list", list_reinf, 0, list_reinf_help },
  { "build", rebuild_reinf, 0, rebuild_reinf_help },
  { "list-units", list_unitdb, 0, list_unitdb_help },
  { "list-scenarios", list_scenarios, 0, list_scenarios_help },
  { "list-nations", list_nations, 0, list_nations_help },
  { "list-sides", list_sides, 0, list_sides_help },
  { "list-target-types", list_target_types, 0, list_target_types_help },
  { "list-classes", list_classes, 0, list_classes_help },

  /* aliases */
  { "add", 0, REINF_INSERT, 0 },
  { "ins", 0, REINF_INSERT, 0 },
  { "change", 0, REINF_MODIFY, 0 },
  { "mod", 0, REINF_MODIFY, 0 },
  { "chg", 0, REINF_MODIFY, 0 },
  { "ch", 0, REINF_MODIFY, 0 },
  { "delete", 0, REINF_REMOVE, 0 },
  { "del", 0, REINF_REMOVE, 0 },
  { "rm", 0, REINF_REMOVE, 0 },
  { "units", 0, LIST_UNITDB, 0 },
  { "unit", 0, LIST_UNITDB, 0 },
  { "list-scens", 0, LIST_SCENARIOS, 0 },
  { "list-scen", 0, LIST_SCENARIOS, 0 },
  { "scens", 0, LIST_SCENARIOS, 0 },
  { "scen", 0, LIST_SCENARIOS, 0 },
  { "list-nat", 0, LIST_NATIONS, 0 },
  { "nations", 0, LIST_NATIONS, 0 },
  { "nats", 0, LIST_NATIONS, 0 },
  { "nat", 0, LIST_NATIONS, 0 },
  { "sides", 0, LIST_SIDES, 0 },
  { "side", 0, LIST_SIDES, 0 },
  { "list-targettype", 0, LIST_TARGET_TYPES, 0 },
  { "target-types", 0, LIST_TARGET_TYPES, 0 },
  { "target-type", 0, LIST_TARGET_TYPES, 0 },
  { "targettypes", 0, LIST_TARGET_TYPES, 0 },
  { "targettype", 0, LIST_TARGET_TYPES, 0 },
  { "tgttypes", 0, LIST_TARGET_TYPES, 0 },
  { "tgttype", 0, LIST_TARGET_TYPES, 0 },
  { "ttp", 0, LIST_TARGET_TYPES, 0 },
  { "tgt", 0, LIST_TARGET_TYPES, 0 },
  { "list-unit-classes", 0, LIST_CLASSES, 0 },
  { "list-unit-class", 0, LIST_CLASSES, 0 },
  { "list-unitclasses", 0, LIST_CLASSES, 0 },
  { "list-unitclass", 0, LIST_CLASSES, 0 },
  { "unit-classes", 0, LIST_CLASSES, 0 },
  { "unit-class", 0, LIST_CLASSES, 0 },
  { "unitclasses", 0, LIST_CLASSES, 0 },
  { "unitclass", 0, LIST_CLASSES, 0 },
  { "classes", 0, LIST_CLASSES, 0 },
  { "class", 0, LIST_CLASSES, 0 },
  { "cls", 0, LIST_CLASSES, 0 },
};

#define NUM_COMMANDS (sizeof commands/sizeof commands[0])

/* predefined locations of files. in_place means in source tree, installed
means in installation directory. */
static const char in_place_unitdb[] = "src/units/pg.udb";
static const char installed_unitdb[1024] = "/units/pg.udb";
static const char in_place_reinfrc[] = "lgeneral-redit/src/reinf.rc";
static const char in_place_reinf_output[] = "lgc-pg/convdata/reinf";
static const char installed_reinf_output[1024] = "/convdata/reinf";

static int command_index = COMMAND_NONE;
static int dry_run;
static int verbosity;
static int show_help;
static const char *output_file;
static const char *input_file;
const char *unitdb;
const char *reinfrc;
const char *reinf_output;

static struct option long_options[] =
{
  {"add", 0, 0, 'a'},
  {"append", 0, 0, 'a'},
  {"build", 0, 0, 'b'},
  {"delete", 0, 0, 'd'},
  {"dry-run", 0, &dry_run, 1},
  {"help", 0, &show_help, 1},
  {"insert", 0, 0, 'i'},
  {"list", 0, 0, 'l'},
  {"modify", 0, 0, 'm'},
  {"output", 1, 0, 'o'},
  {"reinf-file", 1, 0, 'f'},
  {"remove", 0, 0, 'r'},
  {"scenarios", 0, 0, 's' },
  {"unitdb-file", 1, 0, OPT_UNITDB_FILE},
  {"units", 0, 0, 'u'},
  {"verbose", 0, 0, 'v'},
  {"version", 0, 0, OPT_VERSION},
  {0, 0, 0, 0}
};

/* == forward declarations of handler functions. */
static int do_list_reinf(int scen_id, char *scen_bitmap, int side_id, char *side_bitmap);
static void renumber_reinf(void);

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

/* concatenates two strings into a new buffer and returns the result */
static char *concat(const char *s1, const char *s2) {
  unsigned l1 = strlen(s1);
  unsigned l2 = strlen(s2);
  char *r = malloc(l1 + l2 + 1);
  strcpy(r, s1);
  strcpy(r + l1, s2);
  return r;
}

/* resolves alias commands */
static enum Command command_resolve_alias(int cmd) {
  if (cmd < 0 || cmd >= NUM_COMMANDS) return COMMAND_NONE;
  if (commands[cmd].alias_for) cmd = commands[cmd].alias_for;
  return cmd;
}

/* set the command to be executed. Aborts if a command has already been set. */
static void set_command(enum Command cmd) {
  if (command_index != COMMAND_NONE) abortf("Two commands specified\n");
  command_index = cmd;
}

/* handle the command line options */
static void process_cmdline(int argc, char **argv) {
  for (;;) {
    int result = getopt_long(argc, argv, "abdf:ilmo:rsuv", long_options, 0);
    if (result == -1) break;

    switch (result) {
      case 'b': set_command(REINF_BUILD); break;
      case 'f': input_file = optarg; break;
      case 'a':
      case 'i': set_command(REINF_INSERT); break;
      case 'l': set_command(REINF_LIST); break;
      case 'm': set_command(REINF_MODIFY); break;
      case 'o': output_file = optarg; break;
      case 'd':
      case 'r': set_command(REINF_REMOVE); break;
      case 's': set_command(LIST_SCENARIOS); break;
      case 'u': set_command(LIST_UNITDB); break;
      case 'v': verbosity++; break;
      case OPT_UNITDB_FILE: unitdb = optarg; break;
      case OPT_VERSION: printf("%d.%d\n", LGED_MAJOR, LGED_MINOR); exit(0);
    }
  }

  /* check whether the first non option argument is a command. */
  if (optind < argc) {
    int cmd;
    for (cmd = NUM_COMMANDS; cmd > 0; ) {
      cmd--;
      if (strcmp(argv[optind], commands[cmd].name) == 0) {
        set_command(command_resolve_alias(cmd));
        optind++;
        break;
      }
    }
  }

  if (show_help) {
    commands[command_index].help(argc, argv);
    exit(1);
  }

  if (command_index == COMMAND_NONE) abortf("No command specified\n");
}

/** prints all commands */
static void print_commands() {
  int i = 1;
  int cmd_count = 0;
  printf("\t");
  for (; i < NUM_COMMANDS; i++) {
    if (commands[i].alias_for) continue;
    printf("%s%s", cmd_count > 0 ? ", " : "", commands[i].name);
    cmd_count++;
  }
  printf("\n");
}

/** prints all alias-names for cmd */
static void print_alias_names(enum Command cmd) {
  int i = 0;
  int alias_count = 0;
  printf("Alias names:\n\t");
  for (; i < NUM_COMMANDS; i++) {
    if (commands[i].alias_for == cmd) {
      printf("%s%s", alias_count > 0 ? ", " : "", commands[i].name);
      alias_count++;
    }
  }
  if (!alias_count) printf("<none>");
  printf("\n");
}

/**
 * prepends the srcdir, determined from the executable's location,
 * to the given path which must be relative to the srcdir.
 * This is only a guess and is be flatly wrong if lged is installed.
 * Warning: the string is allocated on the heap!
 */
static const char *srcdir_path_from_bindir(const char *path) {
  char buffer[4096];
  if (TOP_SRCDIR[0] == '/')	// absolute
    snprintf(buffer, sizeof buffer, "%s/%s", TOP_SRCDIR, path);
  else				// relative
    snprintf(buffer, sizeof buffer, "%.*s/%s/%s", rt_path_len, rt_path, TOP_SRCDIR, path);
  return strdup(buffer);
}

/** initialises tool */
static void init_tool(int argc, char **argv) {
  /* extract runtime path (relative to current directory). */
  /* this won't work if binary was in $PATH */
  rt_path = strrchr(argv[0], '/');
  if (rt_path) rt_path_len = rt_path + 1 - argv[0];
  rt_path = argv[0];

  /* check whether we're started from within the source directory */
  {
    const char *path = srcdir_path_from_bindir("README.lgeneral");
    FILE *f = fopen(path, "r");
    within_source_tree = !!f;
    if (f) fclose(f);
    free((void *)path);
    
    if (within_source_tree)
      verbosef(2, "Source tree detected. Using inline paths.\n");
  }
  
  /* get path for unitdb */
  if (!unitdb) {
    unitdb = within_source_tree ? srcdir_path_from_bindir(in_place_unitdb) : concat(get_gamedir(), installed_unitdb);
    /* leaks a string, but is harmless */
  }
  verbosef(2, "Location of unit db: %s\n", unitdb);
  
  /* get path for reinforcements */
  reinfrc = input_file;
  if (!reinfrc && within_source_tree) {
    reinfrc = srcdir_path_from_bindir(in_place_reinfrc);
    /* leaks a string, but is harmless */
  }
  verbosef(2, "Location of reinforcements: %s\n", reinfrc);
  
  if (!reinfrc) abortf("Path to reinforcements file missing.\n");
  
  /* check whether reinforcements file exists */
  {
    FILE *f = fopen(reinfrc, "r");
    if (!f)
      fprintf(stderr, "Warning: No reinforcements file at %s. New list created.\n", reinfrc);
    if (f) fclose(f);
  }
  
}

/** returns 1 if criterion is to be omitted */
inline static int is_omitted(const char *criterion) {
  return !criterion || strcmp(criterion, "-") == 0 || strcmp(criterion, "*") == 0;
}

#ifndef HAVE_STRCASESTR
/**
 * returns the pointer of needle within haystack, ignoring case. 0 if
 * needle is not found.
 */
static const char *strcasestr(const char *haystack, const char *needle) {
  while (strcasecmp(haystack, needle) != 0 && *haystack) haystack++;
  return *haystack ? haystack : 0;
}
#endif

/**
 * returns whether needle is a case insensitive substring in any element
 * of ary. If found, the index of the first element is returned,
 * -1 otherwise. 'i' is the starting index, 'sz' the size of the array.
 */
inline static int find_in_array(const char *needle, int i, const char **ary, int sz) {
  for (; i < sz; i++)
    if (strcasestr(ary[i], needle)) return i;
  return -1;
}

/**
 * returns the id of the first unit matching the given pattern or -1.
 * Don't call this while iterating all units by iterate_units_next()
 */
static UnitLib_Entry *find_unit_by_pattern(const char *pattern) {
  UnitLib_Entry *entry;
  iterate_units_begin();
  while ((entry = iterate_units_next())) {
    if (strcasestr(entry->name, pattern)) return entry;
  }
  return 0;
}

/**
 * returns 1 if 'bitmap' is true at position 'idx'. 'sz' is the size of
 * the bitmap.
 */
inline static int in_bitmap(int idx, const char *bitmap, int sz) {
  return idx >= 0 && idx < sz && bitmap[idx];
}

/**
 * prints reinforcements for a specific scenario, both sides. Set
 * 'renumber' to true to renumber the pins of all reinforcements.
 */
static void print_scen_reinf(int scen_id, int renumber) {
  char scen_bitmap[SCEN_COUNT];
  memset(scen_bitmap, 0, sizeof scen_bitmap);
  if (scen_id != -1) scen_bitmap[scen_id] = 1;
  
  if (renumber) renumber_reinf();
  
  do_list_reinf(scen_id, scen_bitmap, -1, 0);
}

/** renumbers the pins of all reinforcements */
static void renumber_reinf(void) {
  int i, j, pin = 1;
  for ( i = 0; i < SCEN_COUNT; i++ ) {
    for ( j = 0; j < 2; j++ ) {
      Unit *unit;
      list_reset( reinf[j][i] );
      while ((unit = list_next( reinf[j][i] )))
        unit->pin = pin++;
    }
  }
}

/**
 * finds a reinforcement by pin or 0 if none. 'scen_id' is assigned the
 * scenario this reinforcement was found in. 'side_id' is assigned the
 * side.
 */
static Unit *find_reinf_by_pin(int pin, int *side_id, int *scen_id) {
  int i, j;
  for ( i = 0; i < SCEN_COUNT; i++ ) {
    for ( j = 0; j < 2; j++ ) {
      Unit *unit;
      list_reset( reinf[j][i] );
      while ((unit = list_next( reinf[j][i] )))
        if (unit->pin == pin) {
          if (scen_id) *scen_id = i;
          if (side_id) *side_id = j;
          return unit;
        }
    }
  }
  return 0;
}

/** writes the changes into the correct reinforcements file */
static void write_reinf(void) {
  /* get path for reinforcements */
  reinfrc = output_file;
  if (!reinfrc && within_source_tree) {
    reinfrc = srcdir_path_from_bindir(in_place_reinfrc);
    /* FIXME: leaks a string */
  }

  if (!save_reinf())
    abortf("Saving changes to %s failed\n", reinfrc);  
}  

static void syntax(int argc, char **argv) {
  printf("LGeneral reinforcement editor and query tool.\n"
         "\n"
         "Syntax: %s [options] command [arguments]\n", "lged");
  printf("\n"
         "<command> is the command to execute. Command is one of:\n");
  print_commands();
  printf("\nCall  %s <command> --help  for help on a specific command\n", "lged");
  printf("\nOptions:\n"
         "-a, --add, --append\n"
         "\t\tSee --insert\n"
         "-b, --build\tRebuild the converter's reinforcements table.\n"
         "\t\tAlias for the command 'build'.\n"
         "-d, --delete\n"
         "\t\tSee --remove\n"
         "-f, --reinf-file=<file>\n"
         "\t\tRead reinforcement tables from <file> instead of the\n"
         "\t\tbuilt-in default.\n"
         "    --dry-run\tPerform all operations, but don't write changes.\n"
         "    --help\tThis help. Use in conjunction with command to show\n"
         "\t\ta command-specific help.\n"
         "-i, --insert\n"
         "\t\tInsert an entry into the reinforcements table.\n"
         "\t\tAlias for the command 'insert'.\n"
         "-l, --list\tList reinforcements for scenarios.\n"
         "\t\tAlias for the command 'list'.\n"
         "-m, --modify\tModify reinforcements. Alias for the command 'modify'.\n"
         "-o, --output=<file>\n"
         "\t\tWrite changes to <file> instead of the built-in default.\n"
         "\t\tThe meaning of the output file depends on the command.\n"
         "-r, --remove\tRemove reinforcements. Alias for the command 'remove'.\n"
         "-s, --scenarios\tPrint a list of all scenarios.\n"
         "\t\tAlias for the command 'list-scenarios'.\n"
         "    --unitdb-file=<file>\n"
         "\t\tRead unit database from <file> instead of the built-in\n"
         "\t\tdefault.\n"
         "-u, --units\tQuery units from the units database.\n"
         "\t\tAlias for the command 'list-units'.\n"
         "-v, --verbose\tIncrease verbosity.\n"
         "    --version\tDisplay version information and exit.\n"
         );
}

static void insert_reinf_help(int argc, char **argv) {
  printf("Insert unit into reinforcements.\n"
      "\n"
      "Syntax: %s %s <scen> <unit> <delay> <str> <exp> <nat> <transp>\n", "lged", commands[REINF_INSERT].name);
  print_alias_names(REINF_INSERT);
  printf("\n"
      "Parameters:\n"
      "<scen>\t\tid or name or pattern of scenario to manipulate\n"
      "<unit>\t\tid or pattern of unit to insert\n"
      "<delay>\t\tturn reinforcement becomes available. Zero-based.\n"
      "<str>\t\tstrength of unit between 1 and 15 (default 10)\n"
      "<exp>\t\texperience level between 0 and 5 (default 0)\n"
      "<nat>\t\tid or keyword of nation unit belongs to. May also be a side\n"
      "\t\tkeyword (command list-sides). If omitted, a suitable default\n"
      "\t\twill be assumed.\n"
      "<transp>\tid or pattern of transport to insert (default none)\n"
      "\n"
      "Whenever a parameter consists of '-' or '*', a suitable default value\n"
      "will be assumed.\n"
  );
}

static int insert_reinf(int argc, char **argv, int idx) {
  int i;
  int scen_id = -1;
  int unit_id = -1;
  int delay = -1;
  int str = 10;
  int exp = 0;
  int nation_id = -1;
  int side_id = -1;
  int transp_id = -1;
  UnitLib_Entry *entry, *trp_entry = 0;
  Unit *unit;

  /* evaluate cmdline */
  for (i = idx; i < argc; i++) {
    if (is_omitted(argv[i])) continue;

    switch (i - idx) {
      case 0: /* <scen> */
        /* check for scenario (exact match) */
        scen_id = to_scenario(argv[i]);
        if (scen_id != -1) break;
        
        /* check for scenario id */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { scen_id = n; break; }
        }
        /* check for scenario (fuzzy search) */
        {
          const char *pattern = argv[i] + (argv[i][0] == '+');
          scen_id = find_in_array(pattern, 0, scenarios, SCEN_COUNT);
          if (scen_id != -1) break;
        }
        abortf("Invalid <scen> pattern: %s\n", argv[i]);

      case 1: /* <unit> */
        /* check for unit id */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { unit_id = n; break; }
        }
        
        entry = find_unit_by_pattern(argv[i] + (argv[i][0] == '+'));
        if (entry) { unit_id = entry->nid; break; }
        
        abortf("Invalid <unit> pattern: %s\n", argv[i]);
        
      case 2: /* <delay> */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { delay = n; break; }
        }
        abortf("Numeric value expected for <delay>: %s\n", argv[i]);
        
      case 3: /* <str> */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { str = n; break; }
        }
        abortf("Numeric value expected for <str>: %s\n", argv[i]);
        
      case 4: /* <exp> */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { exp = n; break; }
        }
        abortf("Numeric value expected for <exp>: %s\n", argv[i]);
        
      case 5: /* <nat> */
        /* check for nation */
        nation_id = to_nation(argv[i]);
        if (nation_id != -1) break;
        
        /* check for side */
        side_id = to_side(argv[i]);
        if (side_id != -1) break;
        
        abortf("Illegal keyword for <nat>: %s\n", argv[i]);
        
      case 6: /* <transp> */
        /* check for special value 'none' */
        if (strcasecmp(argv[i], "none") == 0) break;

        /* check for transport id */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { transp_id = n; break; }
        }
        
        trp_entry = find_unit_by_pattern(argv[i] + (argv[i][0] == '+'));
        if (trp_entry) { transp_id = trp_entry->nid; break; }
        
        abortf("Invalid <transp> pattern: %s\n", argv[i]);
        
      default:
        abortf("Excess argument: %s\n", argv[i]);
    }
  }
  
  /* post-process parameters */
  if (scen_id == -1) abortf("<scen> must be specified\n");
  if (unit_id == -1) abortf("<unit> must be specified\n");
  if (delay < 0) abortf("Illegal value for <delay>: %d\n", delay);
  if (str < 0 || str > 15) abortf("<str> is out of range\n");
  if (exp < 0 || exp > 5) abortf("<exp> is out of range\n");

//   if (!load_reinf())
//     abortf("Could not load reinforcements from %s\n", reinfrc);
    
  entry = find_unit_by_id(unit_id);
  if (!entry) abortf("Illegal unit id: %d\n", unit_id);

  /* determine nation */
  if (nation_id == -1) nation_id = entry->nation;
  /* determine side */
  if (side_id == -1) side_id = nation_to_side(nation_id);
  
  /* transport */
  if (transp_id != -1) {
    trp_entry = find_unit_by_id(transp_id);
    if (!trp_entry) abortf("Illegal transport id: %d\n", transp_id);
  }
  
  /* now that we have unmarshalled all the params,
   * create and fill the unit structure
   */
  unit = calloc(1, sizeof *unit);
  /*unit->pin = ;*/
  unit->uid = unit_id;
  snprintf(unit->id, sizeof unit->id, "%d", unit_id);
  unit->tid = transp_id;
  if (transp_id == -1) strcpy(unit->trp, "none");
  else snprintf(unit->trp, sizeof unit->trp, "%d", transp_id);
  /*unit->info[128];*/
  unit->delay = delay;
  unit->str = str;
  unit->exp = exp;
  unit->nation = nation_id;
  unit->entry = entry;
  unit->trp_entry = trp_entry;
  
  insert_reinf_unit( unit, side_id, scen_id );

  /* print scenario reinforcements for checking */
  if (verbosity >= 1) print_scen_reinf(scen_id, !dry_run);
  
  if (!dry_run) write_reinf();
  return 0;
}

static void modify_reinf_help(int argc, char **argv) {
  printf("Modify reinforcement unit.\n"
      "\n"
      "Syntax: %s %s <pin> <unit> <delay> <str> <exp> <nat> <transp>\n", "lged", commands[REINF_MODIFY].name);
  print_alias_names(REINF_MODIFY);
  printf("\n"
      "Parameters:\n"
      "<pin>\t\tposition of unit over reinforcement tables to manipulate\n"
      "<unit>\t\tid or pattern of unit to change\n"
      "<delay>\t\tturn reinforcement becomes available. Zero-based.\n"
      "<str>\t\tstrength of unit between 1 and 15\n"
      "<exp>\t\texperience level between 0 and 5\n"
      "<nat>\t\tid or keyword of nation unit belongs to. May also be a side\n"
      "\t\tkeyword (command list-sides).\n"
      "<transp>\tid or pattern of transport or 'none' to remove transport\n"
      "\n"
      "Whenever a parameter consists of '-' or '*', the value remains unchanged.\n"
  );
}

static int modify_reinf(int argc, char **argv, int idx) {
  int i;
  int orig_scen_id = -1;
  int orig_side_id = -1;
  int scen_id = -1;
  int pin = -1;
  int unit_id = -1;
  int delay = -1;
  int str = -1;
  int exp = -1;
  int nation_id = -1;
  int side_id = -1;
  int transp_id = -1;
  int remove_transp = 0;
  UnitLib_Entry *entry = 0, *trp_entry = 0;
  Unit *unit;

  /* evaluate cmdline */
  for (i = idx; i < argc; i++) {
    if (is_omitted(argv[i])) continue;

    switch (i - idx) {
      case 0: /* <pin> */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { pin = n; break; }
        }
        abortf("<pin> is not numeric: %s\n", argv[i]);

      case 1: /* <unit> */
        /* check for unit id */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { unit_id = n; break; }
        }
        
        entry = find_unit_by_pattern(argv[i] + (argv[i][0] == '+'));
        if (entry) { unit_id = entry->nid; break; }
        
        abortf("Invalid <unit> pattern: %s\n", argv[i]);
        
      case 2: /* <delay> */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { delay = n; break; }
        }
        abortf("Numeric value expected for <delay>: %s\n", argv[i]);
        
      case 3: /* <str> */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { str = n; break; }
        }
        abortf("Numeric value expected for <str>: %s\n", argv[i]);
        
      case 4: /* <exp> */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { exp = n; break; }
        }
        abortf("Numeric value expected for <exp>: %s\n", argv[i]);
        
      case 5: /* <nat> */
        /* check for nation */
        nation_id = to_nation(argv[i]);
        if (nation_id != -1) break;
        
        /* check for side */
        side_id = to_side(argv[i]);
        if (side_id != -1) break;
        
        abortf("Illegal keyword for <nat>: %s\n", argv[i]);
        
      case 6: /* <transp> */
        /* check for special value 'none' */
        if (strcasecmp(argv[i], "none") == 0) { remove_transp = 1; break; }

        /* check for transport id */
        {
          char *endptr;
          long int n = strtol(argv[i], &endptr, 0);
          /* only take it if it is a valid number */
          if (*endptr == '\0') { transp_id = n; break; }
        }
        
        trp_entry = find_unit_by_pattern(argv[i] + (argv[i][0] == '+'));
        if (trp_entry) { transp_id = trp_entry->nid; break; }
        
        abortf("Invalid <transp> pattern: %s\n", argv[i]);
        
      default:
        abortf("Excess argument: %s\n", argv[i]);
    }
  }
  
  /* post-process parameters */
  if (pin == -1) abortf("<pin> must be specified\n");
  if (delay != -1 && delay < 0) abortf("Illegal value for <delay>: %d\n", delay);
  if (str != -1 && (str < 0 || str > 15)) abortf("<str> is out of range\n");
  if (exp != -1 && (exp < 0 || exp > 5)) abortf("<exp> is out of range\n");

  unit = find_reinf_by_pin(pin, &orig_side_id, &orig_scen_id);
  if (!unit) abortf("No such reinforcement with <pin> %d\n", pin);

  if (unit_id != -1) {    
    entry = find_unit_by_id(unit_id);
    if (!entry) abortf("Illegal unit id: %d\n", unit_id);
  }

  /* transport */
  if (transp_id != -1) {
    trp_entry = find_unit_by_id(transp_id);
    if (!trp_entry) abortf("Illegal transport id: %d\n", transp_id);
  }
  
  /* now that we have unmarshalled all the params,
   * modify the unit structure
   */
  if (unit_id != -1) {
    unit->uid = unit_id;
    unit->entry = entry;
    snprintf(unit->id, sizeof unit->id, "%d", unit_id);
  }
  if (transp_id != -1) {
    unit->tid = transp_id;
    unit->trp_entry = trp_entry;
    snprintf(unit->trp, sizeof unit->trp, "%d", transp_id);
  } else if (remove_transp) {
    unit->tid = -1;
    unit->trp_entry = 0;
    strcpy(unit->trp, "none");
  }
  if (delay != -1) unit->delay = delay;
  if (str != -1) unit->str = str;
  if (exp != -1) unit->exp = exp;
  if (nation_id != -1) unit->nation = nation_id;

  /* determine side from nation if nation is given */
  if (nation_id != -1 && side_id == -1) side_id = nation_to_side(nation_id);
  
  if (scen_id == -1) scen_id = orig_scen_id;

  /* we have to remove and insert it if either nation or side changes */  
  if (nation_id != -1 || side_id != -1) {
    Unit *new_unit = malloc(sizeof *unit);
    *new_unit = *unit;
    new_unit->pin = 0;	/* mark as new */
    list_delete_item(reinf[orig_side_id][orig_scen_id], unit);
    assert(side_id != -1);
    insert_reinf_unit(new_unit, side_id, scen_id);
  }

  /* print scenario reinforcements for checking */
  if (verbosity >= 1) print_scen_reinf(scen_id, !dry_run);
  
  if (!dry_run) write_reinf();  
  return 0;
}

static void remove_reinf_help(int argc, char **argv) {
  printf("Remove reinforcement units.\n"
      "\n"
      "Syntax: %s %s [<pin>] ...\n", "lged", commands[REINF_REMOVE].name);
  print_alias_names(REINF_REMOVE);
  printf("\n"
      "Parameters:\n"
      "<pin>\t\tposition of unit over reinforcement tables to remove\n"
  );
}

static int remove_reinf(int argc, char **argv, int idx) {
  int i;
  int scen_id = -1;
  int side_id = -1;
  Unit *unit;
  int pins[argc - idx];

  /* evaluate cmdline */
  for (i = idx; i < argc; i++) {
    if (is_omitted(argv[i])) continue;

    {
      char *endptr;
      long int n = strtol(argv[i], &endptr, 0);
      /* only take it if it is a valid number */
      if (*endptr == '\0') { pins[i - idx] = n; continue; }
    }
    abortf("<pin> is not numeric: %s\n", argv[i]);
  }

  /* iterate all pins and remove them */  
  for (i = 0; i < argc - idx; i++) {
    unit = find_reinf_by_pin(pins[i], &side_id, &scen_id);
    if (!unit) { printf("Invalid <pin> %d.\n", pins[i]); continue; }
    
    list_delete_item(reinf[side_id][scen_id], unit);
  }

  /* print scenario reinforcements for checking */
  if (verbosity >= 1) print_scen_reinf(scen_id, !dry_run);
  
  if (!dry_run) write_reinf();
  return 0;
}

static void rebuild_reinf_help(int argc, char **argv) {
  printf("Build reinforcement list for game converter lgc-pg.\n"
      "\n"
      "Syntax: %s %s\n", "lged", commands[command_index].name);
  print_alias_names(command_index);
}

static int rebuild_reinf(int argc, char **argv, int idx) {
  if (idx < argc) abortf("Excess parameters to command\n");
  
  /* get path for reinforcements */
  reinf_output = output_file;
  if (!reinf_output) {
    reinf_output = within_source_tree ? srcdir_path_from_bindir(in_place_reinf_output) : concat(get_gamedir(), installed_reinf_output);
    /* FIXME: leaks a string */
  }

  if (!dry_run) build_reinf();
  return 0;
}

static void list_reinf_help(int argc, char **argv) {
  printf("List reinforcements.\n"
      "\n"
      "Syntax: %s %s [criteria]\n", "lged", commands[REINF_LIST].name);
  print_alias_names(REINF_LIST);
  printf("\n"
      "Criteria:\n"
      "\t<criteria> consists of tokens narrowing the search:\n"
      "\t[ scenario | side | [+]pattern ] ...\n"
      "\n"
      "The command will list all reinforcements matching <scenario> AND <side>.\n"
      "Omitting any group will include all items of this group.\n"
      "<scenario> is either the numerical id or the name of the scenario\n"
      "<side> is the player's side\n"
      "<pattern> is a case insensitive pattern matching any part of the scenario's\n"
      "name.\n"
      "Specifying more than one token of the same group will return all\n"
      "reinforcements matching both tokens.\n"
  );
}

static int list_reinf(int argc, char **argv, int idx) {
  int i;
  int scen_id = -1;
  char scen_bitmap[SCEN_COUNT];
  int side_id = -1;
  char side_bitmap[num_sides/2];

  /* initialise bitmaps */
  memset(scen_bitmap, 0, SCEN_COUNT);
  memset(side_bitmap, 0, num_sides/2);
  
  /* evaluate cmdline */
  for (i = idx; i < argc; i++) {
    int id;
    if (is_omitted(argv[i])) continue;

    /* check for scenario (exact match) */
    id = to_scenario(argv[i]);
    if (id != -1) { scen_id = id; scen_bitmap[id] = 1; continue; }
    /* check for unit side */
    id = to_side(argv[i]);
    if (id != -1) { side_id = id; side_bitmap[id] = 1; continue; }
    
    /* check for scenario id */
    {
      char *endptr;
      long int n = strtol(argv[i], &endptr, 0);
      /* only take it if it is a valid number */
      if (*endptr == '\0') { scen_id = n; scen_bitmap[n] = 1; continue; }
    }
    /* check for scenario (fuzzy search) */
    {
      const char *pattern = argv[i] + (argv[i][0] == '+');
      id = 0;
      for (;;) {
        id = find_in_array(pattern, id, scenarios, SCEN_COUNT);
        if (id == -1) break;
        scen_bitmap[id] = 1; scen_id = id++; 
      }
      if (scen_id != -1) continue;
    }
    abortf("Invalid pattern: %s\n", argv[i]);
  }
  
//   if (!load_reinf())
//     abortf("Could not load reinforcements from %s\n", reinfrc);

  return do_list_reinf(scen_id, scen_bitmap, side_id, side_bitmap);
}

static int do_list_reinf(int scen_id, char *scen_bitmap, int side_id, char *side_bitmap) {
  int i, j;

  /* print reinforcements, applying filters as necessary */
  for (j = 0; j < SCEN_COUNT; j++) {
    if (scen_id != -1 && !in_bitmap(j, scen_bitmap, SCEN_COUNT)) continue;
    
    printf("# Reinforcements for %s:\n", scenarios[j]);

    for (i = 0; i < num_sides/2; i++) {
      Unit *unit;
      if (side_id != -1 && !in_bitmap(i, side_bitmap, num_sides/2)) continue;
      
      list_reset(reinf[i][j]);
      while ((unit = list_next(reinf[i][j]))) {
        const char *nat = "---";
        
        if (unit->nation >= 0 && unit->nation < num_nation_table/2)
          nat = nation_table[unit->nation*2];

        if (unit->pin) printf("%3d", unit->pin); else printf("new");
        printf(" %4d %-20s %-3s T%02d (%d, %d exp)",
               unit->uid, unit->entry ? unit->entry->name : "<invalid>",
               nat, unit->delay + 1, unit->str, unit->exp
               );
        if (unit->tid != -1)
          printf(" [%d %s]", unit->tid,
               unit->trp_entry ? unit->trp_entry->name : "<invalid>");
        printf("\n");
      }
    }
  }
  return 0;
}

static void list_unitdb_help(int argc, char **argv) {
  printf("List units from the unit database.\n"
      "\n"
      "Syntax: %s %s [criteria]\n", "lged", commands[LIST_UNITDB].name);
  print_alias_names(LIST_UNITDB);
  printf("\n"
      "Criteria:\n"
      "\t<criteria> consists of tokens narrowing the search:\n"
      "\t[ target-type | class | nation | side | unit-id | [+]pattern ] ...\n"
      "\n"
      "The command will list all units matching <target-type> AND <class> AND\n"
      "<nation> AND <side> AND (<unit-id> OR <pattern>).\n"
      "Omitting any group will include all items of this group.\n"
      "<unit-id> is a numerical id for a specific unit as printed by this command.\n"
      "<pattern> is a case insensitive pattern matching any part of the unit's\n"
      "name.\n"
      "Specifying more than one token of the same group will return all units\n"
      "matching both tokens.\n"
      "\n"
      "See list-<token> for valid tokens.\n"
      "\n"
      "Examples:\n"
      "\tlged unit 12\n"
      "Returns one unit with id 12.\n"
      "\tlged unit so\n"
      "Returns all units belonging to the Soviet Union.\n"
      "\tlged unit eng usa\n"
      "Returns all units either belonging to Great Britain or the USA.\n"
      "\tlged unit pz\n"
      "Returns all units containing \"pz\" anywhere in their names.\n"
      "\tlged unit inf 34\n"
      "Returns all units containing \"inf\" as well as unit with id 34\n"
      "\tlged unit +eng\n"
      "Returns all units containing \"eng\" anywhere in their names.\n"
      "The '+' is needed here to discriminate the pattern against the keyword\n"
      "'eng', which stands for Great Britain.\n"
        );
}

static int list_unitdb(int argc, char **argv, int idx) {
  List *unit_mask_list = list_create(LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK);
  int i;
  int unit_id = -1;
  char unit_bitmap[num_units];
  int class_id = -1;
  char class_bitmap[num_unit_classes/2];
  int tgttype_id = -1;
  char tgttype_bitmap[num_target_types/2];
  int nation_id = -1;
  char nation_bitmap[num_nation_table/2];
  int side_id = -1;
  char side_bitmap[num_sides/2];
  UnitLib_Entry *entry;

  /* initialise bitmaps */
  memset(unit_bitmap, 0, num_units);
  memset(class_bitmap, 0, num_unit_classes/2);
  memset(tgttype_bitmap, 0, num_target_types/2);
  memset(nation_bitmap, 0, num_nation_table/2);
  memset(side_bitmap, 0, num_sides/2);
  
  /* evaluate cmdline */
  for (i = idx; i < argc; i++) {
    int id;
    if (is_omitted(argv[i])) continue;

    /* check for target type */
    id = to_target_type(argv[i]);
    if (id != -1) { tgttype_id = id; tgttype_bitmap[id] = 1; continue; }
    /* check for unit class */
    id = to_unit_class(argv[i]);
    if (id != -1) { class_id = id; class_bitmap[id] = 1; continue; }
    /* check for nation */
    id = to_nation(argv[i]);
    if (id != -1) { nation_id = id; nation_bitmap[id] = 1; continue; }
    /* check for unit side */
    id = to_side(argv[i]);
    if (id != -1) { side_id = id; side_bitmap[id] = 1; continue; }
    /* check for unit id */
    {
      char *endptr;
      long int n = strtol(argv[i], &endptr, 0);
      /* only take it if it is a valid number */
      if (*endptr == '\0') { unit_id = n; unit_bitmap[n] = 1; continue; }
    }
    /* otherwise, interpret as part of the unit name */
    /* remove leading plus if any */
    list_add(unit_mask_list, argv[i] + (argv[i][0] == '+'));
  }

  /* print list, applying filters as necessary */
  iterate_units_begin();
  while ((entry = iterate_units_next())) {
    const char *nat = "---";

    if (tgttype_id != -1 && !in_bitmap(entry->tgttype, tgttype_bitmap, num_target_types/2)) continue;
    if (class_id != -1 && !in_bitmap(entry->class, class_bitmap, num_unit_classes/2)) continue;
    if (nation_id != -1 && !in_bitmap(entry->nation, nation_bitmap, num_nation_table/2)) continue;
    if (side_id != -1 && !in_bitmap(entry->side, side_bitmap, num_sides/2)) continue;
    
    if (in_bitmap(entry->nid, unit_bitmap, num_units));
    else if (unit_mask_list->count) {
      const char *mask;
      
      list_reset(unit_mask_list);
      while ((mask = list_next(unit_mask_list))) {
        if (strcasestr(entry->name, mask)) break;
      }
      
      if (!mask) continue;
    } else if (unit_id != -1)
      continue;
    
    
    if (entry->nation >= 0 && entry->nation < num_nation_table/2)
      nat = nation_table[entry->nation*2];

    printf("%4d %-20s %-8s %-5s %-3s     %04d-%02d to %04d\n",
           entry->nid, entry->name, unit_classes[entry->class*2],
           target_types[entry->tgttype*2], nat,
           entry->start_year, entry->start_month, entry->last_year);
  }

  list_delete(unit_mask_list);
  return 0;
}

static void list_scenarios_help(int argc, char **argv) {
  printf("List scenarios.\n"
      "\n"
      "Syntax: %s %s\n", "lged", commands[command_index].name);
  print_alias_names(command_index);
}

static int list_scenarios(int argc, char **argv, int idx) {
  int i;
  if (idx < argc) abortf("Excess parameters to command\n");
  for (i = 0; i < SCEN_COUNT; i++) {
    printf("%3d %s\n", i, scenarios[i]);
  }
  return 0;
}

static void list_nations_help(int argc, char **argv) {
  printf("List nations.\n"
      "\n"
      "Syntax: %s %s\n", "lged", commands[command_index].name);
  print_alias_names(command_index);
}

static int list_nations(int argc, char **argv, int idx) {
  int i;
  if (idx < argc) abortf("Excess parameters to command\n");
  for (i = 0; i < num_nation_table; i += 2) {
    printf("%-8s %s\n", nation_table[i], nation_table[i+1]);
  }
  return 0;
}

static void list_sides_help(int argc, char **argv) {
  printf("List sides.\n"
      "\n"
      "Syntax: %s %s\n", "lged", commands[command_index].name);
  print_alias_names(command_index);
}

static int list_sides(int argc, char **argv, int idx) {
  int i;
  if (idx < argc) abortf("Excess parameters to command\n");
  for (i = 0; i < num_sides; i += 2) {
    printf("%-8s %s\n", sides[i], sides[i+1]);
  }
  return 0;
}

static void list_classes_help(int argc, char **argv) {
  printf("List unit classes.\n"
      "\n"
      "Syntax: %s %s\n", "lged", commands[command_index].name);
  print_alias_names(command_index);
}

static int list_classes(int argc, char **argv, int idx) {
  int i;
  if (idx < argc) abortf("Excess parameters to command\n");
  for (i = 0; i < num_unit_classes; i += 2) {
    printf("%-10s %s\n", unit_classes[i], unit_classes[i+1]);
  }
  return 0;
}

static void list_target_types_help(int argc, char **argv) {
  printf("List target types.\n"
      "\n"
      "Syntax: %s %s\n", "lged", commands[command_index].name);
  print_alias_names(command_index);
}

static int list_target_types(int argc, char **argv, int idx) {
  int i;
  if (idx < argc) abortf("Excess parameters to command\n");
  for (i = 0; i < num_target_types; i += 2) {
    printf("%-8s %s\n", target_types[i], target_types[i+1]);
  }
  return 0;
}

int main(int argc, char **argv) {

  process_cmdline(argc, argv);

  init_tool(argc, argv);
  init();
  /* get total count of units in unit database */  
  num_units = unit_count();

  return commands[command_index].run(argc, argv, optind);
}

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */

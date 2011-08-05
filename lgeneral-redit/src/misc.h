#ifndef __MISC_H
#define __MISC_H

#include "list.h"
#include <stdio.h>

/* compare strings */
#define STRCMP( str1, str2 ) ( strlen( str1 ) == strlen( str2 ) && !strncmp( str1, str2, strlen( str1 ) ) )

#define AXIS 0
#define ALLIES 1
#ifdef CMDLINE_ONLY
#  define CLASS_COUNT 18
#else
#  define CLASS_COUNT 14
#endif
#define SCEN_COUNT 38

/* unitlib entry */
typedef struct {
    char id[8];
    char name[24];
    int side; /* either axis(0) or allies(1) determined
                 from mirror index list which are the allied
                 units */
    int nid; /* the same as id, but stored as an integer */
#ifdef CMDLINE_ONLY
    int tgttype;
    int class;
    int nation;
    int start_year, start_month;
    int last_year;
#endif
} UnitLib_Entry;

/* reinforcement */
typedef struct {
    int pin; /* PIN to identify unit when removing */
    int uid; /* numerical unit id */
    char id[8]; /* unit id */
    int tid; /* numerical transport id */
    char trp[8]; /* transporter id */
    char info[128]; /* info string */
    int delay;
    int str;
    int exp;
    int nation; /* nation id */
    UnitLib_Entry *entry, *trp_entry;
    /* to reselect unit properties from reinf list */
    int class_id;
    int unit_id;
    int trp_id;
} Unit;

/*
====================================================================
Load/save reinforcement resources
====================================================================
*/
int load_reinf();
int save_reinf();

/*
====================================================================
Build LGC-PG reinforcements file.
====================================================================
*/
void build_reinf();

/*
====================================================================
Inititate application and load resources.
====================================================================
*/
void init();

/*
====================================================================
Cleanup
====================================================================
*/
void finalize();

/*
====================================================================
Copy source to dest and at maximum limit chars. Terminate with 0.
====================================================================
*/
void strcpy_lt( char *dest, char *src, int limit );

/*
====================================================================
Convert string to scenario id. Returns -1 if invalid.
====================================================================
*/
int to_scenario(const char *str);

/*
====================================================================
Convert string to side id. Returns -1 if invalid.
====================================================================
*/
int to_side(const char *str);

/*
====================================================================
Convert string to nation id. Returns -1 if invalid.
====================================================================
*/
int to_nation(const char *str);

/*
====================================================================
Convert string to unit class id. Returns -1 if invalid.
====================================================================
*/
int to_unit_class(const char *str);

/*
====================================================================
Convert string to target type id. Returns -1 if invalid.
====================================================================
*/
int to_target_type(const char *str);

/*
====================================================================
Returns the side this nation typically belongs to.
0 is axis, 1 is allied.
====================================================================
*/
int nation_to_side(int nation);

/*
====================================================================
Returns the total count of units.
====================================================================
*/
int unit_count();

/*
====================================================================
Returns the unit with the given id or 0 if not found.
Don't call this while iterating all units by iterate_units_next()
====================================================================
*/
UnitLib_Entry *find_unit_by_id(int id);

/*
====================================================================
Starts iterating the unit database.
====================================================================
*/
void iterate_units_begin(void);

/*
====================================================================
Returns the next unit or 0 if end reached.
====================================================================
*/
UnitLib_Entry *iterate_units_next(void);

#ifndef CMDLINE_ONLY
/*
====================================================================
Update unit/transporters/reinf list
====================================================================
*/
void update_unit_list( int player, int unit_class );
void update_trp_list( int player );
void update_reinf_list( int scen, int player );
#endif

/*
====================================================================
Read all lines from file.
====================================================================
*/
List* file_read_lines( FILE *file );

/*
====================================================================
Build units info string.
====================================================================
*/
void build_unit_info( Unit *unit, int player );

/*
====================================================================
Insert a new reinforcement unit into the reinforcements array.
====================================================================
*/
void insert_reinf_unit( Unit *unit, int player, int scenario );

#ifdef CMDLINE_ONLY
/*
====================================================================
Return the directory the game data is installed under.
====================================================================
*/
const char *get_gamedir(void);
#endif

#endif

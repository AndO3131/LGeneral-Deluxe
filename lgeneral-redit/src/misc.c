#ifndef CMDLINE_ONLY
#  include <gtk/gtk.h>
#endif
#include <stdlib.h>
#include <string.h>
#ifndef CMDLINE_ONLY
#  include "support.h"
#endif
#include "parser.h"
#include "misc.h"

#ifdef CMDLINE_ONLY

extern const char *unitdb;
extern const char *reinfrc;
extern const char *reinf_output;

#else /* !CMDLINE_ONLY */

extern GtkWidget *window;
const char unitdb[] = "../../src/units/pg.udb";
const char reinfrc[] = "reinf.rc";
const char reinf_output[] = "../../lgc-pg/convdata/reinf";

#endif /* !CMDLINE_ONLY */

const char *unit_classes[] = {
    "inf",      "Infantry",
    "tank",     "Tank",
    "recon",    "Recon",
    "antitank", "Anti-Tank",
    "art",      "Artillery",
    "antiair",  "Anti-Aircraft",
    "airdef",   "Air-Defense",
#ifdef CMDLINE_ONLY
    "fort",	"Fortification",
#endif
    "fighter",  "Fighter", 
    "tacbomb",  "Tactical Bomber",
    "levbomb",  "Level Bomber",
    "sub",      "Submarine",
    "dest",     "Destroyer",
    "cap",      "Capital Ship",
    "carrier",  "Aircraft Carrier",
#ifdef CMDLINE_ONLY
    "landtrp",	"Land Transport",
    "airtrp",	"Air Transport",
    "seatrp",	"Sea Transport",
#else
    /* landtrp is supportive to classes 
       listed above */
#endif
};
#define NUM_UNIT_CLASSES (sizeof unit_classes/sizeof unit_classes[0])
const int num_unit_classes = NUM_UNIT_CLASSES;

const char *target_types[] = {
    "soft",  "Soft Target",
    "hard",  "Hard Target",
    "air",   "Air Target",
    "naval", "Naval Target",
};
#define NUM_TARGET_TYPES (sizeof target_types/sizeof target_types[0])
const int num_target_types = NUM_TARGET_TYPES;

const char *sides[] = {
    "axis",    "Axis Forces",
    "allied",  "Allied Forces",
};
#define NUM_SIDES (sizeof sides/sizeof sides[0])
const int num_sides = NUM_SIDES;

const char *nation_table[] = {
    "aus", "Austria",
    "bel", "Belgia",
    "bul", "Bulgaria",
    "lux", "Luxemburg",
    "den", "Denmark",
    "fin", "Finnland",
    "fra", "France",
    "ger", "Germany",
    "gre", "Greece",
    "usa", "USA",
    "hun", "Hungary",
    "tur", "Turkey",
    "it",  "Italy",
    "net", "Netherlands",
    "nor", "Norway",
    "pol", "Poland",
    "por", "Portugal",
    "rum", "Rumania",
    "esp", "Spain",
    "so",  "Sovjetunion",
    "swe", "Sweden",
    "swi", "Switzerland",
    "eng", "Great Britain",
    "yug", "Yugoslavia",
};
#define NUM_NATION_TABLE (sizeof nation_table/sizeof nation_table[0])
const int num_nation_table = NUM_NATION_TABLE;

const char *scenarios[] = {
    "Poland", "Warsaw", "Norway", "LowCountries", "France",
    "Sealion40", "NorthAfrica", "MiddleEast", "ElAlamein",
    "Caucasus", "Sealion43", "Torch", "Husky", "Anzio",
    "D-Day", "Anvil", "Ardennes", "Cobra", "MarketGarden",
    "BerlinWest", "Balkans", "Crete", "Barbarossa", "Kiev",
    "Moscow41", "Sevastapol", "Moscow42", "Stalingrad",
    "Kharkov", "Kursk", "Moscow43", "Byelorussia",
    "Budapest", "BerlinEast", "Berlin", "Washington",
    "EarlyMoscow", "SealionPlus"
};

/* while axis reinforcements are always 
   supplied to Germany the Allied side 
   depends on the scenario */
const char *nations[] = {
    "pol", "pol", "eng", "eng", "fra", "eng", "eng", "eng",
    "eng", "so", "eng", "usa", "usa", "usa", "usa", "usa",
    "usa", "usa", "usa", "usa", "eng", "eng", "so", "so",
    "so", "so", "so", "so", "so", "so", "so", "so", "so",
    "so", "usa", "usa", "so", "eng"
};

/*
====================================================================
Icon indices that must be mirrored terminated by -1
====================================================================
*/
int mirror_ids[] = {
    83, 84, 85, 86, 87, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    102, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
    115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
    127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
    139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
    151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162,
    163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
    175, 176, 177, 178, 179, 180, 181 ,182, 183, 184, 185, 186, 
    187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198,
    199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 221,
    232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
    244, 250, -1
};

/*
====================================================================
UnitLib entries separated to their classes (axis/allies)
====================================================================
*/
List *unitlib[2][CLASS_COUNT];
List *trplib[2]; /* transporters */
/*
====================================================================
Reinforcements for each scenario (axis/allies)
====================================================================
*/
List *reinf[2][SCEN_COUNT];
/*
====================================================================
Current selections
====================================================================
*/
int cur_scen = 0, cur_player = AXIS, cur_class = 0;
int cur_unit = -1, cur_trp = -1, cur_pin = 1;
int cur_reinf = -1; /* row selection id */

/*
====================================================================
Load/save reinforcement resources
====================================================================
*/
int load_reinf()
{
    char *line;
    Unit *unit;
    int i, j, k, count;
    FILE *file = fopen( reinfrc, "r" );
    List *list, *args;
    if ( file ) 
        list = file_read_lines( file );
    else
        return 0;
    fclose( file );
    cur_pin = 1;
    for ( i = 0; i < SCEN_COUNT; i++ )
    {
        line = list_next( list );
        for ( j = 0; j < 2; j++ ) {
            list_clear( reinf[j][i] );
            count = atoi( list_next( list ) );
            for ( k = 0; k < count; k++ ) {
                line = list_next( list );
                if ( ( args = parser_explode_string( line, ',' ) ) ) {
                    if ( ( unit = calloc( 1, sizeof( Unit ) ) ) ) {
                        const char *arg;
                        unit->pin = cur_pin++;
                        unit->delay = atoi( list_next( args ) );
                        strcpy_lt( unit->id, list_next( args ), 7 );
                        unit->uid = atoi( unit->id );
                        strcpy_lt( unit->trp, list_next( args ), 7 );
                        unit->tid = strcmp( unit->trp, "none" ) == 0 ? -1 : atoi( unit->trp );
                        unit->str = atoi( list_next( args ) );
                        unit->exp = atoi( list_next( args ) );
                        unit->class_id = atoi( list_next( args ) );
                        unit->unit_id = atoi( list_next( args ) );
                        unit->trp_id = atoi( list_next( args ) );
                        if ( (arg = list_next( args )) )
                            unit->nation = atoi( arg );
                        else
                            unit->nation = -1;
                        build_unit_info( unit, j );
                        list_add( reinf[j][i], unit );
                    }
                    list_delete( args );
                }
            }
        }
    }
    list_delete( list );
#ifndef CMDLINE_ONLY
    /* update */
    update_reinf_list( cur_scen, cur_player );
#endif
    return 1;
}
int save_reinf()
{
    Unit *unit;
    int i, j;
    FILE *file = fopen( reinfrc, "w" );
    if ( file == 0 ) return 0;
    for ( i = 0; i < SCEN_COUNT; i++ )
    {
        fprintf( file, "%s\n", scenarios[i] );
        for ( j = 0; j < 2; j++ ) {
            fprintf( file, "%i\n", reinf[j][i]->count );
            list_reset( reinf[j][i] );
            while ( ( unit = list_next( reinf[j][i] ) ) ) 
                fprintf( file, "%i,%s,%s,%i,%i,%i,%i,%i,%i\n", unit->delay,
                         unit->id, unit->trp, unit->str,
                         unit->exp,
                         unit->class_id, unit->unit_id, unit->trp_id,
                         unit->nation );
        }
    }
    fclose( file );
    printf( "Reinforcements saved to %s\n", reinfrc );
    return 1;
}

char *get_nation_id_by_table_id( int id )
{
    static char nat[4];
    strcpy(nat,"---");
    if (id<0||id>=num_nation_table/2) 
        return nat;
    else
    {
        snprintf(nat,4,"%s",nation_table[id*2]);
        return nat;
    }
}
int determine_nation_for_unit(const UnitLib_Entry *entry);

/*
====================================================================
Build LGC-PG reinforcements file in ../../src/convdata
====================================================================
*/
void build_reinf()
{
    Unit *unit;
    int i, j, nat;
    FILE *file = fopen( reinf_output, "w" );
    if ( file ) {
        for ( i = 0; i < SCEN_COUNT; i++ ) {
            if ( reinf[AXIS][i]->count == 0 )
                if ( reinf[ALLIES][i]->count == 0 )
                    continue;
            fprintf( file, "%s {\n", scenarios[i] );
            for ( j = 0; j < 2; j++ ) {
                if ( reinf[j][i]->count == 0 )
                    continue;
                list_reset( reinf[j][i] );
                while ( ( unit = list_next( reinf[j][i] ) ) ) {
                    nat = unit->nation;
                    if (nat==-1)
                    {
                        UnitLib_Entry *entry = find_unit_by_id( unit->uid );
                        nat = determine_nation_for_unit(entry);
                    }
                    fprintf( file, "  unit { nation = %s id = %s trsp = %s "
                                              "delay = %i str = %i "
                                              "exp = %i }\n",
                             get_nation_id_by_table_id(nat),
                             unit->id, unit->trp, unit->delay,
                             unit->str, unit->exp );
                }
            }
            fprintf( file, "}\n\n" );
        }
        fclose( file );
        printf( "Reinforcements built to %s\n", reinf_output );
    }
    else
        printf( "%s not found!\n", reinf_output );
}

/*
====================================================================
Convert string to scenario id. Returns -1 if invalid.
====================================================================
*/
int to_scenario(const char *str) {
    int i = SCEN_COUNT;
    for (; i > 0; ) {
        i--;
        if (strcasecmp(str, scenarios[i]) == 0) return i;
    }
    return -1;
}

/*
====================================================================
Convert string to side id. Returns -1 if invalid.
====================================================================
*/
int to_side(const char *str) {
    int i = NUM_SIDES;
    for (; i > 0; ) {
        i -= 2;
        if (strcasecmp(str, sides[i]) == 0) return i >> 1;
    }
    return -1;
}

/*
====================================================================
Convert string to nation id. Returns -1 if invalid.
====================================================================
*/
int to_nation(const char *str) {
    int i = NUM_NATION_TABLE;
    for (; i > 0; ) {
        i -= 2;
        if (strcasecmp(str, nation_table[i]) == 0) return i >> 1;
    }
    return -1;
}

/*
====================================================================
Convert string to target type id. Returns -1 if invalid.
====================================================================
*/
int to_target_type(const char *str) {
    int i = NUM_TARGET_TYPES;
    for (; i > 0; ) {
        i -= 2;
        if (strcasecmp(str, target_types[i]) == 0) return i >> 1;
    }
    return -1;
}

/*
====================================================================
Convert string to unit class id. Returns -1 if invalid.
====================================================================
*/
int to_unit_class(const char *str) {
    int i = NUM_UNIT_CLASSES;
    for (; i > 0; ) {
        i -= 2;
        if (strcasecmp(str, unit_classes[i]) == 0) return i >> 1;
    }
    return -1;
}

/*
====================================================================
Returns the side this nation typically belongs to.
0 is axis, 1 is allied.
====================================================================
*/
int nation_to_side(int nation) {
   switch (nation) {
       case 0: case 2: case 4: case 5: case 7: case 10: case 11: case 12: case 20: case 21:
           return 0;
       default:
           return 1;
   }
}

/*
====================================================================
Returns the total count of units.
====================================================================
*/
int unit_count() {

    int cnt = 0;
    int side, cls;
    for (side = 0; side < 2; side++) {
        for (cls = 0; cls < CLASS_COUNT; cls++) {
            cnt += unitlib[side][cls]->count;
        }
        cnt += trplib[side]->count;
    }
    return cnt;
}

/*
====================================================================
Determines the nation the given unit belongs to. Returns -1 if
no nation could be determined.
====================================================================
*/
int determine_nation_for_unit(const UnitLib_Entry *entry) {
    static const struct {
        const char * const prefix;
        const char nat[4];
    } prefix_xlate[] = {
        { "ST ", "so" },
        { "US ", "usa" },
        { "GB ", "eng" },
        { "FR ", "fra" },
        { "FFR ", "fra" },
        { "FPO ", "pol" },
        { "IT ", "it" },
        { "PO ", "pol" },
        { "NOR ", "nor" },
        { "LC ", "?" },
        { "AF ", "?" },
        { "AD ", "?" },
        { "Rumanian ", "rum" },
        { "Bulgarian ", "bul" },
        { "Hungarian ", "hun" },
        { "Greek ", "gre" },
        { "Yugoslav ", "yug" },
    };
    int i;
    const char *nat = 0;
    
    for (i = 0; i < sizeof prefix_xlate/sizeof prefix_xlate[0]; i++) {
        int prefix_len = strlen(prefix_xlate[i].prefix);
        if (strncmp(entry->name, prefix_xlate[i].prefix, prefix_len) == 0) {
            nat = prefix_xlate[i].nat;
            break;
        }
    }
    
    /* the Stalinorgeln miss their ST prefix */
    if (entry->nid == 422 || entry->nid == 423) nat = "so";
    else if (!nat) nat = "ger";	/* everything else is German */
    
    return to_nation(nat);
    
}

/*
====================================================================
Returns the unit with the given id or 0 if not found.
Don't call this while iterating all units by iterate_units_next()
====================================================================
*/
UnitLib_Entry *find_unit_by_id(int id) {
    UnitLib_Entry *entry;
    iterate_units_begin();
    while ((entry = iterate_units_next())) {
        if (entry->nid == id) return entry;
    }
    return 0;
}

/** opaque data structure for maintaining iterator state */
struct UdbIteratorState {
    enum { AxisSide, AlliedSide, SidesTraversed } side;
    enum { UnitArray, TransportArray, ArraysTraversed } ary;
    int cls;
    struct UnitLib_Entry *cur;
} udb_it;

/*
====================================================================
Starts iterating the unit database.
====================================================================
*/
void iterate_units_begin(void) {
    udb_it.side = AxisSide;
    udb_it.ary = UnitArray;
    udb_it.cls = 0;
    list_reset(unitlib[udb_it.side][udb_it.cls]);
}

/*
====================================================================
Returns the next unit or 0 if end reached.
====================================================================
*/
UnitLib_Entry *iterate_units_next(void) {
    List *lst = udb_it.ary == UnitArray ? unitlib[udb_it.side][udb_it.cls] : trplib[udb_it.side];
    UnitLib_Entry *item = list_next(lst);
    if (item) return item;

    switch (udb_it.ary) {
        case UnitArray:

            udb_it.cls++;

            if (udb_it.cls == CLASS_COUNT) {
                udb_it.ary++;
                list_reset(trplib[udb_it.side]);
            } else
                list_reset(unitlib[udb_it.side][udb_it.cls]);
            item = iterate_units_next();
            break;

        case TransportArray:

            udb_it.ary++;
            /* fall through */

        case ArraysTraversed:

            udb_it.side++;

            if (udb_it.side < SidesTraversed) {
                udb_it.ary = UnitArray;
                udb_it.cls = 0;
                list_reset(unitlib[udb_it.side][udb_it.cls]);
                item = iterate_units_next();
            }
    }

    return item;
}

/*
====================================================================
Inititate application and load resources.
====================================================================
*/
void init()
{
#ifndef CMDLINE_ONLY
    gchar *row[1];
    GtkWidget *clist = 0;
#endif
    UnitLib_Entry *entry = 0;
    PData *pd = 0, *units = 0, *unit = 0;
    int i, j;
    char *str;
    /* create lists */
    for ( j = 0; j < 2; j++ ) {
        for ( i = 0; i < CLASS_COUNT; i++ )
            unitlib[j][i] = list_create( LIST_AUTO_DELETE, 
                                         LIST_NO_CALLBACK );
        trplib[j] = list_create( LIST_AUTO_DELETE,
                                 LIST_NO_CALLBACK );
        if ( ( entry = calloc( 1, sizeof( UnitLib_Entry ) ) ) ) {
            strcpy( entry->name, "NONE" );
            list_add( trplib[j], entry );
        }
        for ( i = 0; i < SCEN_COUNT; i++ )
            reinf[j][i] = list_create( LIST_AUTO_DELETE, 
                                       LIST_NO_CALLBACK );
    }
    /* load unit lib */
    if ( ( pd = parser_read_file( "unitlib", unitdb ) ) == 0 )
        goto failure;
    if ( !parser_get_pdata( pd, "unit_lib", &units ) )
        goto failure;
    /* load and categorize unit entries */
    list_reset( units->entries );
    while ( ( unit = list_next( units->entries ) ) ) {
        /* id, name and side */
        if ( ( entry = calloc( 1, sizeof( UnitLib_Entry ) ) ) == 0 )
            goto failure;
        strcpy_lt( entry->id, unit->name, 7 );
        entry->nid = atoi(entry->id);
        if ( parser_get_value( unit, "name", &str, 0 ) )
            strcpy_lt( entry->name, str, 23 );
        if ( parser_get_value( unit, "icon_id", &str, 0 ) )
             j = atoi( str );
        i = 0; entry->side = 0;
        while ( 1 ) {
            if ( mirror_ids[i++] == j ) {
                entry->side = 1;
                break;
            }
            if ( mirror_ids[i] == -1 )
                break;
        }
        /* get class and add to list */
        if ( parser_get_value( unit, "class", &str, 0 ) ) {
            int class_id = to_unit_class(str);
#ifdef CMDLINE_ONLY
            entry->class = class_id;
#endif
            if ( STRCMP( "landtrp", str ) )
                /* ground transporters are special */
                list_add( trplib[entry->side], entry );
            else if (class_id >= 0)
                list_add( unitlib[entry->side][class_id], entry );
        }
#ifdef CMDLINE_ONLY
        if ( parser_get_value( unit, "target_type", &str, 0 ) )
             entry->tgttype = to_target_type( str );
        if ( parser_get_value( unit, "start_year", &str, 0 ) )
            entry->start_year = atoi( str );
        if ( parser_get_value( unit, "start_month", &str, 0 ) )
            entry->start_month = atoi( str );
        if ( parser_get_value( unit, "last_year", &str, 0 ) )
            entry->last_year = atoi( str );
        entry->nation = determine_nation_for_unit(entry);
#endif
    }
    /* load reinforcements */
    load_reinf();
#ifndef CMDLINE_ONLY
    /* add scenarios to list l_scenarios */
    if ( ( clist = lookup_widget( window, "scenarios" ) ) ) {
        for ( i = 0; i < SCEN_COUNT; i++ ) {
            row[0] = (char *)scenarios[i];
            gtk_clist_append( GTK_CLIST (clist), row );
        }
    }
    /* show unit list */
    update_unit_list( cur_player, cur_class );
    update_trp_list( cur_player );
#endif
    /* we're done */
    parser_free( &pd );
    return;
failure:
    parser_free( &pd );
    finalize();
    fprintf( stderr, "Aborted: %s\n", parser_get_error() );
    exit( 1 );
}

/*
====================================================================
Cleanup
====================================================================
*/
void finalize()
{
    int i, j;
    for ( j = 0; j < 2; j++ ) {
        for ( i = 0; i < CLASS_COUNT; i++ )
            if ( unitlib[j][i] ) 
                list_delete( unitlib[j][i] );
        if ( trplib[j] )
            list_delete( trplib[j] );
        for ( i = 0; i < SCEN_COUNT; i++ )
            if ( reinf[j][i] ) 
                list_delete( reinf[j][i] );
    }
}

/*
====================================================================
Copy source to dest and at maximum limit chars. Terminate with 0.
====================================================================
*/
void strcpy_lt( char *dest, char *src, int limit )
{
    int len = strlen( src );
    if ( len > limit ) {
        strncpy( dest, src, limit );
        dest[limit] = 0;
    }
    else
        strcpy( dest, src );
}

#ifndef CMDLINE_ONLY
/*
====================================================================
Update unit/transporters list
====================================================================
*/
void update_unit_list( int player, int unit_class )
{
    UnitLib_Entry *entry;
    gchar *row[1];
    GtkWidget *clist;
    if ( ( clist = lookup_widget( window, "units" ) ) ) {
        gtk_clist_clear( GTK_CLIST(clist) );
        list_reset( unitlib[player][unit_class] );
        while ( ( entry = list_next( unitlib[player][unit_class] ) ) ) {
            row[0] = entry->name;
            gtk_clist_append( GTK_CLIST(clist), row );
        }
    }
}
void update_trp_list( int player )
{
    UnitLib_Entry *entry;
    gchar *row[1];
    GtkWidget *clist;
    if ( ( clist = lookup_widget( window, "transporters" ) ) ) {
        gtk_clist_clear( GTK_CLIST(clist) );
        list_reset( trplib[player] );
        while ( ( entry = list_next( trplib[player] ) ) ) {
            row[0] = entry->name;
            gtk_clist_append( GTK_CLIST(clist), row );
        }
    }
}
void update_reinf_list( int scen, int player )
{
    Unit *entry;
    gchar *row[1];
    GtkWidget *clist;
    if ( ( clist = lookup_widget( window, "reinforcements" ) ) ) {
        gtk_clist_clear( GTK_CLIST(clist) );
        list_reset( reinf[player][scen] );
        while ( ( entry = list_next( reinf[player][scen] ) ) ) {
            row[0] = entry->info;
            gtk_clist_append( GTK_CLIST(clist), row );
        }
        gtk_clist_sort( GTK_CLIST(clist) );
    }
}
#endif

/*
====================================================================
Read all lines from file.
====================================================================
*/
List* file_read_lines( FILE *file )
{
    List *list;
    char buffer[1024];

    if ( !file ) return 0;

    list = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    
    /* read lines */
    while( !feof( file ) ) {
        if ( !fgets( buffer, 1023, file ) ) break;
        if ( buffer[0] == 10 ) continue; /* empty line */
        buffer[strlen( buffer ) - 1] = 0; /* cancel newline */
        list_add( list, strdup( buffer ) );
    }
    return list;
}

/*
====================================================================
Build unit's info string.
====================================================================
*/
void build_unit_info( Unit *unit, int player )
{
    UnitLib_Entry *entry = find_unit_by_id( unit->uid );
    UnitLib_Entry *trp_entry = find_unit_by_id( unit->tid );

#ifndef CMDLINE_ONLY
    if ( trp_entry ) {
        snprintf( unit->info, sizeof unit->info,
                  "T %02i: '%s' (Strength: %i, Exp: %i, Nation: %s) [%s]   #%i", 
                  unit->delay + 1, entry ? entry->name : "<invalid>", 
                  unit->str, unit->exp, 
                  (unit->nation==-1)?"auto":get_nation_id_by_table_id(unit->nation), 
                  trp_entry->name, unit->pin );
    }
    else
        snprintf( unit->info, sizeof unit->info,
                  "T %02i: '%s' (Strength: %i, Exp: %i, Nation: %s)   #%i", 
                  unit->delay + 1, entry ? entry->name : "<invalid>", 
                  unit->str, unit->exp, 
                  (unit->nation==-1)?"auto":get_nation_id_by_table_id(unit->nation), 
                  unit->pin );
#endif
    unit->entry = entry;
    unit->trp_entry = trp_entry;
}

/*
====================================================================
Insert a new reinforcement unit into the reinforcements array.
====================================================================
*/
void insert_reinf_unit( Unit *unit, int player, int scenario ) {
    /* insert into list, but grouped by nation */
    {
        Unit *item;
        int i = 0;
        list_reset( reinf[player][scenario] );
        for (i = 0; (item = list_next( reinf[player][scenario] )); i++) {
            if ( unit->nation == item->nation ) break;
        }
        
        list_insert( reinf[player][scenario], unit, i );
    }
}

#ifdef CMDLINE_ONLY

#include "util/paths.h"

/*
====================================================================
Return the directory the game data is installed under.
====================================================================
*/
const char *get_gamedir(void)
{
#ifdef DISABLE_INSTALL
    return ".";
#else
    const char *prefix;
    static char *gamedir;
    static const char suffix[] = "/share/games/lgeneral";
    unsigned len;
    if (gamedir) return gamedir;
    prefix = paths_prefix();
    len = strlen(prefix);
    gamedir = malloc(len + sizeof suffix);
    strcpy(gamedir, prefix);
    strcpy(gamedir + len, suffix);
    return gamedir;
#endif
}
#endif

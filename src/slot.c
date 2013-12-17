/***************************************************************************
                          slot.c  -  description
                             -------------------
    begin                : Sat Jun 23 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/
/***************************************************************************
                          Modifications
    Patch by Galland 2012 http://sourceforge.net/tracker/?group_id=23757&atid=379520
 ***************************************************************************/
/***************************************************************************
                     Modifications by LGD team 2012+.
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <dirent.h>
#include "lgeneral.h"
#include "date.h"
#include "nation.h"
#include "unit.h"
#include "player.h"
#include "map.h"
#include "misc.h"
#include "scenario.h"
#include "slot.h"
#include "campaign.h"
#include "localize.h"

#include <SDL_endian.h>

/*
====================================================================
Externals
====================================================================
*/
extern Config config;
extern Trgt_Type *trgt_types;
extern int trgt_type_count;
extern Mov_Type *mov_types;
extern int mov_type_count;
extern Unit_Class *unit_classes;
extern int unit_class_count;
extern int map_w, map_h;
extern Player *cur_player;
extern int turn;
extern int deploy_turn;
extern int cur_weather;
extern Scen_Info *scen_info;
extern List *units;
extern List *reinf, *avail_units;
extern List *players;
extern int camp_loaded;
extern char *camp_fname;
extern Camp_Entry *camp_cur_scen;
extern Map_Tile **map;

char fname[256]; /* file name */

/*
====================================================================
Locals
====================================================================
*/

/**
 * This enumeration covers the savegame versioning.
 * Each time the layout of the savegame is changed, a new
 * version enumeration constant is inserted before
 * StoreMaxVersion, and the save_* /load_* functions are to
 * be adapted appropriately to handle the higher version.
 */
enum StorageVersion {
    StoreVersionLegacy = 1000,
    StoreUnitIsGuarding,	/* up to this versions are stored per unit */
    StoreGlobalVersioning,
    StorePurchaseData, /* cost for unit lib entries, prestige for players */
    StoreCoreVersionData, /* Core army data are stored in savegame */
    StoreNewFolderStructure, /* scenarios have new folder structure */
    /* insert new versions before this comment */
    StoreMaxVersion,
    StoreHighestSupportedVersion = StoreMaxVersion - 1,
    StoreVersionLimit = 65535	/* must not have more, otherwise endian-detection fails */
};

/** signal endianness treatment of currently processed file */
static int endian_need_swap;
/** version of the loaded file */
static enum StorageVersion store_version;

/*
====================================================================
Save/load slot name to/from file.
====================================================================
*/
static void slot_read_name( char *name, FILE *file )
{
    fread( fname, 256, 1, file );
}
static void slot_write_name( char *name, FILE *file )
{
    fwrite( fname, 256, 1, file );
}

/*
====================================================================
load a single integer with fallback
====================================================================
*/
static int try_load_int( FILE *file, int deflt )
{
    int i;
    if ( fread( &i, sizeof( int ), 1, file ) )
        i = endian_need_swap ? SDL_Swap32(i) : i;
    else
        i = deflt;
    return i;
}

/*
====================================================================
Save/load a single integer
====================================================================
*/
static void save_int( FILE *file, int i )
{
    /* we always write the platform-specific endianness */
    fwrite( &i, sizeof( int ), 1, file );
}
static inline int load_int( FILE *file )
{
    return try_load_int( file, 0 );
}

/*
====================================================================
load a single pointer with fallback
====================================================================
*/
static void *try_load_pointer( FILE *file, void *deflt )
{
    void *p;
    /* we ignore endianness here, you cannot use the pointer
     * anyway.
     */
    if ( !fread( &p, sizeof( void * ), 1, file ) ) p = deflt;
    return p;
}

/*
====================================================================
Save/load a single pointer
====================================================================
*/
static void save_pointer( FILE *file, const void *p )
{
    fwrite( &p, sizeof( void * ), 1, file );
}
static inline void *load_pointer( FILE *file )
{
    return try_load_pointer( file, 0 );
}
/*
====================================================================
Save/load a string to/from file.
====================================================================
*/
static void save_string( FILE *file, char *str )
{
    int length;
    /* save length and then string itself */
    length = strlen( str );
    fwrite( &length, sizeof( int ), 1, file );
    fwrite( str, sizeof( char ), length, file );
}
static char* load_string( FILE *file )
{
    char *str = 0;
    int length;

    fread( &length, sizeof( int ), 1, file );
    str = calloc( length + 1, sizeof( char ) );
    fread( str, sizeof( char ), length, file );
    str[length] = 0;
    return str;
}

/*
====================================================================
Save/load a unit from/to file.
Save the unit struct itself afterwards the id strings of
prop, trsp_prop, player, nation. sel_prop can be rebuild by 
checking embark. As we may merge units prop and trsp_prop
may not be overwritten with original data. Instead all
pointers are resolved by the id string.
====================================================================
*/
enum UnitStoreParams {
    UnitLibSize = 4*sizeof(void *) + 26*sizeof(int) + TARGET_TYPE_LIMIT*sizeof(int)
#ifdef WITH_SOUND
            + 1*sizeof(void *) + 1*sizeof(int)
#endif
            ,
    UnitNameSize = 21*sizeof(char) + 3*sizeof(char)/*padding*/,
    UnitTagSize = 32*sizeof(char),
    UnitBattleHonorsSize = 20*sizeof(char),
    UnitSize = 2*UnitLibSize + 5*sizeof(void *) + UnitNameSize + UnitTagSize + 31*sizeof(int) + 6*UnitBattleHonorsSize
};
static void save_unit_lib_entry( FILE *file, Unit_Lib_Entry *entry )
{
    int i;
    
    COMPILE_TIME_ASSERT(sizeof(Unit_Lib_Entry) == UnitLibSize);
    /* write each member */
    /* reserved (was: id) */
    save_pointer(file, entry->id);
    /* reserved (was: name) */
    save_pointer(file, entry->name);
    /* nation */
    save_int(file, entry->nation);
    /* class */
    save_int(file, entry->class);
    /* trgt_type */
    save_int(file, entry->trgt_type);
    /* ini */
    save_int(file, entry->ini);
    /* mov */
    save_int(file, entry->mov);
    /* mov_type */
    save_int(file, entry->mov_type);
    /* spt */
    save_int(file, entry->spt);
    /* rng */
    save_int(file, entry->rng);
    /* atk_count */
    save_int(file, entry->atk_count);
    /* atks[TARGET_TYPE_LIMIT] */
    for (i = 0; i < TARGET_TYPE_LIMIT; i++)
        save_int(file, entry->atks[i]);
    /* def_grnd */
    save_int(file, entry->def_grnd);
    /* def_air */
    save_int(file, entry->def_air);
    /* def_cls */
    save_int(file, entry->def_cls);
    /* entr_rate */
    save_int(file, entry->entr_rate);
    /* ammo */
    save_int(file, entry->ammo);
    /* fuel */
    save_int(file, entry->fuel);
    /* reserved (was: icon) */
    save_pointer(file, entry->icon);
    /* reserved (was: icon_tiny) */
    save_pointer(file, entry->icon_tiny);
    /* icon_type */
    save_int(file, entry->icon_type);
    /* icon_w */
    save_int(file, entry->icon_w);
    /* icon_h */
    save_int(file, entry->icon_h);
    /* icon_tiny_w */
    save_int(file, entry->icon_tiny_w);
    /* icon_tiny_h */
    save_int(file, entry->icon_tiny_h);
    /* flags */
    save_int(file, entry->flags);
    /* start_year */
    save_int(file, entry->start_year);
    /* start_month */
    save_int(file, entry->start_month);
    /* last_year */
    save_int(file, entry->last_year);
    /* cost */
    save_int(file, entry->cost);
    /* wav_alloc */
#ifdef WITH_SOUND
    COMPILE_TIME_ASSERT_SYMBOL(entry->wav_alloc);
#endif
    save_int(file, 0);
    /* wav_move */
#ifdef WITH_SOUND
    COMPILE_TIME_ASSERT_SYMBOL(entry->wav_move);
#endif
    save_pointer(file, 0);
    /* reserved (was: eval_score) */
    save_int(file, entry->eval_score);
}
static void save_unit( FILE *file, Unit *unit )
{
    /* Don't forget to update save_unit and load_unit if you change Unit.
     * If you have to add a field, use a reserved field.
     * Then increase the version number,
     * and provide defaults on reads of a lower versioned file.
     * Ah, yes: Don't pretend sizeof(int) == sizeof(void *)
     */
    COMPILE_TIME_ASSERT(sizeof(Unit) == UnitSize);
    /* write each member */
    /* prop */
    save_unit_lib_entry(file, &unit->prop);
    /* trsp_prop */
    save_unit_lib_entry(file, &unit->trsp_prop);
    /* reserved (was: sel_prop) */
    save_pointer(file, unit->sel_prop);
    /* reserved (was: backup) */
    save_pointer(file, unit->backup);
    /* name */
    fwrite(unit->name, UnitNameSize, 1, file);
    /* reserved (was: player) */
    save_pointer(file, unit->player);
    /* reserved (was: nation) */
    save_pointer(file, unit->nation);
    /* reserved (was: terrain) */
    save_pointer(file, unit->terrain);
    /* x */
    save_int(file, unit->x);
    /* y */
    save_int(file, unit->y);
    /* str */
    save_int(file, unit->str);
    /* entr */
    save_int(file, unit->entr);
    /* exp */
    save_int(file, unit->exp);
    /* unit storage version (was: exp_level) */
    COMPILE_TIME_ASSERT_SYMBOL(unit->exp_level);
    save_int(file, StoreUnitIsGuarding);
    /* delay */
    save_int(file, unit->delay);
    /* embark */
    save_int(file, unit->embark);
    /* orient */
    save_int(file, unit->orient);
    /* reserved (was: icon_offset) */
    save_int(file, unit->icon_offset);
    /* reserved (was: icon_tiny_offset) */
    save_int(file, unit->icon_tiny_offset);
    /* supply_level */
    save_int(file, unit->supply_level);
    /* cur_fuel */
    save_int(file, unit->cur_fuel);
    /* cur_ammo */
    save_int(file, unit->cur_ammo);
    /* cur_mov */
    save_int(file, unit->cur_mov);
    /* cur_atk_count */
    save_int(file, unit->cur_atk_count);
    /* unused (the *member* is called unused. It's not reserved ;-) ) */
    save_int(file, unit->unused);
    /* reserved (was: damage_bar_width) */
    save_int(file, unit->damage_bar_width);
    /* is_guarding (was: damage_bar_offset) */
    COMPILE_TIME_ASSERT_SYMBOL(unit->damage_bar_offset);
    save_int(file, unit->is_guarding);
    /* reserved (was: suppr) */
    save_int(file, unit->suppr);
    /* turn_suppr */
    save_int(file, unit->turn_suppr);
    /* killed */
    save_int(file, unit->killed);
    /* reserved (was: fresh_deploy) */
    save_int(file, unit->fresh_deploy);
    /* tag */
    fwrite(unit->tag, UnitTagSize, 1, file);
    /* reserved (was: eval_score) */
    save_int(file, unit->eval_score);
    /* reserved (was: target_score) */
    save_int(file, unit->target_score);
    /* prop.id */
    save_string( file, unit->prop.id );
    /* trsp_prop.id */
    if ( unit->trsp_prop.id )
        save_string( file, unit->trsp_prop.id );
    else
        save_string( file, "none" );
    /* player */
    save_string( file, unit->player->id );
    /* nation */
    save_string( file, unit->nation->id );
    /* max_str */
    save_int(file, unit->max_str);
    /* core */
    save_int(file, unit->core);
    /* battle honors */
    int i;
    for (i = 0;i < 5;i++)
        fwrite(unit->star[i], UnitBattleHonorsSize, 1, file);
}
static void load_unit_lib_entry( FILE *file, Unit_Lib_Entry *entry )
{
    int i;
    void *p;
    /* write each member */
    /* reserved */
    p = load_pointer(file);
    /* reserved (was: name) */
    p = load_pointer(file);
    /* nation (since StorePurchaseData) */
    entry->nation = -1;
    if (store_version >= StorePurchaseData)
        entry->nation = load_int(file);
    /* class */
    entry->class = load_int(file);
    /* trgt_type */
    entry->trgt_type = load_int(file);
    /* ini */
    entry->ini = load_int(file);
    /* mov */
    entry->mov = load_int(file);
    /* mov_type */
    entry->mov_type = load_int(file);
    /* spt */
    entry->spt = load_int(file);
    /* rng */
    entry->rng = load_int(file);
    /* atk_count */
    entry->atk_count = load_int(file);
    /* atks[TARGET_TYPE_LIMIT] */
    for (i = 0; i < TARGET_TYPE_LIMIT; i++)
        entry->atks[i] = load_int(file);
    /* def_grnd */
    entry->def_grnd = load_int(file);
    /* def_air */
    entry->def_air = load_int(file);
    /* def_cls */
    entry->def_cls = load_int(file);
    /* entr_rate */
    entry->entr_rate = load_int(file);
    /* ammo */
    entry->ammo = load_int(file);
    /* fuel */
    entry->fuel = load_int(file);
    /* reserved (was: icon) */
    load_pointer(file);
    /* reserved (was: icon_tiny) */
    load_pointer(file);
    /* icon_type */
    entry->icon_type = load_int(file);
    /* icon_w */
    entry->icon_w = load_int(file);
    /* icon_h */
    entry->icon_h = load_int(file);
    /* icon_tiny_w */
    entry->icon_tiny_w = load_int(file);
    /* icon_tiny_h */
    entry->icon_tiny_h = load_int(file);
    /* flags */
    entry->flags = load_int(file);
    /* start_year */
    entry->start_year = load_int(file);
    /* start_month */
    entry->start_month = load_int(file);
    /* last_year */
    entry->last_year = load_int(file);
    /* cost (since StorePurchaseData) */
    entry->cost = 0;
    if (store_version >= StorePurchaseData)
        entry->cost = load_int(file);
    /* wav_alloc */
#ifdef WITH_SOUND
    entry->wav_alloc =
#endif
    load_int(file);
    /* wav_move */
    load_pointer(file);
    /* reserved (was: eval_score) */
    load_int(file);

    /* recalculate members not being stored */
    unit_lib_eval_unit( entry );
}
Unit* load_unit( FILE *file )
{
    Unit_Lib_Entry *lib_entry;
    Unit *unit = 0;
    char *str;
    int unit_store_version;
    int val;

    unit = calloc( 1, sizeof( Unit ) );
    /* read each member */
    /* prop */
    load_unit_lib_entry(file, &unit->prop);
    /* trsp_prop */
    load_unit_lib_entry(file, &unit->trsp_prop);
    /* reserved */
    load_pointer(file);
    /* reserved */
    load_pointer(file);
    /* name */
    fread(unit->name, UnitNameSize, 1, file);
    unit->name[UnitNameSize - 1] = 0;
    /* reserved */
    load_pointer(file);
    /* reserved */
    load_pointer(file);
    /* reserved */
    load_pointer(file);
    /* x */
    unit->x = load_int(file);
    /* y */
    unit->y = load_int(file);
    /* str */
    unit->str = load_int(file);
    /* entr */
    unit->entr = load_int(file);
    /* exp */
    unit->exp = load_int(file);
    /* unit storage version (was: exp_level) */
    unit_store_version = load_int(file);
    /* delay */
    unit->delay = load_int(file);
    /* embark */
    unit->embark = load_int(file);
    /* orient */
    unit->orient = load_int(file);
    /* reserved (was: icon_offset) */
    load_int(file);
    /* reserved (was: icon_tiny_offset) */
    load_int(file);
    /* supply_level */
    unit->supply_level = load_int(file);
    /* cur_fuel */
    unit->cur_fuel = load_int(file);
    /* cur_ammo */
    unit->cur_ammo = load_int(file);
    /* cur_mov */
    unit->cur_mov = load_int(file);
    /* cur_atk_count */
    unit->cur_atk_count = load_int(file);
    /* unused (the *member* is called unused. It's not reserved ;-) ) */
    unit->unused = load_int(file);
    /* reserved (was: damage_bar_width) */
    load_int(file);
    /* is_guarding (was: damage_bar_offset) */
    val = load_int(file);
    unit->is_guarding = unit_store_version >= StoreUnitIsGuarding ? val : 0;
    /* reserved (was: suppr) */
    load_int(file);
    /* turn_suppr */
    unit->turn_suppr = load_int(file);
    /* killed */
    unit->killed = load_int(file);
    /* reserved (was: fresh_deploy) */
    load_int(file);
    /* tag */
    fread(unit->tag, UnitTagSize, 1, file);
    unit->tag[UnitTagSize - 1] = 0;
    /* reserved (was: eval_score) */
    load_int(file);
    /* reserved (was: target_score) */
    load_int(file);

    unit->backup = calloc( 1, sizeof( Unit ) );
    /* sel_prop */
    if ( unit->embark == EMBARK_NONE )
        unit->sel_prop = &unit->prop;
    else
        unit->sel_prop = &unit->trsp_prop;
    /* props */
    str = load_string( file );
    lib_entry = unit_lib_find( str );
    unit->prop.id = lib_entry->id;
    unit->prop.name = lib_entry->name;
    unit->prop.icon = lib_entry->icon;
    unit->prop.icon_tiny = lib_entry->icon_tiny;
#ifdef WITH_SOUND
    unit->prop.wav_move = lib_entry->wav_move;
#endif
    free( str );
    /* transporter */
    str = load_string( file );
    lib_entry = unit_lib_find( str );
    if ( lib_entry ) {
        unit->trsp_prop.id = lib_entry->id;
        unit->trsp_prop.name = lib_entry->name;
        unit->trsp_prop.icon = lib_entry->icon;
        unit->trsp_prop.icon_tiny = lib_entry->icon_tiny;
#ifdef WITH_SOUND
        unit->trsp_prop.wav_move = lib_entry->wav_move;
#endif
    }
    free( str );
    /* player */
    str = load_string( file );
    unit->player = player_get_by_id( str );
    free( str );
    /* nation */
    str = load_string( file );
    unit->nation = nation_find( str );
    free( str );
    /* recalculate members that aren't stored */
    if ( store_version >= StoreCoreVersionData )
    {
		/* patch by Galland 2012 http://sourceforge.net/tracker/?group_id=23757&atid=379520 */
		unit->terrain = map[unit->x][unit->y].terrain;
		/* end patch */
		val = load_int(file);
		/* max_str (since StoreCoreVersionData) */
		unit->max_str = unit_store_version >= StoreCoreVersionData ? val : 10;
		/* core (since StoreCoreVersionData) */
		val = load_int(file);
		unit->core = unit_store_version >= StoreCoreVersionData ? val : AUXILIARY;
		/* battle honors (since StoreCoreVersionData) */
		int i;
		if (store_version >= StoreCoreVersionData)
		    for (i = 0;i < 5;i++)
		    {
		        fread(unit->star[i], UnitBattleHonorsSize, 1, file);
		        unit->star[i][UnitBattleHonorsSize - 1] = 0;
		    }
    }
    unit_adjust_icon( unit );
    unit->exp_level = unit->exp / 100;
    unit_update_bar( unit );
    return unit;
}
/*
====================================================================
Save/load player structs.
The order of players in the scenario file must not have changed.
It is assumed to be kept so only some values are saved.
====================================================================
*/
static void save_player( FILE *file, Player *player )
{
    /* control, air/sea trsp used */
    save_int( file, player->ctrl - 1/* compat with old save games */ );
    save_int( file, player->air_trsp_used );
    save_int( file, player->sea_trsp_used );
    save_int( file, player->cur_prestige );
    save_string( file, player->ai_fname );
}
static void load_player( FILE *file, Player *player )
{
    /* control, air/sea trsp used */
    player->ctrl = load_int( file ) + 1/* compat with old save games */;
    player->air_trsp_used = load_int( file );
    player->sea_trsp_used = load_int( file );
    player->cur_prestige = 0;
    if (store_version >= StorePurchaseData)
        player->cur_prestige = load_int( file );
    free( player->ai_fname );
    player->ai_fname = load_string( file );
}
/*
====================================================================
Save indices in scen::units of ground and air unit on this tile.
====================================================================
*/
static void save_map_tile_units( FILE *file, Map_Tile *tile )
{
    int i;
    int index;
    index = -1;
    /* ground */
    list_reset( units );
    if ( tile->g_unit )
        for ( i = 0; i < units->count; i++ )
            if ( tile->g_unit == list_next( units ) ) {
                index = i;
                break;
            }
    save_int( file, index );
    /* air */
    index = -1;
    list_reset( units );
    if ( tile->a_unit )
        for ( i = 0; i < units->count; i++ )
            if ( tile->a_unit == list_next( units ) ) {
                index = i;
                break;
            }
    save_int( file, index );
}
/* load map tile units assuming that scen::units is set to the correct units */
static void load_map_tile_units( FILE *file, Unit **unit, Unit **air_unit )
{
    int index;
    index = load_int( file );
    if ( index == -1 ) *unit = 0;
    else
        *unit = list_get( units, index );
    index = load_int( file );
    if ( index == -1 ) *air_unit = 0;
    else
        *air_unit = list_get( units, index );
}
/*
====================================================================
Save map flags: nation id and player id strings
====================================================================
*/
static void save_map_tile_flag( FILE *file, Map_Tile *tile )
{
    if ( tile->nation )
        save_string( file, tile->nation->id );
    else
        save_string( file, "none" );
    if ( tile->player )
        save_string( file, tile->player->id );
    else
        save_string( file, "none" );
}
static void load_map_tile_flag( FILE *file, Nation **nation, Player **player )
{
    char *str;
    str = load_string( file );
    *nation = nation_find( str );
    free( str );
    str = load_string( file );
    *player = player_get_by_id( str );
    free( str );
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Check the save directory for saved games and add them to the 
slot list else setup a new entry: '_index_ <empty>'
====================================================================
*/
/*void slots_init()
{
    int i;
    DIR *dir = 0;
    struct dirent *entry = 0;
    FILE *file = 0;*/
    /* open directory */
/*    if ( ( dir = opendir( config.dir_name ) ) == 0 ) {
        fprintf( stderr, tr("init_slots: can't open directory '%s' to read saved games\n"),
                 config.dir_name );
        return;
    }
    closedir(dir);
}*/

/*
====================================================================
Get full slot name from id.
====================================================================
*/
//char *slot_get_name( int id )
//{
//    if ( id >= SLOT_COUNT ) id = 0;
//    return slots[id].name;
//}

/*
====================================================================
Save/load game to/from file.
====================================================================
*/
int slot_save( char *name )
{
    FILE *file = 0;
    char path[512];
    int i, j;
    /* NOTE: If you change the savegame-layout, don't forget
       to increase the version and add proper discrimination
       for loading
       layout:
        slot_name
        version (since version StoreGlobalVersioning)
        campaign loaded
        campaign name (optional)
        campaign scenario id (optional)
        scenario file name
        fog_of_war
        supply
        weather
	deploy
	purchase
        current turns
        current player_id
        player info
        unit count
        units
        reinf unit count
        reinf units
        available unit count
        available units
        map width
        map height
        map tile units
        map tile flags
    */
    /* if we ever hit this, we have *big* problems */
    COMPILE_TIME_ASSERT(StoreHighestSupportedVersion <= StoreVersionLimit);
    /* get file name */
    sprintf( path, "%s/pg/Save/%s", config.dir_name, name );
    /* open file */
    if ( ( file = fopen( path, "w" ) ) == 0 ) {
        fprintf( stderr, tr("%s: not found\n"), path );
        return 0;
    }
    /* write slot identification */
    slot_write_name( name, file );
    /* write version */
    save_int(file, StoreHighestSupportedVersion);
    /* if campaign is set some campaign info follows */
    save_int( file, camp_loaded );
    if ( camp_loaded ) {
        save_string( file, camp_fname );
        save_string( file, camp_cur_scen->id );
    }
    /* basic data */
    save_string( file, scen_info->fname );
    save_int( file, config.fog_of_war );
    save_int( file, config.supply );
    save_int( file, config.weather );
    save_int( file, config.deploy_turn );
    save_int( file, config.purchase );
    save_int( file, config.merge_replacements );
    save_int( file, config.use_core_units );
    save_int( file, turn );
    save_int( file, player_get_index( cur_player ) );
    /* players */
    list_reset( players );
    for ( i = 0; i < players->count; i++ )
        save_player( file, list_next( players ) );
    /* units */
    list_reset( units );
    save_int( file, units->count );
    for ( i = 0; i < units->count; i++ )
        save_unit( file, list_next( units ) );
    /* reinforcements */
    list_reset( reinf );
    save_int( file, reinf->count );
    for ( i = 0; i < reinf->count; i++ )
        save_unit( file, list_next( reinf ) );
    list_reset( avail_units );
    save_int( file, avail_units->count );
    for ( i = 0; i < avail_units->count; i++ )
        save_unit( file, list_next( avail_units ) );
    /* map stuff */
    save_int( file, map_w );
    save_int( file, map_h );
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ )
            save_map_tile_units( file, map_tile( i, j ) );
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ )
            save_map_tile_flag( file, map_tile( i, j ) );
    fclose( file );
    return 1;
}
int slot_load( char *name )
{
    FILE *file = 0;
    char path[512], temp[256];
    int camp_saved;
    int i, j;
    char *scen_file_name = 0;
    int unit_count;
    char *str;
	
    store_version = StoreVersionLegacy;
    /* get file name */
    sprintf( path, "%s/pg/Save/%s", config.dir_name, name );
    /* open file */
#ifdef WIN32
    if ( ( file = fopen( path, "rb" ) ) == 0 ) {
#else
    if ( ( file = fopen( path, "r" ) ) == 0 ) {
#endif
        fprintf( stderr, tr("%s: not found\n"), path );
        return 0;
    }
    /* read slot identification -- won't change anything but the file handle needs to move */
    slot_read_name( temp, file );
    /* read version */
    fread( &store_version, sizeof( int ), 1, file );
    /* check for endianness */
    endian_need_swap = !!(store_version & 0xFFFF0000);
    if (endian_need_swap)
        store_version = SDL_Swap32(store_version);

    /* reject game if it's too new */
    if (store_version > StoreHighestSupportedVersion) {
        fprintf(stderr, "%s: Needs version %d, we only provide %d\n", path, store_version, StoreHighestSupportedVersion);
        fclose(file);
        return 0;
    }
    /* if no versioning yet, it is legacy */
    if (store_version < StoreVersionLegacy)
        camp_saved = store_version;
    else	/* otherwise, campaign flag comes afterwards */
        camp_saved = load_int(file);
    /* if campaign is set some campaign info follows */
    camp_delete();
    if ( camp_saved ) {
        /* reload campaign and set to current scenario id */
        str = load_string( file );
        camp_load( str );
        free( str );
        str = load_string( file );
        camp_set_cur( str );
        free( str );
    }
    /* the scenario that is loaded now is the one that belongs to the scenario id of the campaign above */
    /* read scenario file name */
    scen_file_name = load_string( file );
    if ( store_version < StoreNewFolderStructure )
        snprintf( scen_file_name, strlen( scen_file_name ), "%s", strrchr( scen_file_name, '/' ) );
    if ( !scen_load( scen_file_name ) )
    {
        free( scen_file_name );
        fclose(file);
        return 0;
    }
    free( scen_file_name );
    /* basic data */
    config.fog_of_war = load_int( file );
    config.supply = load_int( file );
    config.weather = load_int( file );
    if (store_version < StorePurchaseData) {
        /* since no proper prestige info for already played turns and all 
         * stored units have cost 0 which means there is no gain for damaging
         * or destroying units, disable purchase for old saved games. to
         * prevent confusion display a note. :-)
         * deploy is left unchanged and just used as configured by command 
         * line option. */
        if (config.purchase != NO_PURCHASE) {
            printf( "**********\n"
		"Note: Games saved before LGeneral version 1.2 do not provide proper prestige\n"
		"and purchase information. Therefore purchase option will be disabled. You can\n"
		"re-enable it in the settings menu when starting a new scenario.\n"
		"**********\n");
            config.purchase = NO_PURCHASE;
        }
    } else {
        config.deploy_turn = load_int( file );
        config.purchase = load_int( file );
    }
    if (store_version >= StoreCoreVersionData) {
        config.merge_replacements = load_int( file );
        config.use_core_units = load_int( file );
    }
    turn = load_int( file );
    cur_player = player_get_by_index( load_int( file ) );
    cur_weather = scen_get_weather();
    /* players */
    list_reset( players );
    for ( i = 0; i < players->count; i++ )
        load_player( file, list_next( players ) );
    /* unit stuff */
    list_clear( units );
    unit_count = load_int( file );
    for ( i = 0; i < unit_count; i++ )
        list_add( units, load_unit( file ) );
    list_clear( reinf );
    unit_count = load_int( file );
    for ( i = 0; i < unit_count; i++ )
        list_add( reinf, load_unit( file ) );
    list_clear( avail_units );
    unit_count = load_int( file );
    for ( i = 0; i < unit_count; i++ )
        list_add( avail_units, load_unit( file ) );
    /* map stuff */
    map_w = load_int( file );
    map_h = load_int( file );
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ ) {
            load_map_tile_units( file, &map[i][j].g_unit, &map[i][j].a_unit );
            if ( map[i][j].g_unit )
                map[i][j].g_unit->terrain = map[i][j].terrain;
            if ( map[i][j].a_unit )
                map[i][j].a_unit->terrain = map[i][j].terrain;
        }
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ )
            load_map_tile_flag( file, &map[i][j].nation, &map[i][j].player );
    /* cannot have deployment-turn (those cannot be saved) */
    deploy_turn = 0;
    fclose( file );
    return 1;
}

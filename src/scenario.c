/***************************************************************************
                          scenario.c  -  description
                             -------------------
    begin                : Fri Jan 19 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
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

#include "lgeneral.h"
#include "parser.h"
#include "date.h"
#include "nation.h"
#include "unit.h"
#include "file.h"
#include "map.h"
#include "scenario.h"
#include "localize.h"
#include "campaign.h"
#include "FPGE/pgf.h"

#if DEBUG_CAMPAIGN
#  include <stdio.h>
#  include <stdlib.h>
#endif

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern Config config;
extern Nation *nations;
extern int nation_count;
extern int nation_flag_width, nation_flag_height;
extern Trgt_Type *trgt_types;
extern int trgt_type_count;
extern Mov_Type *mov_types;
extern int mov_type_count;
extern Unit_Class *unit_classes;
extern int unit_class_count;
extern Map_Tile **map;
extern Mask_Tile **mask;
extern int map_w, map_h;
extern Weather_Type *weather_types;
extern int weather_type_count;
extern List *players;
extern Font *log_font;
extern Setup setup;
extern int deploy_turn;
extern List *unit_lib;
extern int camp_loaded;
extern Camp_Entry *camp_cur_scen;
extern char *camp_first;

/*
====================================================================
Scenario data.
====================================================================
*/
Setup setup;            /* engine setup with scenario information */
Scen_Info *scen_info = 0;
int *weather = 0;       /* weather type for each turn as id to weather_types */
int cur_weather;        /* weather of current turn */
int turn;               /* current turn id */
List *units = 0;        /* active units */
List *vis_units = 0;    /* all units spotted by current player */
List *avail_units = 0;  /* available units for current player to deploy */
List *reinf = 0;        /* reinforcements -- units with a delay (if delay is 0
                           a unit is made available in avail_units if the player
                           controls it. */
Player *player = 0;     /* current player */
int log_x, log_y;       /* position where to draw log info */
char *scen_domain;	/* domain this scenario was loaded under */
/* VICTORY CONDITIONS */
char scen_result[64] = "";  /* the scenario result is saved here */
char scen_message[128] = ""; /* the final scenario message is saved here */
int vcond_check_type = 0;   /* test victory conditions this turn */
VCond *vconds = 0;          /* victory conditions */
int vcond_count = 0;
int cheat_endscn = 0;       /* variable used to determine end scenario by cheat code */
int *casualties;	/* sum of casualties grouped by unit class and player */

/*
====================================================================
Locals
====================================================================
*/
int zones_weather_table[4][12][4] = {
{
//Zone 0 - Desert
//======
//AvgClearPeriod   AvgOvercastPeriod   ProbOfSnow   ProbOfPrecip */
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0},
    {12,               2,                   0,            0}
},
{
//Zone 1 - Mediterranean
//======
//AvgClearPeriod   AvgOvercastPeriod   ProbOfSnow   ProbOfPrecip */
    {5,                5,                   95,           60},
    {5,                5,                   95,           50},
    {10,               5,                   35,           40},
    {10,               5,                   0,            30},
    {12,               5,                   0,            20},
    {12,               4,                   0,            20},
    {12,               4,                   0,            20},
    {12,               4,                   0,            20},
    {12,               5,                   5,            20},
    {10,               5,                   5,            35},
    {8,                5,                   35,           50},
    {5,                5,                   75,           60}
},
{
//Zone 2 - Northern Europe
//======
//AvgClearPeriod   AvgOvercastPeriod   ProbOfSnow   ProbOfPrecip
    {5,                5,                   60,           20},
    {5,                5,                   60,           30},
    {5,                5,                   15,           50},
    {5,                5,                   5,            60},
    {10,               5,                   0,            50},
    {12,               3,                   0,            20},
    {12,               3,                   0,            20},
    {12,               3,                   0,            20},
    {10,               5,                   0,            20},
    {5,                5,                   10,           20},
    {5,                5,                   100,           20},
    {5,                14,                  100,          75}
},
{
//Zone 3 -  Eastern Europe
//======
//AvgClearPeriod   AvgOvercastPeriod   ProbOfSnow   ProbOfPrecip
    {5,                5,                   100,          60},
    {3,                6,                   100,          60},
    {5,                5,                   80,           50},
    {5,                5,                   60,           40},
    {10,               5,                   0,            20},
    {12,               4,                   0,            20},
    {12,               4,                   0,            20},
    {12,               4,                   0,            20},
    {10,               5,                   0,            20},
    {5,                5,                   80,           40},
    {5,                5,                   95,           50},
    {5,                5,                   100,          60}
} };

/*
====================================================================
Check if subcondition is fullfilled.
====================================================================
*/
static int subcond_check( VSubCond *cond )
{
    Unit *unit;
    int x,y,count;
    if ( cond == 0 ) return 0;
    switch ( cond->type ) {
        case VSUBCOND_CTRL_ALL_HEXES:
            for ( x = 0; x < map_w; x++ )
                for ( y = 0; y < map_h; y++ )
                    if ( map[x][y].player && map[x][y].obj )
                        if ( !player_is_ally( cond->player, map[x][y].player ) )
                            return 0;
            return 1;
        case VSUBCOND_CTRL_HEX:
            if ( map[cond->x][cond->y].player )
                if ( player_is_ally( cond->player, map[cond->x][cond->y].player ) )
                    return 1;
            return 0;
        case VSUBCOND_TURNS_LEFT:
            if ( scen_info->turn_limit - turn > cond->count )
                return 1;
            else
                return 0;
        case VSUBCOND_CTRL_HEX_NUM:
            count = 0;
            for ( x = 0; x < map_w; x++ )
                for ( y = 0; y < map_h; y++ )
                    if ( map[x][y].player && map[x][y].obj )
                        if ( player_is_ally( cond->player, map[x][y].player ) )
                            count++;
            if ( count >= cond->count )
                return 1;
            return 0;
        case VSUBCOND_UNITS_KILLED:
            /* as long as there are units out using this tag this condition
               is not fullfilled */
            list_reset( units );
            while ( ( unit = list_next( units ) ) )
                if ( !unit->killed && !player_is_ally( cond->player, unit->player ) )
                    if ( unit->tag[0] != 0 && STRCMP( unit->tag, cond->tag ) )
                        return 0;
            return 1;
        case VSUBCOND_UNITS_SAVED:
            /* we counted the number of units with this tag so count again
               and if one is missing: bingo */
            list_reset( units ); count = 0;
            while ( ( unit = list_next( units ) ) )
                if ( player_is_ally( cond->player, unit->player ) )
                    if ( unit->tag[0] != 0 && STRCMP( unit->tag, cond->tag ) )
                        count++;
            if ( count == cond->count )
                return 1;
            return 0;
    }
    return 0;
}

/** Set no_purchase for all nations that either have no purchasable
 * units in current unit library or have neither flag nor unit under
 * control (thus are not present in this scenario). */
void update_nations_purchase_flag()
{
	int i, x, y;
	Unit_Lib_Entry *e = NULL;
	Unit *u = NULL;
	
	/* clear flag -- misuse it for checks below */
	for (i = 0; i < nation_count; i++)
		nations[i].no_purchase = 0;
	
	/* mark all nations that have at least one entry assigned */
	list_reset( unit_lib );
	while ((e = list_next( unit_lib )))
		if (e->nation != -1)
			nations[e->nation].no_purchase |= 0x01;
		
	/* mark all nations that have at least one unit/flag under control */
	list_reset( units );
	while ((u = list_next( units )))
		u->nation->no_purchase |= 0x02;
	list_reset( reinf );
	while ((u = list_next( reinf )))
		u->nation->no_purchase |= 0x02;
	for ( x = 0; x < map_w; x++ )
		for ( y = 0; y < map_h; y++ )
			if ( map[x][y].nation )
				map[x][y].nation->no_purchase |= 0x02;
			
	/* map results - purchase okay if both flags set */
	for (i = 0; i < nation_count; i++)
		if (nations[i].no_purchase == 0x03)
			nations[i].no_purchase = 0;
		else 
			nations[i].no_purchase = 1;
}

/*
====================================================================
Load a scenario in old lgscn format.
====================================================================
*/
int scen_load_lgscn( const char *fname, const char *path )
{
    char log_str[256];
    int unit_ref = 0;
    int i, j, x, y, obj;
    Nation *nation;
    PData *pd, *sub, *flag, *subsub;
    PData *pd_vcond, *pd_bind, *pd_vsubcond;
    List *entries, *values, *flags;
    char *str, *lib, *temp;
    char *domain;
    Player *player = 0;
    Unit_Lib_Entry *unit_prop = 0, *trsp_prop = 0, *land_trsp_prop = 0;
    Unit *unit;
    Unit unit_base; /* used to store values for unit */
    int unit_delayed = 0;
	
    scen_delete();
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    free(scen_domain);
    domain = scen_domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    SDL_FillRect( sdl.screen, 0, 0x0 );
    log_font->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    log_x = 2; log_y = 2;
    sprintf( log_str, tr("*** Loading scenario '%s' ***"), fname );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    /* get scenario info */
    sprintf( log_str, tr("Loading Scenario Info"));
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    scen_info = calloc( 1, sizeof( Scen_Info ) );
    scen_info->fname = strdup( fname );
    scen_info->mod_name = strdup( config.mod_name );
    if ( !parser_get_localized_string( pd, "name", domain, &scen_info->name ) ) goto parser_failure;
    if ( !parser_get_localized_string( pd, "desc", domain, &scen_info->desc ) ) goto parser_failure;
    if ( !parser_get_localized_string( pd, "authors", domain, &scen_info->authors) ) goto parser_failure;
    if ( !parser_get_value( pd, "date", &str, 0 ) ) goto parser_failure;
    str_to_date( &scen_info->start_date, str );
    if ( !parser_get_int( pd, "turns", &scen_info->turn_limit ) ) goto parser_failure;
    if ( !parser_get_int( pd, "turns_per_day", &scen_info->turns_per_day ) ) goto parser_failure;
    if ( !parser_get_int( pd, "days_per_turn", &scen_info->days_per_turn ) ) goto parser_failure;
    if ( !parser_get_entries( pd, "players", &entries ) ) goto parser_failure;
    scen_info->player_count = entries->count;
    /* nations */
    temp = calloc( 256, sizeof(char) );
    sprintf( log_str, tr("Loading Nations") );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    if ( !parser_get_value( pd, "nation_db", &str, 0 ) ) goto parser_failure;
    if ( !nations_load( str ) ) goto failure;
    /* unit libs */
    if ( !parser_get_pdata( pd, "unit_db", &sub ) ) {
        /* check the scenario file itself but only for the main entry */
        sprintf(str, "../scenarios/%s", fname); 
        sprintf( log_str, tr("Loading Main Unit Library '%s'"), str );
        write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
        if ( !unit_lib_load( str, UNIT_LIB_MAIN ) ) goto parser_failure;
    }
    else {
        if ( !parser_get_value( sub, "main", &str, 0 ) ) goto parser_failure;
        sprintf( log_str, tr("Loading Main Unit Library '%s'"), str );
        write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
        if ( !unit_lib_load( str, UNIT_LIB_MAIN ) ) goto parser_failure;
        if ( parser_get_values( sub, "add", &values ) ) {
            list_reset( values );
            while ( ( lib = list_next( values ) ) ) {
                sprintf( log_str, tr("Loading Additional Unit Library '%s'"), lib );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                if ( !unit_lib_load( lib, UNIT_LIB_ADD ) )
                    goto failure;
            }
        }
    }
    /* map and weather */
    if ( !parser_get_value( pd, "map", &str, 0 ) ) {
        sprintf(str, "../scenarios/%s", fname); /* check the scenario file itself */
    }
    sprintf( log_str, tr("Loading Map '%s'"), str );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    if ( !map_load( str ) ) goto failure;
    if ( !parser_get_int( pd, "weather_zone", &scen_info->weather_zone ) ) goto parser_failure;
    weather = calloc( scen_info->turn_limit, sizeof( int ) );
    if ( config.weather == OPTION_WEATHER_OFF )
        for ( i = 0; i < scen_info->turn_limit; i++ )
            weather[i] = 0;
    else
    {
        if ( config.weather == OPTION_WEATHER_PREDETERMINED )
        {
            if ( parser_get_values( pd, "weather", &values ) ) {
                list_reset( values ); i = 0;
                while ( ( str = list_next( values ) ) ) {
                    if ( i == scen_info->turn_limit ) break;
                    for ( j = 0; j < weather_type_count; j++ )
                        if ( STRCMP( str, weather_types[j].id ) ) {
                            weather[i] = j;
                            break;
                        }
                    i++;
                }
            }
        }
        else
        {
            int water_level, start_weather;
            if ( parser_get_values( pd, "weather", &values ) ) {
                list_reset( values ); i = 0;
                str = list_next( values );
                for ( j = 0; j < weather_type_count; j++ )
                    if ( STRCMP( str, weather_types[j].id ) )
                    {
                        start_weather = j;
                        break;
                    }
            }
            if ( j > 3 )
                water_level = 5;
            else
                water_level = 0;
            scen_create_random_weather( water_level, start_weather );
        }
    }
    /* players */
    sprintf( log_str, tr("Loading Players") );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    if ( !parser_get_entries( pd, "players", &entries ) ) goto parser_failure;
    list_reset( entries );
    while ( ( sub = list_next( entries ) ) ) {
        /* create player */
        player = calloc( 1, sizeof( Player ) );
	    
        player->id = strdup( sub->name );
        if ( !parser_get_localized_string( sub, "name", domain, &player->name ) ) goto parser_failure;
        if ( !parser_get_values( sub, "nations", &values ) ) goto parser_failure;
        player->nation_count = values->count;
        player->nations = calloc( player->nation_count, sizeof( Nation* ) );
        list_reset( values );
        for ( i = 0; i < values->count; i++ )
            player->nations[i] = nation_find( list_next( values ) );
	
        /* unit limit (0 = no limit) */
        parser_get_int(sub, "unit_limit", &player->unit_limit);

        if ((camp_loaded != NO_CAMPAIGN) && config.use_core_units)
        {
            parser_get_int(sub, "core_limit", &player->core_limit);
        }
        else
            player->core_limit = -1;
	
        if ( !parser_get_value( sub, "orientation", &str, 0 ) ) goto parser_failure;
        if ( STRCMP( str, "right" ) ) /* alldirs not implemented yet */
            player->orient = UNIT_ORIENT_RIGHT;
        else
            player->orient = UNIT_ORIENT_LEFT;
        if ( !parser_get_value( sub, "control", &str, 0 ) ) goto parser_failure;
        if ( STRCMP( str, "cpu" ) )
        {
            player->ctrl = PLAYER_CTRL_CPU;
            player->core_limit = -1;
        }
        else
            player->ctrl = PLAYER_CTRL_HUMAN;
        if ( !parser_get_string( sub, "ai_module", &player->ai_fname ) )
            player->ai_fname = strdup( "default" );
        if ( !parser_get_int( sub, "strategy", &player->strat ) ) goto parser_failure;
        if ( !parser_get_int( sub, "strength_row", &player->strength_row ) ) goto parser_failure;
        if ( parser_get_pdata( sub, "transporters/air", &subsub ) ) {
            if ( !parser_get_value( subsub, "unit", &str, 0 ) ) goto parser_failure;
            player->air_trsp = unit_lib_find( str );
            if ( !parser_get_int( subsub, "count", &player->air_trsp_count ) ) goto parser_failure;
        }
        if ( parser_get_pdata( sub, "transporters/sea", &subsub ) ) {
            if ( !parser_get_value( subsub, "unit", &str, 0 ) ) goto parser_failure;
            player->sea_trsp = unit_lib_find( str );
            if ( !parser_get_int( subsub, "count", &player->sea_trsp_count ) ) goto parser_failure;
        }
	/* Load prestige info if any. If none is present show a warning. */ 
	player->prestige_per_turn = calloc( scen_info->turn_limit, sizeof(int));
	if (!player->prestige_per_turn) {
		fprintf( stderr, tr("Out of memory\n") );
		goto failure;
	}
	if ( parser_get_values( sub, "prestige", &values ) ) {
		list_reset( values ); i = 0;
		while ( ( str = list_next( values ) ) ) {
			player->prestige_per_turn[i] = atoi(str);
			i++;
		}
	} else
		fprintf( stderr, tr("Oops: No prestige info for player %s "
					"in scenario %s.\nMaybe you did not "
					"convert scenarios after update?\n"), 
					player->name, scen_info->name );
		player->cur_prestige = 0; /* will be adjusted on turn begin */
	    
		player->uber_units = 0;
        player->force_retreat = 0;
        player_add( player ); player = 0;
    }
    /* flip icons if scenario demands it */
    adjust_fixed_icon_orientation();
    /* set alliances */
    list_reset( players );
    for ( i = 0; i < players->count; i++ ) {
        player = list_next( players );
        player->allies = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
        sprintf( temp, "players/%s/allied_players", player->id );
        if ( parser_get_values( pd, temp, &values ) ) {
            list_reset( values );
            for ( j = 0; j < values->count; j++ )
                list_add( player->allies, player_get_by_id( list_next( values ) ) );
        }
    }
    /* flags */
    sprintf( log_str, tr("Loading Flags") );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    if ( !parser_get_entries( pd, "flags", &flags ) ) goto parser_failure;
    list_reset( flags );
    while ( ( flag = list_next( flags ) ) ) {
        if ( !parser_get_int( flag, "x", &x ) ) goto parser_failure;
        if ( !parser_get_int( flag, "y", &y ) ) goto parser_failure;
        obj = 0; parser_get_int( flag, "obj", &obj );
        if ( !parser_get_value( flag, "nation", &str, 0 ) ) goto parser_failure;
        nation = nation_find( str );
        map[x][y].nation = nation;
        map[x][y].player = player_get_by_nation( nation );
        if ( map[x][y].nation )
            map[x][y].deploy_center = 1;
        map[x][y].obj = obj;
    }
    /* victory conditions */
    scen_result[0] = 0;
    sprintf( log_str, tr("Loading Victory Conditions") );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    /* check type */
    vcond_check_type = VCOND_CHECK_EVERY_TURN;
    if ( parser_get_value( pd, "result/check", &str, 0 ) ) {
        if ( STRCMP( str, "last_turn" ) )
            vcond_check_type = VCOND_CHECK_LAST_TURN;
    }
    if ( !parser_get_entries( pd, "result", &entries ) ) goto parser_failure;
    /* reset vic conds may not be done in scen_delete() as this is called
       before the check */
    scen_result[0] = 0;
    scen_message[0] = 0;
    /* count conditions */
    list_reset( entries ); i = 0;
    while ( ( pd_vcond = list_next( entries ) ) )
        if ( STRCMP( pd_vcond->name, "cond" ) ) i++;
    vcond_count = i + 1;
    /* create conditions */
    vconds = calloc( vcond_count, sizeof( VCond ) );
    list_reset( entries ); i = 1; /* the very first condition is the else condition */
    while ( ( pd_vcond = list_next( entries ) ) ) {
        if ( STRCMP( pd_vcond->name, "cond" ) ) {
            /* result & message */
            if ( parser_get_value( pd_vcond, "result", &str, 0 ) )
                strcpy_lt( vconds[i].result, str, 63 );
            else
                strcpy( vconds[i].result, "undefined" );
            if ( parser_get_value( pd_vcond, "message", &str, 0 ) )
                strcpy_lt( vconds[i].message, trd(domain, str), 127 );
            else
                strcpy( vconds[i].message, "undefined" );
            /* and linkage */
            if ( parser_get_pdata( pd_vcond, "and", &pd_bind ) ) {
                vconds[i].sub_and_count = pd_bind->entries->count;
                /* create subconditions */
                vconds[i].subconds_and = calloc( vconds[i].sub_and_count, sizeof( VSubCond ) );
                list_reset( pd_bind->entries ); j = 0;
                while ( ( pd_vsubcond = list_next( pd_bind->entries ) ) ) {
                    /* get subconds */
                    if ( STRCMP( pd_vsubcond->name, "control_all_hexes" ) ) {
                        vconds[i].subconds_and[j].type = VSUBCOND_CTRL_ALL_HEXES;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_and[j].player = player_get_by_id( str );
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "control_hex" ) ) {
                        vconds[i].subconds_and[j].type = VSUBCOND_CTRL_HEX;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_and[j].player = player_get_by_id( str );
                        if ( !parser_get_int( pd_vsubcond, "x", &vconds[i].subconds_and[j].x ) ) goto parser_failure;
                        if ( !parser_get_int( pd_vsubcond, "y", &vconds[i].subconds_and[j].y ) ) goto parser_failure;
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "turns_left" ) ) {
                        vconds[i].subconds_and[j].type = VSUBCOND_TURNS_LEFT;
                        if ( !parser_get_int( pd_vsubcond, "count", &vconds[i].subconds_and[j].count ) ) goto parser_failure;
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "control_hex_num" ) ) {
                        vconds[i].subconds_and[j].type = VSUBCOND_CTRL_HEX_NUM;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_and[j].player = player_get_by_id( str );
                        if ( !parser_get_int( pd_vsubcond, "count", &vconds[i].subconds_and[j].count ) ) goto parser_failure;
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "units_killed" ) ) {
                        vconds[i].subconds_and[j].type = VSUBCOND_UNITS_KILLED;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_and[j].player = player_get_by_id( str );
                        if ( !parser_get_value( pd_vsubcond, "tag", &str, 0 ) ) goto parser_failure;
                        strcpy_lt( vconds[i].subconds_and[j].tag, str, 31 );
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "units_saved" ) ) {
                        vconds[i].subconds_and[j].type = VSUBCOND_UNITS_SAVED;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_and[j].player = player_get_by_id( str );
                        if ( !parser_get_value( pd_vsubcond, "tag", &str, 0 ) ) goto parser_failure;
                        strcpy_lt( vconds[i].subconds_and[j].tag, str, 31 );
                        vconds[i].subconds_and[j].count = 0; /* units will be counted */
                    }
                    j++;
                }
            }
            /* or linkage */
            if ( parser_get_pdata( pd_vcond, "or", &pd_bind ) ) {
                vconds[i].sub_or_count = pd_bind->entries->count;
                /* create subconditions */
                vconds[i].subconds_or = calloc( vconds[i].sub_or_count, sizeof( VSubCond ) );
                list_reset( pd_bind->entries ); j = 0;
                while ( ( pd_vsubcond = list_next( pd_bind->entries ) ) ) {
                    /* get subconds */
                    if ( STRCMP( pd_vsubcond->name, "control_all_hexes" ) ) {
                        vconds[i].subconds_or[j].type = VSUBCOND_CTRL_ALL_HEXES;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_or[j].player = player_get_by_id( str );
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "control_hex" ) ) {
                        vconds[i].subconds_or[j].type = VSUBCOND_CTRL_HEX;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_or[j].player = player_get_by_id( str );
                        if ( !parser_get_int( pd_vsubcond, "x", &vconds[i].subconds_or[j].x ) ) goto parser_failure;
                        if ( !parser_get_int( pd_vsubcond, "y", &vconds[i].subconds_or[j].y ) ) goto parser_failure;
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "turns_left" ) ) {
                        vconds[i].subconds_or[j].type = VSUBCOND_TURNS_LEFT;
                        if ( !parser_get_int( pd_vsubcond, "count", &vconds[i].subconds_or[j].count ) ) goto parser_failure;
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "control_hex_num" ) ) {
                        vconds[i].subconds_or[j].type = VSUBCOND_CTRL_HEX_NUM;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_or[j].player = player_get_by_id( str );
                        if ( !parser_get_int( pd_vsubcond, "count", &vconds[i].subconds_or[j].count ) ) goto parser_failure;
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "units_killed" ) ) {
                        vconds[i].subconds_or[j].type = VSUBCOND_UNITS_KILLED;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_or[j].player = player_get_by_id( str );
                        if ( !parser_get_value( pd_vsubcond, "tag", &str, 0 ) ) goto parser_failure;
                        strcpy_lt( vconds[i].subconds_or[j].tag, str, 31 );
                    }
                    else
                    if ( STRCMP( pd_vsubcond->name, "units_saved" ) ) {
                        vconds[i].subconds_or[j].type = VSUBCOND_UNITS_SAVED;
                        if ( !parser_get_value( pd_vsubcond, "player", &str, 0 ) ) goto parser_failure;
                        vconds[i].subconds_or[j].player = player_get_by_id( str );
                        if ( !parser_get_value( pd_vsubcond, "tag", &str, 0 ) ) goto parser_failure;
                        strcpy_lt( vconds[i].subconds_or[j].tag, str, 31 );
                        vconds[i].subconds_or[j].count = 0; /* units will be counted */
                    }
                    j++;
                }
            }
            /* no sub conditions at all? */
            if ( vconds[i].sub_and_count == 0 && vconds[i].sub_or_count == 0 ) {
                fprintf( stderr, tr("No subconditions specified!\n") );
                goto failure;
            }
            /* next condition */
            i++;
        }
    }
    /* else condition (used if no other condition is fullfilled and scenario ends) */
    strcpy( vconds[0].result, "defeat" );
    strcpy( vconds[0].message, tr("Defeat") );
    if ( parser_get_pdata( pd, "result/else", &pd_vcond ) ) {
        if ( parser_get_value( pd_vcond, "result", &str, 0 ) )
            strcpy_lt( vconds[0].result, str, 63 );
        if ( parser_get_value( pd_vcond, "message", &str, 0 ) )
            strcpy_lt( vconds[0].message, trd(domain, str), 127 );
    }
    /* units */
    sprintf( log_str, tr("Loading Units") );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    units = list_create( LIST_AUTO_DELETE, unit_delete );
    /* load core units from earlier scenario or create reinf list - unit status is checked elsewhere */
    if (camp_loaded > 1 && config.use_core_units)
    {
        Unit *entry;
        list_reset( reinf );
        while ( ( entry = list_next( reinf ) ) )
            if ( (camp_loaded == CONTINUE_CAMPAIGN) || ((camp_loaded == RESTART_CAMPAIGN) && entry->core == STARTING_CORE) )
            {
                if ( ( entry->nation = nation_find_by_id( entry->prop.nation ) ) == 0 ) {
                    fprintf( stderr, tr("%s: not a nation\n"), str );
                    goto failure;
                }
                if ( ( entry->player = player_get_by_nation( entry->nation ) ) == 0 ) {
                    fprintf( stderr, tr("%s: no player controls this nation\n"), entry->nation->name );
                    goto failure;
                }
                entry->orient = entry->player->orient;
                entry->core = STARTING_CORE;
                unit_reset_attributes(entry);
                if (camp_loaded != NO_CAMPAIGN)
                {
                    unit = unit_duplicate( entry,0 );
                    unit->killed = 2;
                /* put unit to available units list */
                    list_add( units, unit );
                }
                entry->core = CORE;
            }
    }
    else
        reinf = list_create( LIST_AUTO_DELETE, unit_delete );
    avail_units = list_create( LIST_AUTO_DELETE, unit_delete );
    vis_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    if ( !parser_get_entries( pd, "units", &entries ) ) goto parser_failure;
    list_reset( entries );
    int auxiliary_units_count = 0;
    while ( ( sub = list_next( entries ) ) ) {
        /* unit type */
        if ( !parser_get_value( sub, "id", &str, 0 ) ) goto parser_failure;
        if ( ( unit_prop = unit_lib_find( str ) ) == 0 ) {
            fprintf( stderr, tr("%s: unit entry not found\n"), str );
            goto failure;
        }
        memset( &unit_base, 0, sizeof( Unit ) );
        /* nation & player */
        if ( !parser_get_value( sub, "nation", &str, 0 ) ) goto parser_failure;
        if ( ( unit_base.nation = nation_find( str ) ) == 0 ) {
            fprintf( stderr, tr("%s: not a nation\n"), str );
            goto failure;
        }
        if ( ( unit_base.player = player_get_by_nation( unit_base.nation ) ) == 0 ) {
            fprintf( stderr, tr("%s: no player controls this nation\n"), unit_base.nation->name );
            goto failure;
        }
        /* name */
        unit_set_generic_name( &unit_base, unit_ref + 1, unit_prop->name );
        /* delay */
        unit_delayed = parser_get_int( sub, "delay", &unit_base.delay );
        /* position */
        if ( !parser_get_int( sub, "x", &unit_base.x ) && !unit_delayed ) goto parser_failure;
        if ( !parser_get_int( sub, "y", &unit_base.y ) && !unit_delayed ) goto parser_failure;
        if ( !unit_delayed && ( unit_base.x <= 0 || unit_base.y <= 0 || unit_base.x >= map_w - 1 || unit_base.y >= map_h - 1 ) ) {
            fprintf( stderr, tr("%s: out of map: ignored\n"), unit_base.name );
            continue;  
        }
        /* strength, entrenchment, experience */
        if ( !parser_get_int( sub, "str", &unit_base.str ) ) goto parser_failure;
        if (unit_base.str > 10 )
            unit_base.max_str = 10;
        else
            unit_base.max_str = unit_base.str;
        if ( !parser_get_int( sub, "entr", &unit_base.entr ) ) goto parser_failure;
        if ( !parser_get_int( sub, "exp", &unit_base.exp_level ) ) goto parser_failure;
        /* transporter */
        trsp_prop = 0;
        if ( parser_get_value( sub, "trsp", &str, 0 ) && !STRCMP( str, "none" ) )
            trsp_prop = unit_lib_find( str );
        land_trsp_prop = 0;
        if ( parser_get_value( sub, "land_trsp", &str, 0 ) && !STRCMP( str, "none" ) )
            land_trsp_prop = unit_lib_find( str );
        /* core */
        if ( !parser_get_int( sub, "core", &unit_base.core ) ) goto parser_failure;
        if ( !config.use_core_units )
            unit_base.core = 0;
        /* orientation */
        unit_base.orient = unit_base.player->orient;
        /* tag if set */
        unit_base.tag[0] = 0;
        if ( parser_get_value( sub, "tag", &str, 0 ) ) {
            strcpy_lt( unit_base.tag, str, 31 );
            /* check all subconds for UNITS_SAVED and increase the counter
               if this unit is allied */
            for ( i = 1; i < vcond_count; i++ ) {
                for ( j = 0; j < vconds[i].sub_and_count; j++ )
                    if ( vconds[i].subconds_and[j].type == VSUBCOND_UNITS_SAVED )
                        if ( STRCMP( unit_base.tag, vconds[i].subconds_and[j].tag ) )
                            vconds[i].subconds_and[j].count++;
                for ( j = 0; j < vconds[i].sub_or_count; j++ )
                    if ( vconds[i].subconds_or[j].type == VSUBCOND_UNITS_SAVED )
                        if ( STRCMP( unit_base.tag, vconds[i].subconds_or[j].tag ) )
                            vconds[i].subconds_or[j].count++;
            }
        }
        /* actual unit */
        if ( !(unit_base.player->ctrl == PLAYER_CTRL_HUMAN && auxiliary_units_count >= 
               unit_base.player->unit_limit - unit_base.player->core_limit) ||
               ( (camp_loaded != NO_CAMPAIGN) && STRCMP(camp_cur_scen->id, camp_first) && config.use_core_units ) )
        {
            unit = unit_create( unit_prop, trsp_prop, land_trsp_prop, &unit_base );
            /* put unit to active or reinforcements list or available units list */
            if ( !unit_delayed ) {
                list_add( units, unit );
                /* add unit to map */
                map_insert_unit( unit );
            }
        }
        else if (config.purchase == NO_PURCHASE) /* no fixed reinfs with purchase enabled */
            list_add( reinf, unit );
        /* adjust transporter count */
        if ( unit->embark == EMBARK_SEA ) {
            unit->player->sea_trsp_count++;
            unit->player->sea_trsp_used++;
        }
        else
            if ( unit->embark == EMBARK_AIR ) {
                unit->player->air_trsp_count++;
                unit->player->air_trsp_used++;
            }
        unit_ref++;
        if (unit_base.player->ctrl == PLAYER_CTRL_HUMAN)
            auxiliary_units_count++;
    }
    /* load deployment hexes */
    if ( parser_get_entries( pd, "deployfields", &entries ) ) {
        list_reset( entries );
        while ( ( sub = list_next( entries ) ) ) {
            Player *pl;
            int plidx;
            if ( !parser_get_value( sub, "id", &str, 0 ) ) goto parser_failure;
            pl = player_get_by_id( str );
            if ( !pl ) goto parser_failure;
            plidx = player_get_index( pl );
            if ( parser_get_values( sub, "coordinates", &values ) && values->count>0 )
            {
                list_reset( values );
                while ( ( lib = list_next( values ) ) ) {
                    if (!strcmp(lib,"default"))
                        { x=-1; y=-1; }
                    else if (!strcmp(lib,"none"))
                        { pl->no_init_deploy = 1; continue; }
                    else
                        get_coord( lib, &x, &y );
                    map_set_deploy_field( x, y, plidx );
                }
            }
        }
    }
    parser_free( &pd );

    casualties = calloc( scen_info->player_count * unit_class_count, sizeof casualties[0] );
    deploy_turn = config.deploy_turn;
    
    /* check which nations may do purchase for this scenario */
    update_nations_purchase_flag();
    
    return 1;
parser_failure:        
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    terrain_delete();
    scen_delete();
    if ( player ) player_delete( player );
    if ( pd ) parser_free( &pd );
    return 0;
}

/*
====================================================================
Load a scenario in new lgdscn format.
====================================================================
*/
int scen_load_lgdscn( const char *fname, const char *path )
{
    char log_str[256];
    int unit_ref = 0;
    int i, j, x, y;
    Nation *nation;
    Unit_Lib_Entry *unit_prop = 0, *trsp_prop = 0, *land_trsp_prop = 0;
    Unit *unit;
    Unit unit_base; /* used to store values for unit */
    int unit_delayed = 0;

    FILE *inf;
    char line[MAX_SCENARIO_LINE],tokens[MAX_TOKENS][256];
    int block=0,last_line_length=-1,cursor=0,token=0;
    int utf16 = 0, lines=0, cur_player_count = 0, cur_flag = 0, cur_vic_cond = 0, cur_unit = 0, auxiliary_units_count = 0;
    vcond_count = -1;

    scen_delete();
    /* reset vic conds may not be done in scen_delete() as this is called
       before the check */
    scen_result[0] = 0;
    scen_message[0] = 0;

    inf=fopen(path,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open scenario file\n");
        return 0;
    }

    free(scen_domain);
    SDL_FillRect( sdl.screen, 0, 0x0 );
    log_font->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    log_x = 2; log_y = 2;
    sprintf( log_str, tr("*** Loading scenario '%s' ***"), fname );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );

    scen_info = calloc( 1, sizeof( Scen_Info ) );
    scen_info->fname = strdup( fname );
    scen_info->mod_name = strdup( config.mod_name );
    scen_info->player_count = 0;

    //find number of players
    while (load_line(inf,line,utf16)>=0)
    {
        //strip comments
        if (line[0]==0x23 || line[0]==0x09) { line[0]=0; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        if (block == 2 && strlen(line)>0)
        {
            scen_info->player_count++;
        }

        if (block == 7 && strlen(line)>0)
        {
            vcond_count++;
        }

        if (block == 8)
        {
            fclose(inf);
            break;
        }
    }
    vconds = calloc( vcond_count, sizeof( VCond ) );

    block=0;last_line_length=-1;cursor=0;token=0;lines=0;
    inf=fopen(path,"rb");

    while (load_line(inf,line,utf16)>=0)
    {
        lines++;
        //strip comments
        if (line[0]==0x23 || line[0]==0x09) { line[0]=0; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                if ( line[i + 1] != 0x09 )
                {
                    token++;
                    cursor=0;
                }
                else
                    break;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;

        //Block#1 general scenario data
        if (block == 1 && strlen(line)>0)
        {
            if ( strcmp( tokens[0], "name" ) == 0 )
                scen_info->name = strdup( tokens[1] );
            if ( strcmp( tokens[0], "desc" ) == 0 )
                scen_info->desc = strdup( tokens[1] );
            if ( strcmp( tokens[0], "turns" ) == 0 )
                scen_info->turn_limit = atoi( tokens[1] );
            if ( strcmp( tokens[0], "authors" ) == 0 )
                scen_info->authors = strdup( tokens[1] );
            if (strcmp(tokens[0],"year")==0)
                scen_info->start_date.year=atoi(tokens[1]);
            if (strcmp(tokens[0],"month")==0)
                scen_info->start_date.month=atoi(tokens[1]) - 1;
            if (strcmp(tokens[0],"day")==0)
                scen_info->start_date.day=atoi(tokens[1]);
            if (strcmp(tokens[0],"turns_per_day")==0)
                scen_info->turns_per_day = atoi( tokens[1] );
            if (strcmp(tokens[0],"days_per_turn")==0)
                scen_info->days_per_turn = atoi( tokens[1] );;

            if ( strcmp( tokens[0], "nation_db" ) == 0 )
            {
                /* nations */
                sprintf( log_str, tr("Loading Nations") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                if ( !nations_load( tokens[1] ) ) goto failure;
            }

            if ( strcmp( tokens[0], "unit_db" ) == 0 )
            {
                /* unit lib */
                sprintf( log_str, tr("Loading Main Unit Library '%s'"), tokens[1] );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                if ( !unit_lib_load( tokens[1], UNIT_LIB_MAIN ) )
                    return 0;
            }

            if ( strcmp( tokens[0], "map file" ) == 0 )
            {
                /* map */
                sprintf( log_str, tr("Loading Map '%s'"), tokens[1] );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                if ( !map_load( tokens[1] ) ) goto failure;
            }

            if ( strcmp( tokens[0], "weather_zone" ) == 0 )
            {
                scen_info->weather_zone = atoi( tokens[1] );
            }

            if ( strcmp( tokens[0], "weather" ) == 0 )
            {
                /* weather */
                weather = calloc( scen_info->turn_limit, sizeof( int ) );
                if ( config.weather == OPTION_WEATHER_OFF )
                    for ( i = 0; i < scen_info->turn_limit; i++ )
                        weather[i] = 0;
                else
                {
                    if ( config.weather == OPTION_WEATHER_PREDETERMINED )
                    {
                        for ( i = 1; i <= token; i++ )
                        {
                            if ( i - 1 == scen_info->turn_limit ) break;
                            for ( j = 0; j < weather_type_count; j++ )
                                if ( strcmp( tokens[i], weather_types[j].id ) == 0 ) {
                                    weather[i - 1] = j;
                                    break;
                                }
                        }
                    }
                    else
                    {
                        int water_level, start_weather;
                        for ( j = 0; j < weather_type_count; j++ )
                        {
                            if ( strcmp( tokens[1], weather_types[j].id ) == 0 )
                            {
                                start_weather = j;
                                break;
                            }
                        }
                        if ( j > 3 )
                            water_level = 5;
                        else
                            water_level = 0;
                        scen_create_random_weather( water_level, start_weather );
                    }
                }
            }
        }

        //Block#2 players
        if (block == 2 && strlen(line)>0)
        {
            Player *pl;
            if ( cur_player_count == 0 )
            {
                sprintf( log_str, tr("Loading Players") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
            }
            /* create player */
            pl = calloc( 1, sizeof( Player ) );

            pl->id = strdup( tokens[0] );
            pl->name = strdup( tokens[1] );

            /* unit limit (0 = no limit) */
            pl->unit_limit = atoi( tokens[2] );

            if ((camp_loaded != NO_CAMPAIGN) && config.use_core_units)
            {
                pl->core_limit = atoi( tokens[3] );
            }
            else
                pl->core_limit = -1;

            if ( strcmp( tokens[4], "right" ) == 0 ) /* alldirs not implemented yet */
                pl->orient = UNIT_ORIENT_RIGHT;
            else
                pl->orient = UNIT_ORIENT_LEFT;

            if ( strcmp( tokens[5], "cpu" ) == 0 )
            {
                pl->ctrl = PLAYER_CTRL_CPU;
                pl->core_limit = -1;
            }
            else
                pl->ctrl = PLAYER_CTRL_HUMAN;

            pl->ai_fname = strdup( "default" );
            pl->strat = atoi( tokens[6] );
            pl->strength_row = atoi( tokens[7] );

            pl->air_trsp = unit_lib_find( tokens[8] );
            pl->air_trsp_count = atoi( tokens[9] );
            pl->sea_trsp = unit_lib_find( tokens[10] );
            pl->sea_trsp_count = atoi( tokens[11] );

            player_add( pl );
            cur_player_count++;
        }

        //Block#3 allied players
        if (block == 3 && strlen(line)>0)
        {
            Player *pl;
            pl = player_get_by_id( tokens[0] );
            pl->allies = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
            for ( i = 1; i <= token; i++ )
                list_add( pl->allies, player_get_by_id( tokens[i] ) );
        }

        //Block#4 prestige allocation
        if (block == 4 && strlen(line)>0)
        {
            Player *pl;
            pl = player_get_by_id( tokens[0] );
			pl->prestige_per_turn = calloc( scen_info->turn_limit, sizeof(int));
			if (!pl->prestige_per_turn) {
				fprintf( stderr, tr("Out of memory\n") );
				goto failure;
			}
			for ( i = 1; i <= token; i++ ) {
				pl->prestige_per_turn[i - 1] = atoi( tokens[i] );
			}
			pl->cur_prestige = 0; /* will be adjusted on turn begin */

			pl->uber_units = 0;
            pl->force_retreat = 0;
        }

        //Block#5 player nations
        if (block == 5 && strlen(line)>0)
        {
            Player *pl;
            pl = player_get_by_id( tokens[0] );
            pl->nation_count = token;
            pl->nations = calloc( pl->nation_count, sizeof( Nation* ) );
            for ( i = 1; i <= token; i++ )
                pl->nations[i - 1] = nation_find( tokens[i] );
       }

        //Block#6 flags
        if (block == 6 && strlen(line)>0)
        {
            if ( cur_flag == 0 )
            {
                sprintf( log_str, tr("Loading Flags") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
            }
            x = atoi( tokens[0] );
            y = atoi( tokens[1] );
            nation = nation_find( tokens[2] );
            map[x][y].nation = nation;
            map[x][y].player = player_get_by_nation( nation );
            if ( map[x][y].nation )
                map[x][y].deploy_center = 1;
            map[x][y].obj = atoi( tokens[3] );
            cur_flag++;
        }

        //Block#7 victory conditions
        if (block == 7 && strlen(line)>0)
        {
            if ( strcmp( tokens[0], "vcond_check_type" ) == 0 )
            {
                sprintf( log_str, tr("Loading Victory Conditions") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                /* check type */
                vcond_check_type = VCOND_CHECK_EVERY_TURN;
                if ( strcmp( tokens[1], "last_turn" ) == 0 )
                    vcond_check_type = VCOND_CHECK_LAST_TURN;
            }
            else
            {
                /* result & message */
                strcpy_lt( vconds[cur_vic_cond].result, tokens[0], 63 );
                strcpy_lt( vconds[cur_vic_cond].message, tokens[1], 127 );
                /* and linkage */
                if ( strcmp( tokens[2], "and" ) == 0 )
                {
                    vconds[cur_vic_cond].sub_and_count = (int)token / 6;
                    /* create subconditions */
                    vconds[cur_vic_cond].subconds_and = calloc( vconds[cur_vic_cond].sub_and_count, sizeof( VSubCond ) );
                    for ( j = 0; j < vconds[cur_vic_cond].sub_and_count; j++ ) {
                        /* get subconds */
                        if ( strcmp( tokens[3 + j * 6], "control_all_hexes" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_and[j].type = VSUBCOND_CTRL_ALL_HEXES;
                            vconds[cur_vic_cond].subconds_and[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "control_hex" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_and[j].type = VSUBCOND_CTRL_HEX;
                            vconds[cur_vic_cond].subconds_and[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            vconds[cur_vic_cond].subconds_and[j].x = atoi( tokens[3 + 2 + j * 6] );
                            vconds[cur_vic_cond].subconds_and[j].y = atoi( tokens[3 + 3 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "turns_left" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_and[j].type = VSUBCOND_TURNS_LEFT;
                            vconds[cur_vic_cond].subconds_and[j].count = atoi( tokens[3 + 4 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "control_hex_num" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_and[j].type = VSUBCOND_CTRL_HEX_NUM;
                            vconds[cur_vic_cond].subconds_and[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            vconds[cur_vic_cond].subconds_and[j].count = atoi( tokens[3 + 4 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "units_killed" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_and[j].type = VSUBCOND_UNITS_KILLED;
                            vconds[cur_vic_cond].subconds_and[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            strcpy_lt( vconds[cur_vic_cond].subconds_and[j].tag, tokens[3 + 5 + j * 6], 31 );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "units_saved" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_and[j].type = VSUBCOND_UNITS_SAVED;
                            vconds[cur_vic_cond].subconds_and[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            strcpy_lt( vconds[cur_vic_cond].subconds_and[j].tag, tokens[3 + 5 + j * 6], 31 );
                            vconds[cur_vic_cond].subconds_and[j].count = 0; /* units will be counted */
                        }
                    }
                }
                /* or linkage */
                if ( strcmp( tokens[2], "or" ) == 0 )
                {
                    vconds[cur_vic_cond].sub_or_count = (int)token / 6;
                    /* create subconditions */
                    vconds[cur_vic_cond].subconds_or = calloc( vconds[cur_vic_cond].sub_or_count, sizeof( VSubCond ) );
                    for ( j = 0; j < vconds[cur_vic_cond].sub_or_count; j++ ) {
                        /* get subconds */
                        if ( strcmp( tokens[3 + j * 6], "control_all_hexes" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_or[j].type = VSUBCOND_CTRL_ALL_HEXES;
                            vconds[cur_vic_cond].subconds_or[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "control_hex" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_or[j].type = VSUBCOND_CTRL_HEX;
                            vconds[cur_vic_cond].subconds_or[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            vconds[cur_vic_cond].subconds_or[j].x = atoi( tokens[3 + 2 + j * 6] );
                            vconds[cur_vic_cond].subconds_or[j].y = atoi( tokens[3 + 3 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "turns_left" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_or[j].type = VSUBCOND_TURNS_LEFT;
                            vconds[cur_vic_cond].subconds_or[j].count = atoi( tokens[3 + 4 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "control_hex_num" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_or[j].type = VSUBCOND_CTRL_HEX_NUM;
                            vconds[cur_vic_cond].subconds_or[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            vconds[cur_vic_cond].subconds_or[j].count = atoi( tokens[3 + 4 + j * 6] );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "units_killed" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_or[j].type = VSUBCOND_UNITS_KILLED;
                            vconds[cur_vic_cond].subconds_or[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            strcpy_lt( vconds[cur_vic_cond].subconds_or[j].tag, tokens[3 + 5 + j * 6], 31 );
                        }
                        else
                        if ( strcmp( tokens[3 + j * 6], "units_saved" ) == 0 ) {
                            vconds[cur_vic_cond].subconds_or[j].type = VSUBCOND_UNITS_SAVED;
                            vconds[cur_vic_cond].subconds_or[j].player = player_get_by_id( tokens[3 + 1 + j * 6] );
                            strcpy_lt( vconds[cur_vic_cond].subconds_or[j].tag, tokens[3 + 5 + j * 6], 31 );
                            vconds[cur_vic_cond].subconds_or[j].count = 0; /* units will be counted */
                        }
                    }
                }
                cur_vic_cond++;
            }
        }

        //Block#8 deployment hexes
        if (block == 8 && strlen(line)>0)
        {
            Player *pl;
            int plidx;
            pl = player_get_by_id( tokens[0] );
            plidx = player_get_index( pl );
            if ( strcmp(tokens[1], "default") == 0)
            {
                x=-1;
                y=-1;
            }
            else if ( strcmp(tokens[1], "none") == 0 )
            {
                pl->no_init_deploy = 1;
                continue;
            }
            for ( i = 2; i <= token; i++ )
            {
                get_coord( tokens[i], &x, &y );
                map_set_deploy_field( x, y, plidx );
            }
        }

        //Block#9 units
        if (block == 9 && strlen(line)>0)
        {
            if ( cur_unit == 0 )
            {
                sprintf( log_str, tr("Loading Units") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                units = list_create( LIST_AUTO_DELETE, unit_delete );

                /* load core units from earlier scenario or create reinf list - unit status is checked elsewhere */
                if (camp_loaded > 1 && config.use_core_units)
                {
                    Unit *entry;
                    list_reset( reinf );
                    while ( ( entry = list_next( reinf ) ) )
                        if ( (camp_loaded == CONTINUE_CAMPAIGN) || ((camp_loaded == RESTART_CAMPAIGN) && entry->core == STARTING_CORE) )
                        {
                            if ( ( entry->nation = nation_find_by_id( entry->prop.nation ) ) == 0 ) {
                                fprintf( stderr, tr("%d: not a nation\n"), entry->prop.nation );
                                goto failure;
                            }
                            if ( ( entry->player = player_get_by_nation( entry->nation ) ) == 0 ) {
                                fprintf( stderr, tr("%s: no player controls this nation\n"), entry->nation->name );
                                goto failure;
                            }
                            entry->orient = entry->player->orient;
                            entry->core = STARTING_CORE;
                            unit_reset_attributes(entry);
                            if (camp_loaded != NO_CAMPAIGN)
                            {
                                unit = unit_duplicate( entry,0 );
                                unit->killed = 2;
                                /* put unit to available units list */
                                list_add( units, unit );
                            }
                            entry->core = CORE;
                        }
                }
                else
                    reinf = list_create( LIST_AUTO_DELETE, unit_delete );
                avail_units = list_create( LIST_AUTO_DELETE, unit_delete );
                vis_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
            }
            /* unit type */
            if ( ( unit_prop = unit_lib_find( tokens[0] ) ) == 0 ) {
                fprintf( stderr, tr("%s: unit entry not found\n"), tokens[0] );
                goto failure;
            }
            memset( &unit_base, 0, sizeof( Unit ) );
            /* nation & player */
            if ( ( unit_base.nation = nation_find( tokens[1] ) ) == 0 ) {
                fprintf( stderr, tr("%s: not a nation\n"), tokens[1] );
                goto failure;
            }
            if ( ( unit_base.player = player_get_by_nation( unit_base.nation ) ) == 0 ) {
                fprintf( stderr, tr("%s: no player controls this nation\n"), unit_base.nation->name );
                goto failure;
            }
            /* name */
            unit_set_generic_name( &unit_base, unit_ref + 1, unit_prop->name );
            /* delay */
            unit_base.delay = 0;
            /* position */
            unit_base.x = atoi( tokens[2] );
            unit_base.y = atoi( tokens[3] );
            if ( !unit_delayed && ( unit_base.x <= 0 || unit_base.y <= 0 || unit_base.x >= map_w - 1 || unit_base.y >= map_h - 1 ) ) {
                fprintf( stderr, tr("%s: out of map: ignored\n"), unit_base.name );
                continue;  
            }
            /* strength, entrenchment, experience */
            unit_base.str = atoi( tokens[4] );
            if (unit_base.str > 10 )
                unit_base.max_str = 10;
            else
                unit_base.max_str = unit_base.str;
            unit_base.entr = atoi( tokens[5] );
            unit_base.exp_level = atoi( tokens[6] );
            /* transporter */
            trsp_prop = 0;
            land_trsp_prop = 0;
            if ( strcmp( tokens[7], "none" ) != 0 )
            {
                if ( strcmp( tokens[8], "none" ) != 0 )
                {
                    trsp_prop = unit_lib_find( tokens[8] );
                    land_trsp_prop = unit_lib_find( tokens[7] );
                }
                else
                    trsp_prop = unit_lib_find( tokens[7] );
            }
            else
                if ( strcmp( tokens[8], "none" ) != 0 )
                    trsp_prop = unit_lib_find( tokens[8] );
            /* core */
            unit_base.core = atoi( tokens[9] );
            if ( !config.use_core_units )
                unit_base.core = 0;
            /* orientation */
            unit_base.orient = unit_base.player->orient;
            /* tag if set */
            unit_base.tag[0] = 0;
            if ( strcmp( tokens[10], "0" ) != 0 )
            {
                strcpy_lt( unit_base.tag, tokens[10], 31 );
                /* check all subconds for UNITS_SAVED and increase the counter
                   if this unit is allied */
                for ( i = 1; i < vcond_count; i++ ) {
                    for ( j = 0; j < vconds[i].sub_and_count; j++ )
                        if ( vconds[i].subconds_and[j].type == VSUBCOND_UNITS_SAVED )
                            if ( strcmp( unit_base.tag, vconds[i].subconds_and[j].tag ) == 0 )
                                vconds[i].subconds_and[j].count++;
                    for ( j = 0; j < vconds[i].sub_or_count; j++ )
                        if ( vconds[i].subconds_or[j].type == VSUBCOND_UNITS_SAVED )
                            if ( strcmp( unit_base.tag, vconds[i].subconds_or[j].tag ) == 0 )
                                vconds[i].subconds_or[j].count++;
                }
            }
            /* actual unit */
            if ( !(unit_base.player->ctrl == PLAYER_CTRL_HUMAN && auxiliary_units_count >= 
                   unit_base.player->unit_limit - unit_base.player->core_limit) ||
                   ( (camp_loaded != NO_CAMPAIGN) && STRCMP(camp_cur_scen->id, camp_first) && config.use_core_units ) )
            {
                unit = unit_create( unit_prop, trsp_prop, land_trsp_prop, &unit_base );
                /* put unit to active or reinforcements list or available units list */
                if ( !unit_delayed ) {
                    list_add( units, unit );
                    /* add unit to map */
                    map_insert_unit( unit );
                }
            }
            else if (config.purchase == NO_PURCHASE) /* no fixed reinfs with purchase enabled */
                list_add( reinf, unit );
            /* adjust transporter count */
            if ( unit->embark == EMBARK_SEA ) {
                unit->player->sea_trsp_count++;
                unit->player->sea_trsp_used++;
            }
            else
                if ( unit->embark == EMBARK_AIR ) {
                    unit->player->air_trsp_count++;
                    unit->player->air_trsp_used++;
                }
            unit_ref++;
            if (unit_base.player->ctrl == PLAYER_CTRL_HUMAN)
                auxiliary_units_count++;
            cur_unit++;
        }
    }

    fclose(inf);
    casualties = calloc( scen_info->player_count * unit_class_count, sizeof casualties[0] );
    deploy_turn = config.deploy_turn;

    /* flip icons if scenario demands it */
    adjust_fixed_icon_orientation();
    /* check which nations may do purchase for this scenario */
    update_nations_purchase_flag();
    
    return 1;
failure:
    terrain_delete();
    scen_delete();
    if ( player ) player_delete( player );
    return 0;
}

/*
====================================================================
Load a scenario description in old lgscn format.
====================================================================
*/
char* scen_load_lgscn_info( const char *fname, const char *path )
{
    PData *pd, *sub;
    List *entries;
    char *name, *desc, *turns, count[4], *str;
    char *info = 0;
    char *domain = 0;
    int i;
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    /* title and description */
    domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    if ( !parser_get_value( pd, "name", &name, 0 ) ) goto parser_failure;
    name = (char *)trd(domain, name);
    if ( !parser_get_value( pd, "desc", &desc, 0 ) ) goto parser_failure;
    desc = (char *)trd(domain, desc);
    if ( !parser_get_entries( pd, "players", &entries ) ) goto parser_failure;
    if ( !parser_get_value( pd, "turns", &turns, 0 ) ) goto parser_failure;
    sprintf( count, "%i",  entries->count );
    if ( ( info = calloc( strlen( name ) + strlen( desc ) + strlen( count ) + strlen( turns ) + 30, sizeof( char ) ) ) == 0 ) {
        fprintf( stderr, tr("Out of memory\n") );
        goto failure;
    }
    sprintf( info, tr("%s##%s##%s Turns#%s Players"), name, desc, turns, count );
    /* set setup */
    scen_clear_setup();
    strcpy( setup.fname, fname );
    strcpy( setup.camp_entry_name, "" );
    setup.player_count = entries->count;
    setup.ctrl = calloc( setup.player_count, sizeof( int ) );
    setup.names = calloc( setup.player_count, sizeof( char* ) );
    setup.modules = calloc( setup.player_count, sizeof( char* ) );
    /* load the player ctrls */
    list_reset( entries ); i = 0;
    while ( ( sub = list_next( entries ) ) ) {
        if ( !parser_get_value( sub, "name", &str, 0 ) ) goto parser_failure;
        setup.names[i] = strdup(trd(domain, str));
        if ( !parser_get_value( sub, "control", &str, 0 ) ) goto parser_failure;
        if ( STRCMP( str, "cpu" ) )
            setup.ctrl[i] = PLAYER_CTRL_CPU;
        else
            setup.ctrl[i] = PLAYER_CTRL_HUMAN;
        if ( !parser_get_string( sub, "ai_module", &setup.modules[i] ) )
            setup.modules[i] = strdup( "default" );
        i++;
    }
    parser_free( &pd );
    free(domain);
    return info;
parser_failure:        
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    if ( pd ) parser_free( &pd );
    if ( info ) free( info );
    free(domain);
    return 0;
}

/*
====================================================================
Load a scenario description in new lgdscn format.
====================================================================
*/
char* scen_load_lgdscn_info( const char *fname, const char *path )
{
    int i;
    char *domain = 0;
    char *name, *desc, *turns, count[4];
    char *info = 0;

    FILE *inf;
    char line[MAX_SCENARIO_LINE],tokens[MAX_TOKENS][256];
    int block=0,last_line_length=-1,cursor=0,token=0;
    int utf16 = 0, lines=0, cur_player_count = 0;

    scen_clear_setup();
    strcpy( setup.fname, fname );
    strcpy( setup.camp_entry_name, "" );
    setup.player_count = 0;

    inf=fopen(path,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open scenario file\n");
        return 0;
    }

    scen_clear_setup();
    strcpy( setup.fname, fname );
    strcpy( setup.camp_entry_name, "" );
    //find number of players
    while (load_line(inf,line,utf16)>=0)
    {
        //strip comments
        if (line[0]==0x23 || line[0]==0x09) { line[0]=0; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        if (block == 2 && strlen(line)>0)
        {
            setup.player_count++;
        }

        if (block == 3)
        {
            fclose(inf);
            break;
        }
    }
    setup.ctrl = calloc( setup.player_count, sizeof( int ) );
    setup.names = calloc( setup.player_count, sizeof( char* ) );
    setup.modules = calloc( setup.player_count, sizeof( char* ) );

    block=0;last_line_length=-1;cursor=0;token=0;lines=0;
    inf=fopen(path,"rb");

    while (load_line(inf,line,utf16)>=0)
    {
        lines++;
        //strip comments
        if (line[0]==0x23 || line[0]==0x09) { line[0]=0; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        if (block == 1 && strlen(line)>0)
        {
            if ( strcmp( tokens[0], "name" ) == 0 )
                name = strdup( tokens[1] );
            if ( strcmp( tokens[0], "desc" ) == 0 )
                desc = strdup( tokens[1] );
            if ( strcmp( tokens[0], "turns" ) == 0 )
                turns = strdup( tokens[1] );
        }

        if (block == 2 && strlen(line)>0)
        {
            setup.names[cur_player_count] = strdup( tokens[1] );
            if ( strcmp( tokens[5], "cpu" ) == 0 )
                setup.ctrl[cur_player_count] = PLAYER_CTRL_CPU;
            else
                setup.ctrl[cur_player_count] = PLAYER_CTRL_HUMAN;
            setup.modules[cur_player_count] = strdup( tokens[8] );
            cur_player_count++;
        }

        if (block == 3)
        {
            fclose(inf);
            break;
        }
    }

    sprintf( count, "%i", setup.player_count );
    if ( ( info = calloc( strlen( name ) + strlen( desc ) + strlen( count ) + strlen( turns ) + 30, sizeof( char ) ) ) == 0 ) {
        fprintf( stderr, tr("Out of memory\n") );
        goto failure;
    }
    sprintf( info, tr("%s##%s##%s Turns#%s Players"), name, desc, turns, count );
    return info;
failure:
    if ( info ) free( info );
    free(domain);
    return 0;
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Load a scenario.
====================================================================
*/
int scen_load( char *fname )
{
    char *path, *extension;
    path = calloc( 256, sizeof( char ) );
    extension = calloc( 10, sizeof( char ) );
    search_file_name( path, extension, fname, config.mod_name, "Scenario", 'o' );
    if ( strcmp( extension, "lgscn" ) == 0 )
        return scen_load_lgscn( fname, path );
    else if ( strcmp( extension, "pgscn" ) == 0 )
        return load_pgf_pgscn( fname, path, atoi( fname ) );
    else if ( strcmp( extension, "lgdscn" ) == 0 )
        return scen_load_lgdscn( fname, path );
    return 0;
}

/*
====================================================================
Load a scenario description.
====================================================================
*/
char *scen_load_info( char *fname )
{
    char path[MAX_PATH], extension[10], temp[MAX_PATH];
    snprintf( temp, MAX_PATH, "%s/Scenario", config.mod_name );
    search_file_name( path, extension, fname, config.mod_name, "Scenario", 'o' );
    if ( strcmp( extension, "lgscn" ) == 0 )
    {
        return scen_load_lgscn_info( fname, path );
    }
    else if ( strcmp( extension, "pgscn" ) == 0 )
    {
        return load_pgf_pgscn_info( fname, path, 0 );
    }
    else if ( strcmp( extension, "lgdscn" ) == 0 )
    {
        return scen_load_lgdscn_info( fname, path );
    }
    return 0;
}

/*
====================================================================
Fill the scenario part in 'setup' with the loaded player 
information.
====================================================================
*/
void scen_set_setup()
{
    int i;
    Player *player;
    scen_clear_setup();
    setup.player_count = players->count;
    setup.ctrl = calloc( setup.player_count, sizeof( int ) );
    setup.names = calloc( setup.player_count, sizeof( char* ) );
    setup.modules = calloc( setup.player_count, sizeof( char* ) );
    list_reset( players );
    for ( i = 0; i < setup.player_count; i++ ) {
        player = list_next( players );
        setup.ctrl[i] = player->ctrl;
        setup.names[i] = strdup( player->name );
        setup.modules[i] = strdup( player->ai_fname );
    }
}

/*
====================================================================
Clear the pointers in 'setup' which were loaded by scen_load_info()
====================================================================
*/
void scen_clear_setup()
{
    int i;
    if ( setup.ctrl ) {
        free( setup.ctrl ); 
        setup.ctrl = 0;
    }
    if ( setup.names ) {
        for ( i = 0; i < setup.player_count; i++ )
            if ( setup.names[i] ) free( setup.names[i] );
        free( setup.names ); setup.names = 0;
    }
    if ( setup.modules ) {
        for ( i = 0; i < setup.player_count; i++ )
            if ( setup.modules[i] ) free( setup.modules[i] );
        free( setup.modules ); setup.modules = 0;
    }
    setup.player_count = 0;
}

/*
====================================================================
Delete scenario
====================================================================
*/
void scen_delete()
{
    int i;
#if 0
    int j;
    printf( "Casualties:\n" );
    for (i = 0; casualties && i < scen_info->player_count; i++) {
        printf( "%s Side\n", player_get_by_index(i)->name );
        for (j = 0; j < unit_class_count; j++)
            printf( "%d\t%s\n", scen_get_casualties( i, j ), unit_classes[j].name );
    }
#endif
    free( casualties );
    casualties = 0;
    turn = 0;
    cur_weather = 0;
    if ( scen_info ) {
        if ( scen_info->fname ) free( scen_info->fname );
        if ( scen_info->mod_name ) free( scen_info->mod_name );
        if ( scen_info->name ) free( scen_info->name );
        if ( scen_info->desc ) free( scen_info->desc );
        if ( scen_info->authors ) free( scen_info->authors );
        free( scen_info ); scen_info = 0;
    }
    if ( weather ) {
        free( weather ); weather = 0;
    }
    if ( vconds ) {
        for ( i = 0; i < vcond_count; i++ ) {
            if ( vconds[i].subconds_and )
                free( vconds[i].subconds_and );
            if ( vconds[i].subconds_or )
                free( vconds[i].subconds_or );
        }
        free( vconds ); vconds = 0;
        vcond_count = 0;
    }
    if ( units )
    {
        if (camp_loaded == CONTINUE_CAMPAIGN)
        {
            Unit *entry;
            list_reset( units );
            while ( ( entry = list_next( units ) ) )
                if (entry->core && !entry->killed)
                    list_transfer( units,reinf,entry );
        }
        else
            if (camp_loaded == RESTART_CAMPAIGN)
            {
                Unit *entry;
                list_reset( units );
                while ( ( entry = list_next( units ) ) )
                    if (entry->core == STARTING_CORE)// && !entry->killed)
                        list_transfer( units,reinf,entry );
            }
        list_delete( units );
        units = 0;
    }
    if ( vis_units ) {
        list_delete( vis_units );
        vis_units = 0;
    }
    if ( avail_units ) {
        list_delete( avail_units );
        avail_units = 0;
    }
    if ( reinf && camp_loaded <= 1 )
    {
        list_delete( reinf );
        reinf = 0;
    }
    nations_delete();
    if (camp_loaded <= 1)
        unit_lib_delete();
    players_delete();
    map_delete();

    free(scen_domain);
    scen_domain = 0;
}

/*
====================================================================
Set the current attack and movement. If SCEN_PREP_UNIT_FIRST is 
passed the entrenchment is not modified. Units are neither supplied
or killed here!
====================================================================
*/
void scen_prep_unit( Unit *unit, int type )
{
    int min_entr, max_entr;
    /* unit may not be undeployed */
    unit->fresh_deploy = 0;
    /* remove turn suppression */
    unit->turn_suppr = 0;
    /* allow actions */
    unit_unmount( unit );
    /* check instant_purchase button and stop recently placed units from acting */
    if (type == SCEN_PREP_UNIT_DEPLOY && config.purchase == INSTANT_PURCHASE)
    {
        unit->cur_mov = 0;
        unit->cur_atk_count = 0;
        unit->unused = 0;
    }
    else
    {
        unit->unused = 1;
        unit->cur_mov = unit->sel_prop->mov;
        if ( unit_check_ground_trsp( unit ) )
            unit->cur_mov = unit->trsp_prop.mov;
        /* if we have bad weather the relation between mov : fuel is 1 : 2 
         * for ground units
         */
        if ( !unit_has_flag( unit->sel_prop, "swimming" ) && !unit_has_flag( unit->sel_prop, "flying" )
             && (weather_types[scen_get_weather()].flags & DOUBLE_FUEL_COST) )
        {
            if ( unit_check_fuel_usage( unit ) && unit->cur_fuel < unit->cur_mov * 2 )
                unit->cur_mov = unit->cur_fuel / 2;
        }
        else
            if ( unit_check_fuel_usage( unit ) && unit->cur_fuel < unit->cur_mov )
                unit->cur_mov = unit->cur_fuel;
        unit->cur_atk_count = unit->sel_prop->atk_count;
    }
    /* if unit is prepared normally check entrenchment */
    if ( type == SCEN_PREP_UNIT_NORMAL && !unit_has_flag( unit->sel_prop, "flying" ) ) {
        min_entr = terrain_type_find( map_tile( unit->x, unit->y )->terrain_id[0] )->min_entr;
        max_entr = terrain_type_find( map_tile( unit->x, unit->y )->terrain_id[0] )->max_entr;
        if ( unit->entr < min_entr )
            unit->entr = min_entr;
        else
            if ( unit->entr < max_entr )
                unit->entr++;
    }
}

/*
====================================================================
Check if the victory conditions are fullfilled and if so
return True. 'result' is used then
to determine the next scenario in the campaign.
If 'after_last_turn' is set this is the check called by end_turn().
If no condition is fullfilled the else condition is used (very 
first one).
====================================================================
*/
int scen_check_result( int after_last_turn )
{
    int i, j, and_okay, or_okay;
#if DEBUG_CAMPAIGN
    char fname[MAX_PATH];
    FILE *f;
    snprintf(fname, sizeof fname, "%s/.lgames/.scenresult", getenv("HOME"));
    f = fopen(fname, "r");
    if (f) {
        unsigned len;
        scen_result[0] = '\0';
        fgets(scen_result, sizeof scen_result, f);
        fclose(f);
        len = strlen(scen_result);
        if (len > 0 && scen_result[len-1] == '\n') scen_result[len-1] = '\0';
        strcpy(scen_message, scen_result);
        if (len > 0) return 1;
    }
#endif
    if ( cheat_endscn ) {
        cheat_endscn = 0;
        return 1;
    }
    else {
        if ( vcond_check_type == VCOND_CHECK_EVERY_TURN || after_last_turn ) {
            for ( i = 1; i < vcond_count; i++ ) {
                /* AND binding */
                and_okay = 1;
                for ( j = 0; j < vconds[i].sub_and_count; j++ )
                    if ( !subcond_check( &vconds[i].subconds_and[j] ) ) {
                        and_okay = 0;
                        break;
                }
                /* OR binding */
                or_okay = 0;
                for ( j = 0; j < vconds[i].sub_or_count; j++ )
                    if ( subcond_check( &vconds[i].subconds_or[j] ) ) {
                        or_okay = 1;
                        break;
                    }
                if ( vconds[i].sub_or_count == 0 ) 
                    or_okay = 1;
                if ( or_okay && and_okay ) {
                    strcpy( scen_result, vconds[i].result );
                    strcpy( scen_message, vconds[i].message );
                    return 1;
                }
            }
        }
    }
    if ( after_last_turn ) {
        strcpy( scen_result, vconds[0].result );
        strcpy( scen_message, vconds[0].message );
        return 1;
    }
    return 0;
}
/*
====================================================================
Return True if scenario is done.
====================================================================
*/
int scen_done()
{
    return scen_result[0] != 0;
}
/*
====================================================================
Return result string.
====================================================================
*/
char *scen_get_result()
{
    return scen_result;
}
/*
====================================================================
Return result message
====================================================================
*/
char *scen_get_result_message()
{
    return scen_message;
}
/*
====================================================================
Clear result and message
====================================================================
*/
void scen_clear_result()
{
    scen_result[0] = 0;
    scen_message[0] = 0;
}

/*
====================================================================
Check the supply level of a unit. (hex tiles with SUPPLY_GROUND
have 100% supply)
====================================================================
*/
void scen_adjust_unit_supply_level( Unit *unit )
{
    unit->supply_level = map_get_unit_supply_level( unit->x, unit->y, unit );
}

/*
====================================================================
Get current weather/forecast
====================================================================
*/
int scen_get_weather( void )
{
    if ( turn < scen_info->turn_limit && config.weather != OPTION_WEATHER_OFF )
        return weather[turn];
    else
        return 0;
}
int scen_get_forecast( void )
{
    if ( turn + 1 < scen_info->turn_limit )
        return weather[turn + 1];
    else
        return 0;
}

/*
====================================================================
Get date string of current date.
====================================================================
*/
void scen_get_date( char *date_str )
{
    int hour;
    int phase;
    char buf[256];
    Date date = scen_info->start_date;
    if ( scen_info->days_per_turn > 0 ) {
        date_add_days( &date, scen_info->days_per_turn * turn );
        date_to_str( date_str, date, FULL_NAME_DATE );
    }
    else {
        date_add_days( &date, turn / scen_info->turns_per_day );
        date_to_str( buf, date, FULL_NAME_DATE );
        phase = turn % scen_info->turns_per_day;
        hour = 8 + phase * 6;
        sprintf( date_str, "%s %02i:00", buf, hour );
    }
}

/*
====================================================================
Get/Add casualties for unit class and player.
====================================================================
*/
int scen_get_casualties( int player, int class )
{
    if (!casualties || player < 0 || player >= scen_info->player_count
        || class < 0 || class >= unit_class_count)
        return 0;
    return casualties[player*unit_class_count + class];
}
int scen_inc_casualties( int player, int class )
{
    if (!casualties || player < 0 || player >= scen_info->player_count
         || class < 0 || class >= unit_class_count)
        return 0;
    return ++casualties[player*unit_class_count + class];
}
/*
====================================================================
Add casualties for unit. Regard unit and transport classes.
====================================================================
*/
int scen_inc_casualties_for_unit( Unit *unit )
{
    int player = player_get_index( unit->player );
    int cnt = scen_inc_casualties( player, unit->prop.class );
    if ( unit->trsp_prop.id )
        cnt = scen_inc_casualties( player, unit->trsp_prop.class );
    return cnt;
}

/*
====================================================================
Panzer General random weather generator using algorithm discovered and
documented by @Rudankort at
http://www.panzercentral.com/forum/viewtopic.php?p=577467#p577467
====================================================================
*/
void scen_create_random_weather( int start_water_level, int cur_period )
{
    /* counter of turns left in current weather period */
    int period_length = 0;
    /* changing initial period to the oposite - will be reset on first period_length reading */
    cur_period = !cur_period;

    int i, precipitation_type;
    precipitation_type = 0; // used to select rain or snow for overcast period: 0 - rain; 1 - snow
/*    weather_date.day = scen_info->start_date.day;
    weather_date.month = scen_info->start_date.month;
    weather_date.year = scen_info->start_date.year;*/
//    fprintf(stderr, "%d.%d.%d\n", weather_date.day, weather_date.month, weather_date.year);

    for ( i = 0; i < scen_info->turn_limit; i++ )
    {
        Date weather_date = scen_info->start_date;
        if ( period_length == 0 )
        {
            cur_period = !cur_period;
            if ( zones_weather_table[scen_info->weather_zone][scen_info->start_date.month][cur_period] == 0 ) 
                cur_period = !cur_period;
            period_length = ( rand() % zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]
                            + rand() % zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]
                            + rand() % zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]
                            + rand() % zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]
                            + 2 ) / 2;
            if ( period_length > 3*zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]/2 )
                period_length = 3*zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]/2;
            if ( period_length < zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]/2 )
                period_length = zones_weather_table[scen_info->weather_zone][weather_date.month][cur_period]/2;
            period_length--;
            // precipitation type: 0 - rain; 1 - snow
            if ( (rand() % 100) + 1 <= zones_weather_table[scen_info->weather_zone][weather_date.month][2] )
                precipitation_type = 1;
            else
                precipitation_type = 0;
        }
        else
            period_length--;
        weather[i] = cur_period;
        // determine rain or snow for overcast period only
        if ( cur_period == 1 )
            if ( (rand() % 100) + 1 <= zones_weather_table[scen_info->weather_zone][weather_date.month][3] && i > 0 )
            {
                weather[i] += precipitation_type + 1;
                start_water_level += 2;
            }
            else
                start_water_level += -1 + precipitation_type;
        else
            start_water_level -= 2;
        if ( start_water_level < 0 )
            start_water_level = 0;
        else if ( start_water_level > 5 )
            start_water_level = 5;
        weather[i] += 4 * ( precipitation_type + 1 ) * (int)( start_water_level / 3 );

//        fprintf(stderr, "zone:%d period:%d period length:%d water level:%d  %d\n", scen_info->weather_zone, cur_period, period_length, start_water_level, precipitation_type);
        if ( scen_info->days_per_turn > 0 )
            date_add_days( &weather_date, scen_info->days_per_turn * i );
        else
            date_add_days( &weather_date, i / scen_info->turns_per_day );
/*
        char str[100];
        date_to_str( str, weather_date, FULL_NAME_DATE );
        fprintf(stderr, "%s %d\n", str, weather[i]);
*/
    }
//    fprintf(stderr, "\n", period_length);
}

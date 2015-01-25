/***************************************************************************
                          map.c  -  description
                             -------------------
    begin                : Mon Jan 22 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
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
#include "file.h"
#include "nation.h"
#include "unit.h"
#include "map.h"
#include "misc.h"
#include "localize.h"

#include <assert.h>
#include <limits.h>

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern Terrain_Type *terrain_types;
extern int terrain_type_count;
extern int hex_w, hex_h, hex_x_offset;
extern List *vis_units;
extern int cur_weather;
extern int nation_flag_width, nation_flag_height;
extern Config config;
extern Terrain_Icons *terrain_icons;
extern Unit_Info_Icons *unit_info_icons;
extern Player *cur_player;
extern List *units;
extern int modify_fog;

/*
====================================================================
Map
====================================================================
*/
int map_w = 0, map_h = 0;
Map_Tile **map = 0;
Mask_Tile **mask = 0;

/*
====================================================================
Locals
====================================================================
*/

enum { DIST_AIR_MAX = SHRT_MAX };

typedef struct {
    short x, y;
} MapCoord;

static List *deploy_fields;

/*
====================================================================
Check the surrounding tiles and get the one with the highest
in_range value.
====================================================================
*/
static void map_get_next_unit_point( int x, int y, int *next_x, int *next_y )
{
    int high_x, high_y;
    int i;
    high_x = x; high_y = y;
    for ( i = 0; i < 6; i++ )
        if ( get_close_hex_pos( x, y, i, next_x, next_y ) )
            if ( mask[*next_x][*next_y].in_range > mask[high_x][high_y].in_range ) {
                high_x = *next_x;
                high_y = *next_y;
            }
    *next_x = high_x;
    *next_y = high_y;
}

/*
====================================================================
Add a unit's influence to the (vis_)infl mask.
====================================================================
*/
static void map_add_vis_unit_infl( Unit *unit )
{
    int i, next_x, next_y;
    if ( unit->sel_prop->flags & FLYING ) {
        mask[unit->x][unit->y].vis_air_infl++;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].vis_air_infl++;
    }
    else {
        mask[unit->x][unit->y].vis_infl++;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].vis_infl++;
    }
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Load map.
====================================================================
*/
int map_load( char *fname )
{
    int i, x, y, j, limit;
    PData *pd;
    char path[512];
    char *str, *tile;
    char *domain = 0;
    List *tiles, *names;
    sprintf( path, "%s/maps/%s", get_gamedir(), fname );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    /* map size */
    if ( !parser_get_int( pd, "width", &map_w ) ) goto parser_failure;
    if ( !parser_get_int( pd, "height", &map_h ) ) goto parser_failure;
    /* load terrains */
    if ( !parser_get_value( pd, "terrain_db", &str, 0 ) ) goto parser_failure;
    if ( !terrain_load( str ) ) goto failure;
    if ( !parser_get_values( pd, "tiles", &tiles ) ) goto parser_failure;
    /* allocate map memory */
    map = calloc( map_w, sizeof( Map_Tile* ) );
    for ( i = 0; i < map_w; i++ )
        map[i] = calloc( map_h, sizeof( Map_Tile ) );
    mask = calloc( map_w, sizeof( Mask_Tile* ) );
    for ( i = 0; i < map_w; i++ )
        mask[i] = calloc( map_h, sizeof( Mask_Tile ) );
    /* map itself */
    list_reset( tiles );
    for ( y = 0; y < map_h; y++ )
        for ( x = 0; x < map_w; x++ ) {
            tile = list_next( tiles );
            /* default is no flag */
            map[x][y].nation = 0;
            map[x][y].player = 0;
            map[x][y].deploy_center = 0;
            map[x][y].supply_center = 0;
            /* default is no mil target */
            map[x][y].obj = 0;
            /* check tile type */
            for ( j = 0; j < terrain_type_count; j++ ) {
                if ( terrain_types[j].id == tile[0] ) {
                    map[x][y].terrain = &terrain_types[j];
                    map[x][y].terrain_id = j;
                }
            }
            /* tile not found, used first one */
            if ( map[x][y].terrain == 0 )
                map[x][y].terrain = &terrain_types[0];
            /* check image id -- set offset */
            limit = map[x][y].terrain->images[0]->w / hex_w - 1;
            if ( tile[1] == '?' )
                /* set offset by random */
                map[x][y].image_offset = RANDOM( 0, limit ) * hex_w;
            else
                map[x][y].image_offset = atoi( tile + 1 ) * hex_w;
            /* set name */
            map[x][y].name = strdup( map[x][y].terrain->name );
        }
    /* map names */
    if ( parser_get_values( pd, "names", &names ) ) {
        list_reset( names );
        for ( i = 0; i < names->count; i++ ) {
            str = list_next( names );
            x = i % map_w;
            y = i / map_w;
            free( map[x][y].name );
            map[x][y].name = strdup( trd(domain, str) );
        }
    }
    parser_free( &pd );
    free(domain);
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    map_delete();
    if ( pd ) parser_free( &pd );
    free(domain);
    return 0;
}

/*
====================================================================
Delete map.
====================================================================
*/
void map_delete( )
{
    int i, j;
    if ( deploy_fields ) list_delete( deploy_fields );
    deploy_fields = 0;
    terrain_delete();
    if ( map ) {
        for ( i = 0; i < map_w; i++ )
            if ( map[i] ) {
                for ( j = 0; j < map_h; j++ )
                    if ( map[i][j].name )
                        free ( map[i][j].name );
                free( map[i] );
            }
        free( map );
    }
    if ( mask ) {
        for ( i = 0; i < map_w; i++ )
            if ( mask[i] )
                free( mask[i] );
        free( mask );
    }
    map = 0; mask = 0;
    map_w = map_h = 0;
}

/*
====================================================================
Get tile at x,y
====================================================================
*/
Map_Tile* map_tile( int x, int y )
{
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) {
#if 0
        fprintf( stderr, "map_tile: map tile at %i,%i doesn't exist\n", x, y);
#endif
        return 0;
    }
    return &map[x][y];
}
Mask_Tile* map_mask_tile( int x, int y )
{
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) {
#if 0
        fprintf( stderr, "map_tile: mask tile at %i,%i doesn't exist\n", x, y);
#endif
        return 0;
    }
    return &mask[x][y];
}

/*
====================================================================
Clear the passed map mask flags.
====================================================================
*/
void map_clear_mask( int flags )
{
    int i, j;
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ ) {
            if ( flags & F_FOG ) mask[i][j].fog = 1;
            if ( flags & F_INVERSE_FOG ) mask[i][j].fog = 0;
            if ( flags & F_SPOT ) mask[i][j].spot = 0;
            if ( flags & F_IN_RANGE ) mask[i][j].in_range = 0;
            if ( flags & F_MOUNT ) mask[i][j].mount = 0;
            if ( flags & F_SEA_EMBARK ) mask[i][j].sea_embark = 0;
            if ( flags & F_AUX ) mask[i][j].aux = 0;
            if ( flags & F_INFL ) mask[i][j].infl = 0;
            if ( flags & F_INFL_AIR ) mask[i][j].air_infl = 0;
            if ( flags & F_VIS_INFL ) mask[i][j].vis_infl = 0;
            if ( flags & F_VIS_INFL_AIR ) mask[i][j].vis_air_infl = 0;
            if ( flags & F_BLOCKED ) mask[i][j].blocked = 0;
            if ( flags & F_BACKUP ) mask[i][j].backup = 0;
            if ( flags & F_MERGE_UNIT ) mask[i][j].merge_unit = 0;
            if ( flags & F_DEPLOY ) mask[i][j].deploy = 0;
            if ( flags & F_CTRL_GRND ) mask[i][j].ctrl_grnd = 0;
            if ( flags & F_CTRL_AIR ) mask[i][j].ctrl_air = 0;
            if ( flags & F_CTRL_SEA ) mask[i][j].ctrl_sea = 0;
            if ( flags & F_MOVE_COST ) mask[i][j].moveCost = 0;
            if ( flags & F_DISTANCE ) mask[i][j].distance = -1;
            if ( flags & F_DANGER ) mask[i][j].danger = 0;
            if ( flags & F_SPLIT_UNIT ) 
            { 
                mask[i][j].split_unit = 0; 
                mask[i][j].split_okay = 0;
            }
        }
}

/*
====================================================================
Swap units. Returns the previous unit or 0 if none.
====================================================================
*/
Unit *map_swap_unit( Unit *unit )
{
    Unit *old;
    if ( unit->sel_prop->flags & FLYING ) {
        old = map_tile( unit->x, unit->y )->a_unit;
        map_tile( unit->x, unit->y )->a_unit = unit;
    }
    else {
        old = map_tile( unit->x, unit->y )->g_unit;
        map_tile( unit->x, unit->y )->g_unit = unit;
    }
    unit->terrain = map[unit->x][unit->y].terrain;
    return old;
}

/*
====================================================================
Insert, Remove unit pointer from map.
====================================================================
*/
void map_insert_unit( Unit *unit )
{
    Unit *old = map_swap_unit( unit );
    if ( old ) {
        fprintf( stderr, "insert_unit_to_map: warning: "
                         "unit %s hasn't been removed properly from %i,%i:"
                         "overwrite it\n",
                     old->name, unit->x, unit->y );
    }
}
void map_remove_unit( Unit *unit )
{
    if ( unit->sel_prop->flags & FLYING )
        map_tile( unit->x, unit->y )->a_unit = 0;
    else
        map_tile( unit->x, unit->y )->g_unit = 0;
}

/*
====================================================================
Get neighbored tiles clockwise with id between 0 and 5.
====================================================================
*/
Map_Tile* map_get_close_hex( int x, int y, int id )
{
    int next_x, next_y;
    if ( get_close_hex_pos( x, y, id, &next_x, &next_y ) )
        return &map[next_x][next_y];
    return 0;
}

/*
====================================================================
Add/set spotting of a unit to auxiliary mask
====================================================================
*/
void map_add_unit_spot_mask_rec( Unit *unit, int x, int y, int points )
{
    int i, next_x, next_y;
    /* break if this tile is already spotted */
    if ( mask[x][y].aux >= points ) return;
    /* spot tile */
    mask[x][y].aux = points;
    /* substract points */
    points -= map[x][y].terrain->spt[cur_weather];
    /* if there are points remaining continue spotting */
    if ( points > 0 )
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( x, y, i, &next_x, &next_y ) )
                if ( !( map[next_x][next_y].terrain->flags[cur_weather] & NO_SPOTTING ) )
                    map_add_unit_spot_mask_rec( unit, next_x, next_y, points );
}
void map_add_unit_spot_mask( Unit *unit )
{
    int i, next_x, next_y;
    if ( unit->x < 0 || unit->y < 0 || unit->x >= map_w || unit->y >= map_h ) return;
    mask[unit->x][unit->y].aux = unit->sel_prop->spt;
    for ( i = 0; i < 6; i++ )
        if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
            map_add_unit_spot_mask_rec( unit, next_x, next_y, unit->sel_prop->spt );
}
void map_get_unit_spot_mask( Unit *unit )
{
    map_clear_mask( F_AUX );
    map_add_unit_spot_mask( unit );
}

/*
====================================================================
Set movement range of a unit to in_range/sea_embark/mount.
====================================================================
*/
#ifdef OLD
void map_add_unit_move_mask_rec( Unit *unit, int x, int y, int distance, int points, int stage )
{
    int next_x, next_y, i, moves;
// if (strstr(unit->name,"leF")) printf("%s: points=%d stage=%d\n", unit->name, points, stage);
    /* break if this tile is already checked */
    if ( mask[x][y].in_range >= points ) return;
    /* the outter map tiles may not be entered */
    if ( x <= 0 || y <= 0 || x >= map_w - 1 || y >= map_h - 1 ) return;
    /* must mount to come here?
     * Do set mount flag only in stage 1, and only under the condition if
     * the field could not have been reached without the unit having a
     * transport.
     */
    if ( stage == 1 && !mask[x][y].in_range
         && unit->embark == EMBARK_NONE && unit->trsp_prop.id ) {
        if ( unit->cur_mov - points < unit->prop.mov )
            mask[x][y].mount = 0;
        else
            mask[x][y].mount = 1;
    }
    /* mark as reachable */
    mask[x][y].in_range = points;
    /* remember distance */
    if (mask[x][y].distance==-1||distance<mask[x][y].distance)
        mask[x][y].distance = distance;
    /* substract points */
    moves = unit_check_move( unit, x, y, stage );
    if ( moves == -1 ) {
        /* -1 means that it takes all points and the
           unit must be beside the tile to enter it
           thus it must not have moved before */
        if ( points < ( stage == 0 && unit->cur_mov > unit->sel_prop->mov
			? unit->sel_prop->mov : unit->cur_mov ) )
            mask[x][y].in_range = 0;
        points = 0;
    }
    else
        points -= moves;
// if (strstr(unit->name,"leF")) printf("moves=%d mask[%d][%d].in_range=%d\n", moves, x, y, mask[x][y].in_range);
    /* go on if points left */
    if ( points > 0 )
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( x, y, i, &next_x, &next_y ) )
                if ( unit_check_move( unit, next_x, next_y, stage ) != 0 )
                    map_add_unit_move_mask_rec( unit, next_x, next_y, distance+1, points, stage );
}
void map_get_unit_move_mask( Unit *unit )
{
    int i, next_x, next_y, stage, distance = 0;

    map_clear_unit_move_mask();
    if ( unit->x < 0 || unit->y < 0 || unit->x >= map_w || unit->y >= map_h ) return;
    if ( unit->cur_mov == 0 ) return;
    mask[unit->x][unit->y].in_range = unit->cur_mov + 1;
    mask[unit->x][unit->y].distance = distance;
    for ( stage = 0; stage < 2; stage++) {
    for ( i = 0; i < 6; i++ )
        if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) ) {
            if ( map_check_unit_embark( unit, next_x, next_y, EMBARK_SEA, 0 ) ) {
                /* unit may embark to sea transporter */
                mask[next_x][next_y].sea_embark = 1;
                continue;
            }
            if ( map_check_unit_debark( unit, next_x, next_y, EMBARK_SEA, 0 ) ) {
                /* unit may debark from sea transporter */
                mask[next_x][next_y].sea_embark = 1;
                continue;
            }
            if ( unit_check_move( unit, next_x, next_y, stage ) )
                map_add_unit_move_mask_rec( unit, next_x, next_y, distance+1,
			stage == 0 && unit->cur_mov > unit->sel_prop->mov
			? unit->sel_prop->mov : unit->cur_mov, stage );
        }
    }
    mask[unit->x][unit->y].blocked = 1;
}
#endif
/*
====================================================================
Check whether unit can enter (x,y) provided it has 'points' move
points remaining. 'mounted' means, to use the base cost for the 
transporter. Return the fuel cost (<=points) of this. If entering
is not possible, 'cost' is undefined.
====================================================================
*/
int unit_can_enter_hex( Unit *unit, int x, int y, int is_close, int points, int mounted, int *cost )
{
    int base = terrain_get_mov( map[x][y].terrain, unit->sel_prop->mov_type, cur_weather );
    /* if we check the mounted case, we'll have to use the ground transporter's cost */
    if ( mounted && unit->trsp_prop.id )
        base = terrain_get_mov( map[x][y].terrain, unit->trsp_prop.mov_type, cur_weather );
    /* allied bridge engineers on river? */
    if ( map[x][y].terrain->flags[cur_weather] & RIVER )
        if ( map[x][y].g_unit && map[x][y].g_unit->sel_prop->flags & BRIDGE_ENG )
            if ( player_is_ally( unit->player, map[x][y].g_unit->player ) )
                base = 1;
    /* impassable? */
    if (base==0) return 0;
    /* cost's all but not close? */
    if (base==-1&&!is_close) return 0;
    /* not enough points left? */
    if (base>0&&points<base) return 0;
    /* you can move over allied units but then mask::blocked must be set
     * because you must not stop at this tile */
    if ( ( x != unit->x  || y != unit->y ) && mask[x][y].spot ) {
        if ( map[x][y].a_unit && ( unit->sel_prop->flags & FLYING ) ) {
            if ( !player_is_ally( unit->player, map[x][y].a_unit->player ) )
                return 0;
            else
                map_mask_tile( x, y )->blocked = 1;
        }
        if ( map[x][y].g_unit && !( unit->sel_prop->flags & FLYING ) ) {
            if ( !player_is_ally( unit->player, map[x][y].g_unit->player ) )
                return 0;
            else
                map_mask_tile( x, y )->blocked = 1;
        }
    }
    /* if we already have to spent all; we are done */
    if (base==-1) { *cost = points; return 1; }
    /* entering an influenced tile costs all remaining points */
    *cost = base;
    if ( unit->sel_prop->flags & FLYING ) {
        if ( mask[x][y].vis_air_infl > 0 )
            *cost = points;
    }
    else {
        if ( mask[x][y].vis_infl > 0 )
            *cost = points;
    }
    return 1;
}
/*
====================================================================
Check whether hex (x,y) is reachable by the unit. 'distance' is the
distance to the hex, the unit is standing on. 'points' is the number
of points the unit has remaining before trying to enter (x,y).
'mounted' means to re-check with the move mask of the transporter.
And to set mask[x][y].mount if a tile came in reach that was 
previously not.
====================================================================
*/
void map_add_unit_move_mask_rec( Unit *unit, int x, int y, int distance, int points, int mounted )
{
    int i, next_x, next_y, cost = 0;
    /* break if this tile is already checked */
    if ( mask[x][y].in_range >= points ) return;
    if ( mask[x][y].sea_embark ) return;
    /* the outer map tiles may not be entered */
    if ( x <= 0 || y <= 0 || x >= map_w - 1 || y >= map_h - 1 ) return;
    /* can we enter? if yes, how much does it cost? */
    if (distance==0||unit_can_enter_hex(unit,x,y,(distance==1),points,mounted,&cost))
    {
        /* remember distance */
        if (mask[x][y].distance==-1||distance<mask[x][y].distance)
            mask[x][y].distance = distance;
        distance = mask[x][y].distance;
        /* re-check: after substracting the costs, there must be more points left
           than previously */
        if ( mask[x][y].in_range >= points-cost ) return;
        /* enter tile new or with more points */
        points -= cost;
        if (mounted&&mask[x][y].in_range==-1)
        {
            mask[x][y].mount = 1;
            mask[x][y].moveCost = unit->trsp_prop.mov-points;
        }
        mask[x][y].in_range = points;
        /* get total move costs (basic unmounted) */
        if (!mounted)
            mask[x][y].moveCost = unit->sel_prop->mov-points;
    }
    else
        points = 0;
    /* all points consumed? if so, we can't go any further */
    if (points==0) return;
    /* check whether close hexes in range */
    for ( i = 0; i < 6; i++ )
        if ( get_close_hex_pos( x, y, i, &next_x, &next_y ) )
        {
            if ( distance==0 && map_check_unit_embark( unit, next_x, next_y, EMBARK_SEA, 0 ) ) {
                /* unit may embark to sea transporter */
                mask[next_x][next_y].sea_embark = 1;
                continue;
            }
            if ( distance==0 && map_check_unit_debark( unit, next_x, next_y, EMBARK_SEA, 0 ) ) {
                /* unit may debark from sea transporter */
                mask[next_x][next_y].sea_embark = 1;
                continue;
            }
            map_add_unit_move_mask_rec( unit, next_x, next_y, distance+1, points, mounted );
        }
}
void map_get_unit_move_mask( Unit *unit )
{
    int x,y;
    map_clear_unit_move_mask();
    /* we keep the semantic change of in_range local by doing a manual adjustment */
    for (x=0;x<map_w;x++) for(y=0;y<map_h;y++)
        mask[x][y].in_range=-1; 
    if ( unit->embark == EMBARK_NONE && unit->trsp_prop.id )
    {
        /* this goes wrong if a transportable unit may move in intervals which 
           is logically correct however (for that case automatic mount is not 
           possible, the user'd have to explicitly mount/unmount before starting
           to move); so all units should move in one go */

        //begin patch: we need to consider if our range is restricted by lack of fuel -trip
        //map_add_unit_move_mask_rec(unit,unit->x,unit->y,0,unit->prop.mov,0);
        //map_add_unit_move_mask_rec(unit,unit->x,unit->y,0,unit->trsp_prop.mov,1);
        int maxpoints = unit->prop.mov;
        if ((unit->prop.fuel || unit->trsp_prop.fuel) && 
                                                unit->cur_fuel < maxpoints) {
            maxpoints = unit->cur_fuel;
            //printf("limiting movement because fuel = %d\n", unit->cur_fuel);
        }
        map_add_unit_move_mask_rec(unit,unit->x,unit->y,0,maxpoints,0);
        /* fix for crashing when don't have enough fuel to use the land transport's full range -trip */
        maxpoints = unit->trsp_prop.mov;
        if ((unit->prop.fuel || unit->trsp_prop.fuel) && 
                                                unit->cur_fuel < maxpoints) {
            maxpoints = unit->cur_fuel;
            //printf("limiting expansion of movement via transport because fuel = %d\n", unit->cur_fuel);
        }
        map_add_unit_move_mask_rec(unit,unit->x,unit->y,0,maxpoints,1);
        //end of patch -trip
        
    }
    else
        map_add_unit_move_mask_rec(unit,unit->x,unit->y,0,unit->cur_mov,0);
    for (x=0;x<map_w;x++) for(y=0;y<map_h;y++)
        mask[x][y].in_range++;
}
void map_clear_unit_move_mask()
{
    map_clear_mask( F_IN_RANGE | F_MOUNT | F_SEA_EMBARK | F_BLOCKED | F_AUX | F_MOVE_COST | F_DISTANCE );
}

/*
====================================================================
Writes into the given array the coordinates of all friendly
airports, returning the number of coordinates written.
The array must be big enough to hold all the coordinates.
The function is guaranteed to never write more than (map_w*map_h)
entries.
====================================================================
*/
static int map_write_friendly_depot_list( Unit *unit, MapCoord *coords )
{
    int x, y;
    MapCoord *p = coords;
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if ( map_is_allied_depot( &map[x][y], unit ) ) {
                p->x = x; p->y = y;
                ++p;
            }
    return p - coords;
}
/*
====================================================================
Sets the distance mask beginning with the airfield at (ax, ay).
====================================================================
*/
static void map_get_dist_air_mask( int ax, int ay, short *dist_air_mask )
{
    int x, y;
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ ) {
            int d = get_dist( ax, ay, x, y ) - 1;
            if (d < dist_air_mask[y*map_w+x])
                dist_air_mask[y*map_w+x] = d;
        }
    dist_air_mask[ay*map_w+ax] = 0;
}
    
/*
====================================================================
Recreates the danger mask for 'unit'.
The fog must be set to the movement range of 'unit' for this
function to work properly.
The movement cost of the mask must have been set for 'unit'.
Returns 1 when at least one tile's danger mask was set, otherwise 0.
====================================================================
*/
int map_get_danger_mask( Unit *unit )
{
    int i, x, y, retval = 0;
    short *dist_air_mask = alloca(map_w*map_h*sizeof dist_air_mask[0]);
    MapCoord *airfields = alloca(map_w*map_h*sizeof airfields[0]);
    int airfield_count = map_write_friendly_depot_list( unit, airfields );

    /* initialise masks */
    for (i = 0; i < map_w*map_h; i++) dist_air_mask[i] = DIST_AIR_MAX;

    /* gather distance mask considering all friendly airfields */
    for ( i = 0; i < airfield_count; i++ )
        map_get_dist_air_mask( airfields[i].x, airfields[i].y, dist_air_mask );

    /* now mark as danger-zone any tile whose next friendly airfield is
       farther away than the fuel quantity left. */
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if (!mask[x][y].fog) {
                int left = unit->cur_fuel - unit_calc_fuel_usage(unit, mask[x][y].distance);
                retval |=
                mask[x][y].danger =
                    /* First compare distance to prospected fuel qty. If it's
                     * too far away, it's dangerous.
                     */
                    dist_air_mask[y*map_w+x] > left
                    /* Specifically allow supplied tiles.
                     */
                    && !map_get_unit_supply_level(x, y, unit);
            }
    return retval;
}
/*
====================================================================
Get a list of way points the unit moves along to it's destination.
This includes check for unseen influence by enemy units (e.g.
Surprise Contact).
====================================================================
*/
Way_Point* map_get_unit_way_points( Unit *unit, int x, int y, int *count, Unit **ambush_unit )
{
    Way_Point *way = 0, *reverse = 0;
    int i;
    int next_x, next_y;
    /* same tile ? */
    if ( unit->x == x && unit->y == y ) return 0;
    /* allocate memory */
    way = calloc( unit->cur_mov + 1, sizeof( Way_Point ) );
    reverse = calloc( unit->cur_mov + 1, sizeof( Way_Point ) );
    /* it's easiest to get positions in reverse order */
    next_x = x; next_y = y; *count = 0;
    while ( next_x != unit->x || next_y != unit->y ) {
        reverse[*count].x = next_x; reverse[*count].y = next_y;
        map_get_next_unit_point( next_x, next_y, &next_x, &next_y );
        (*count)++;
    }
    reverse[*count].x = unit->x; reverse[*count].y = unit->y; (*count)++;
    for ( i = 0; i < *count; i++ ) {
        way[i].x = reverse[(*count) - 1 - i].x;
        way[i].y = reverse[(*count) - 1 - i].y;
    }
    free( reverse );
    /* debug way points
    printf( "'%s': %i,%i", unit->name, way[0].x, way[0].y );
    for ( i = 1; i < *count; i++ )
        printf( " -> %i,%i", way[i].x, way[i].y );
    printf( "\n" ); */
    /* check for ambush and influence
     * if there is a unit in the way it must be an enemy (friends, spotted enemies are not allowed)
     * so cut down way to this way_point and set ambush_unit
     * if an unspotted tile does have influence >0 an enemy is nearby and our unit must stop
     */
    for ( i = 1; i < *count; i++ ) {
        /* check if on this tile a unit is waiting */
        /* if mask::blocked is set it's an own unit so don't check for ambush */
        if ( !map_mask_tile( way[i].x, way[i].y )->blocked ) {
            if ( map_tile( way[i].x, way[i].y )->g_unit )
                if ( !( unit->sel_prop->flags & FLYING ) ) {
                    *ambush_unit = map_tile( way[i].x, way[i].y )->g_unit;
                    break;
                }
            if ( map_tile( way[i].x, way[i].y )->a_unit )
                if ( unit->sel_prop->flags & FLYING ) {
                    *ambush_unit = map_tile( way[i].x, way[i].y )->a_unit;
                    break;
                }
        }
        /* if we get here there is no unit waiting but maybe close too the tile */
        /* therefore check tile of moving unit if it is influenced by a previously unspotted unit */
        if ( unit->sel_prop->flags & FLYING ) {
            if ( map_mask_tile( way[i - 1].x, way[i - 1].y )->air_infl && !map_mask_tile( way[i - 1].x, way[i - 1].y )->vis_air_infl )
                break;
        }
        else {
            if ( map_mask_tile( way[i - 1].x, way[i - 1].y )->infl && !map_mask_tile( way[i - 1].x, way[i - 1].y )->vis_infl )
                break;
        }
    }
    if ( i < *count ) *count = i; /* enemy in the way; cut down */
    return way;
}

/*
====================================================================
Backup/restore spot mask to/from backup mask.
====================================================================
*/
void map_backup_spot_mask()
{
    int x, y;
    map_clear_mask( F_BACKUP );
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            map_mask_tile( x, y )->backup = map_mask_tile( x, y )->spot;
}
void map_restore_spot_mask()
{
    int x, y;
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            map_mask_tile( x, y )->spot = map_mask_tile( x, y )->backup;
    map_clear_mask( F_BACKUP );
}

/*
====================================================================
Get unit's merge partners and set
mask 'merge'. At maximum MAP_MERGE_UNIT_LIMIT units.
All unused entries in partners are set 0.
====================================================================
*/
void map_get_merge_units( Unit *unit, Unit **partners, int *count )
{
    Unit *partner;
    int i, next_x, next_y;
    *count = 0;
    map_clear_mask( F_MERGE_UNIT );
    memset( partners, 0, sizeof( Unit* ) * MAP_MERGE_UNIT_LIMIT );
    /* check surrounding tiles */
    for ( i = 0; i < 6; i++ )
        if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) ) {
            partner = 0;
            if ( map[next_x][next_y].g_unit && unit_check_merge( unit, map[next_x][next_y].g_unit ) )
                partner = map[next_x][next_y].g_unit;
            else
                if ( map[next_x][next_y].a_unit && unit_check_merge( unit, map[next_x][next_y].a_unit ) )
                    partner = map[next_x][next_y].a_unit;
            if ( partner ) {
                partners[(*count)++] = partner;
                mask[next_x][next_y].merge_unit = partner;
            }
        }
}

/*
====================================================================
Check if unit may transfer strength to unit (if not NULL) or create
a stand alone unit (if unit NULL) on the coordinates.
====================================================================
*/
int map_check_unit_split( Unit *unit, int str, int x, int y, Unit *dest )
{
    if (unit->str-str<4) return 0;
    if (dest)
    {
        int old_str, ret;
        old_str = unit->str; unit->str = str;
        ret = unit_check_merge(unit,dest); /* is equal for now */
        unit->str = old_str;
        return ret;
    }
    else
    {
        if (str<4) return 0;
        if (!is_close(unit->x,unit->y,x,y)) return 0;
        if ((unit->sel_prop->flags&FLYING)&&map[x][y].a_unit) return 0;
        if (!(unit->sel_prop->flags&FLYING)&&map[x][y].g_unit) return 0;
        if (!terrain_get_mov(map[x][y].terrain,unit->sel_prop->mov_type,cur_weather)) return 0;
    }
    return 1;
}

/*
====================================================================
Get unit's split partners assuming unit wants to give 'str' strength
points and set mask 'split'. At maximum MAP_SPLIT_UNIT_LIMIT units.
All unused entries in partners are set 0. 'str' must be valid amount,
this is not checked here.
====================================================================
*/
void map_get_split_units_and_hexes( Unit *unit, int str, Unit **partners, int *count )
{
    Unit *partner;
    int i, next_x, next_y;
    *count = 0;
    map_clear_mask( F_SPLIT_UNIT );
    memset( partners, 0, sizeof( Unit* ) * MAP_SPLIT_UNIT_LIMIT );
    /* check surrounding tiles */
    for ( i = 0; i < 6; i++ )
        if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) ) {
            partner = 0;
            if ( map[next_x][next_y].g_unit && map_check_unit_split( unit, str, next_x, next_y, map[next_x][next_y].g_unit ) )
                partner = map[next_x][next_y].g_unit;
            else
                if ( map[next_x][next_y].a_unit && map_check_unit_split( unit, str, next_x, next_y, map[next_x][next_y].a_unit ) )
                    partner = map[next_x][next_y].a_unit;
            else
                if ( map_check_unit_split( unit, str, next_x, next_y, 0 ) )
                    mask[next_x][next_y].split_okay = 1;
            if ( partner ) {
                partners[(*count)++] = partner;
                mask[next_x][next_y].split_unit = partner;
            }
        }
}


/*
====================================================================
Get a list (vis_units) of all visible units by checking spot mask.
====================================================================
*/
void map_get_vis_units( void )
{
    int x, y;
    list_clear( vis_units );
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if ( mask[x][y].spot || ( cur_player && cur_player->ctrl == PLAYER_CTRL_CPU ) ) {
                if ( map[x][y].g_unit ) list_add( vis_units, map[x][y].g_unit );
                if ( map[x][y].a_unit ) list_add( vis_units, map[x][y].a_unit );
            }
}

/*
====================================================================
Draw a map tile terrain to surface. (fogged if mask::fog is set)
====================================================================
*/
void map_draw_terrain( SDL_Surface *surf, int map_x, int map_y, int x, int y )
{
    Map_Tile *tile;
    if ( map_x < 0 || map_y < 0 || map_x >= map_w || map_y >= map_h ) return;
    tile = &map[map_x][map_y];
    /* terrain */
    DEST( surf, x, y, hex_w, hex_h );
    if ( mask[map_x][map_y].fog )
        SOURCE( tile->terrain->images_fogged[cur_weather], tile->image_offset, 0 )
    else
        SOURCE( tile->terrain->images[cur_weather], tile->image_offset, 0 )
    blit_surf();
    /* nation flag */
    if ( tile->nation !=0 && tile->supply_center<2) {
        nation_draw_flag( tile->nation, surf,
                          x + ( ( hex_w - nation_flag_width ) >> 1 ),
                          y + hex_h - nation_flag_height - 2,
                          tile->obj );
    }
    /* grid */
    if ( config.grid ) {
        DEST( surf, x, y, hex_w, hex_h );
        SOURCE( terrain_icons->grid, 0, 0 );
        blit_surf();
    }
}
/*
====================================================================
Draw tile units. If mask::fog is set no units are drawn.
If 'ground' is True the ground unit is drawn as primary
and the air unit is drawn small (and vice versa).
If 'select' is set a selection frame is added.
====================================================================
*/
void map_draw_units( SDL_Surface *surf, int map_x, int map_y, int x, int y, int ground, int select )
{
    Unit *unit = 0;
    Map_Tile *tile;
    if ( map_x < 0 || map_y < 0 || map_x >= map_w || map_y >= map_h ) return;
    tile = &map[map_x][map_y];
    /* units */
    if ( MAP_CHECK_VIS( map_x, map_y ) ) {
        if ( tile->g_unit ) {
            if ( ground || tile->a_unit == 0 ) {
                /* large ground unit */
                DEST( surf,
                      x + ( (hex_w - tile->g_unit->sel_prop->icon_w) >> 1 ),
                      y + ( ( hex_h - tile->g_unit->sel_prop->icon_h ) >> 1 ),
                      tile->g_unit->sel_prop->icon_w, tile->g_unit->sel_prop->icon_h );
                SOURCE( tile->g_unit->sel_prop->icon, tile->g_unit->icon_offset, 0 );
                blit_surf();
                unit = tile->g_unit;
            }
            else {
                /* small ground unit */
                DEST( surf,
                      x + ( (hex_w - tile->g_unit->sel_prop->icon_tiny_w) >> 1 ),
                      y + ( ( hex_h - tile->g_unit->sel_prop->icon_tiny_h ) >> 1 ) + 4,
                      tile->g_unit->sel_prop->icon_tiny_w, tile->g_unit->sel_prop->icon_tiny_h );
                SOURCE( tile->g_unit->sel_prop->icon_tiny, tile->g_unit->icon_tiny_offset, 0 );
                blit_surf();
                unit = tile->a_unit;
            }
        }
        if ( tile->a_unit ) {
            if ( !ground || tile->g_unit == 0 ) {
                /* large air unit */
                DEST( surf,
                      x + ( (hex_w - tile->a_unit->sel_prop->icon_w) >> 1 ),
                      y + 6,
                      tile->a_unit->sel_prop->icon_w, tile->a_unit->sel_prop->icon_h );
                SOURCE( tile->a_unit->sel_prop->icon, tile->a_unit->icon_offset, 0 );
                blit_surf();
                unit = tile->a_unit;
            }
            else {
                /* small air unit */
                DEST( surf,
                      x + ( (hex_w - tile->a_unit->sel_prop->icon_tiny_w) >> 1 ),
                      y + 6,
                      tile->a_unit->sel_prop->icon_tiny_w, tile->a_unit->sel_prop->icon_tiny_h );
                SOURCE( tile->a_unit->sel_prop->icon_tiny, tile->a_unit->icon_tiny_offset, 0 );
                blit_surf();
                unit = tile->g_unit;
            }
        }
        /* unit info icons */
        if ( unit && config.show_bar ) {
            /* strength */
            DEST( surf,
                  x + ( ( hex_w - unit_info_icons->str_w ) >> 1 ),
                  y + hex_h - unit_info_icons->str_h,
                  unit_info_icons->str_w, unit_info_icons->str_h );
            if ( cur_player && player_is_ally( cur_player, unit->player ) )
                SOURCE( unit_info_icons->str, 
                        (unit&&(unit_low_ammo(unit)||unit_low_fuel(unit)))?unit_info_icons->str_w:0, 
                        unit_info_icons->str_h * ( unit->str - 1 + 15 ) )
            else
                SOURCE( unit_info_icons->str, 0, unit_info_icons->str_h * ( unit->str - 1 ) )
            blit_surf();
            /* for current player only */
            if ( unit->player == cur_player ) {
                /* attack */
                if ( unit->cur_atk_count > 0 ) {
                    DEST( surf, x + ( hex_w - hex_x_offset ), y + hex_h - unit_info_icons->atk->h,
                          unit_info_icons->atk->w, unit_info_icons->atk->h );
                    SOURCE( unit_info_icons->atk, 0, 0 );
                    blit_surf();
                }
                /* move */
                if ( unit->cur_mov > 0 ) {
                    DEST( surf, x + hex_x_offset - unit_info_icons->mov->w, y + hex_h - unit_info_icons->mov->h,
                          unit_info_icons->mov->w, unit_info_icons->mov->h );
                    SOURCE( unit_info_icons->mov, 0, 0 );
                    blit_surf();
                }
                /* guarding */
                if ( unit->is_guarding ) {
                    DEST( surf, x + ((hex_w-unit_info_icons->guard->w)>>1), 
                          y + hex_h - unit_info_icons->guard->h,
                          unit_info_icons->guard->w, unit_info_icons->guard->h );
                    SOURCE( unit_info_icons->guard, 0, 0 );
                    blit_surf();
                }
            }
        }
    }
    /* selection frame */
    if ( select ) {
        DEST( surf, x, y, hex_w, hex_h );
        SOURCE( terrain_icons->select, 0, 0 );
        blit_surf();
    }
}
/*
====================================================================
Draw danger tile. Expects 'surf' to contain a fully drawn tile at
the given position which will be tinted by overlaying the danger
terrain surface.
====================================================================
*/
void map_apply_danger_to_tile( SDL_Surface *surf, int map_x, int map_y, int x, int y )
{
    DEST( surf, x, y, hex_w, hex_h );
    SOURCE( terrain_icons->danger, 0, 0 );
    alpha_blit_surf( DANGER_ALPHA );
}
/*
====================================================================
Draw terrain and units.
====================================================================
*/
void map_draw_tile( SDL_Surface *surf, int map_x, int map_y, int x, int y, int ground, int select )
{
    map_draw_terrain( surf, map_x, map_y, x, y );
    map_draw_units( surf, map_x, map_y, x, y, ground, select );
}

/*
====================================================================
Set/update spot mask by engine's current player or unit.
The update adds the tiles seen by unit.
====================================================================
*/
void map_set_spot_mask()
{
    int i;
    int x, y, next_x, next_y;
    Unit *unit;
    map_clear_mask( F_SPOT );
    map_clear_mask( F_AUX ); /* buffer here first */
    /* get spot_mask for each unit and add to fog */
    /* use map::mask::aux as buffer */
    list_reset( units );
    for ( i = 0; i < units->count; i++ ) {
        unit = list_next( units );
        if ( unit->killed ) continue;
        if ( player_is_ally( cur_player, unit->player ) ) /* it's your unit or at least it's allied... */
            map_add_unit_spot_mask( unit );
    }
    /* check all flags; if flag belongs to you or any of your partners you see the surrounding, too */
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if ( map[x][y].player != 0 )
                if ( player_is_ally( cur_player, map[x][y].player ) ) {
                    mask[x][y].aux = 1;
                    for ( i = 0; i < 6; i++ )
                        if ( get_close_hex_pos( x, y, i, &next_x, &next_y ) )
                            mask[next_x][next_y].aux = 1;
                }
    /* convert aux to fog */
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if ( mask[x][y].aux || !config.fog_of_war )
                 mask[x][y].spot = 1;
    /* update the visible units list */
    map_get_vis_units();
}
void map_update_spot_mask( Unit *unit, int *enemy_spotted )
{
    int x, y;
    *enemy_spotted = 0;
    if ( player_is_ally( cur_player, unit->player ) ) {
        /* it's your unit or at least it's allied... */
        map_get_unit_spot_mask( unit );
        for ( x = 0; x < map_w; x++ )
            for ( y = 0; y < map_h; y++ )
                if ( mask[x][y].aux ) {
                    /* if there is an enemy in this auxialiary mask that wasn't spotted before */
                    /* set enemy_spotted true */
                    if ( !mask[x][y].spot ) {
                        if ( map[x][y].g_unit && !player_is_ally( unit->player, map[x][y].g_unit->player ) )
                            *enemy_spotted = 1;
                        if ( map[x][y].a_unit && !player_is_ally( unit->player, map[x][y].a_unit->player ) )
                            *enemy_spotted = 1;
                    }
                    mask[x][y].spot = 1;
                }
    }
}

/*
====================================================================
Set mask::fog (which is the actual fog of the engine) to either
spot mask, in_range mask (covers sea_embark), merge mask,
deploy mask.
====================================================================
*/
void map_set_fog( int type )
{
    int x, y;
    for ( y = 0; y < map_h; y++ )
        for ( x = 0; x < map_w; x++ )
            switch ( type ) {
                case F_SPOT: mask[x][y].fog = !mask[x][y].spot; break;
                case F_IN_RANGE: mask[x][y].fog = ( (!mask[x][y].in_range && !mask[x][y].sea_embark) || mask[x][y].blocked ); break;
                case F_MERGE_UNIT: mask[x][y].fog = !mask[x][y].merge_unit; break;
                case F_SPLIT_UNIT: mask[x][y].fog = !mask[x][y].split_unit&&!mask[x][y].split_okay; break;
                case F_DEPLOY: mask[x][y].fog = !mask[x][y].deploy; break;
                default: mask[x][y].fog = 0; break;
            }
}

/*
====================================================================
Set the fog to players spot mask by using mask::aux (not mask::spot)
====================================================================
*/
void map_set_fog_by_player( Player *player )
{
    int i;
    int x, y, next_x, next_y;
    Unit *unit;
    map_clear_mask( F_AUX ); /* buffer here first */
    /* units */
    list_reset( units );
    for ( i = 0; i < units->count; i++ ) {
        unit = list_next( units );
        if ( unit->killed ) continue;
        if ( player_is_ally( player, unit->player ) ) /* it's your unit or at least it's allied... */
            map_add_unit_spot_mask( unit );
    }
    /* check all flags; if flag belongs to you or any of your partners you see the surrounding, too */
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if ( map[x][y].player != 0 )
                if ( player_is_ally( player, map[x][y].player ) ) {
                    mask[x][y].aux = 1;
                    for ( i = 0; i < 6; i++ )
                        if ( get_close_hex_pos( x, y, i, &next_x, &next_y ) )
                            mask[next_x][next_y].aux = 1;
                }
    /* convert aux to fog */
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if ( mask[x][y].aux || !config.fog_of_war )
                 mask[x][y].fog = 0;
            else
                 mask[x][y].fog = 1;
}

/*
====================================================================
Modify the various influence masks.
====================================================================
*/
void map_add_unit_infl( Unit *unit )
{
    int i, next_x, next_y;
    if ( unit->sel_prop->flags & FLYING ) {
        mask[unit->x][unit->y].air_infl++;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].air_infl++;
    }
    else {
        mask[unit->x][unit->y].infl++;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].infl++;
    }
}
void map_remove_unit_infl( Unit *unit )
{
    int i, next_x, next_y;
    if ( unit->sel_prop->flags & FLYING ) {
        mask[unit->x][unit->y].air_infl--;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].air_infl--;
    }
    else {
        mask[unit->x][unit->y].infl--;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].infl--;
    }
}
void map_remove_vis_unit_infl( Unit *unit )
{
    int i, next_x, next_y;
    if ( unit->sel_prop->flags & FLYING ) {
        mask[unit->x][unit->y].vis_air_infl--;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].vis_air_infl--;
    }
    else {
        mask[unit->x][unit->y].vis_infl--;
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &next_x, &next_y ) )
                mask[next_x][next_y].vis_infl--;
    }
}
void map_set_infl_mask()
{
    Unit *unit = 0;
    map_clear_mask( F_INFL | F_INFL_AIR );
    /* add all hostile units influence */
    list_reset( units );
    while ( ( unit = list_next( units ) ) )
        if ( !unit->killed && !player_is_ally( cur_player, unit->player ) )
            map_add_unit_infl( unit );
    /* visible influence must also be updated */
    map_set_vis_infl_mask();
}
void map_set_vis_infl_mask()
{
    Unit *unit = 0;
    map_clear_mask( F_VIS_INFL | F_VIS_INFL_AIR );
    /* add all hostile units influence */
    list_reset( units );
    while ( ( unit = list_next( units ) ) )
        if ( !unit->killed && !player_is_ally( cur_player, unit->player ) )
            if ( map_mask_tile( unit->x, unit->y )->spot )
                map_add_vis_unit_infl( unit );
}

/*
====================================================================
Check if unit may air/sea embark/debark at x,y.
If 'init' != 0, used relaxed rules for deployment
====================================================================
*/
int map_check_unit_embark( Unit *unit, int x, int y, int type, int init )
{
    int i, nx, ny;
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) return 0;
    if ( type == EMBARK_AIR ) {
        if ( unit->sel_prop->flags & FLYING ) return 0;
        if ( unit->sel_prop->flags & SWIMMING ) return 0;
        if ( cur_player->air_trsp == 0 ) return 0;
        if ( unit->embark != EMBARK_NONE ) return 0;
        if ( !init && map[x][y].a_unit ) return 0;
        if ( unit->player->air_trsp_used >= unit->player->air_trsp_count ) return 0;
        if ( !init && !unit->unused ) return 0;
        if ( !init && !( map[x][y].terrain->flags[cur_weather] & SUPPLY_AIR ) ) return 0;
        if ( init && !(unit->sel_prop->flags&PARACHUTE) && !( map[x][y].terrain->flags[cur_weather] & SUPPLY_AIR ) ) return 0;
        if ( !( unit->sel_prop->flags & AIR_TRSP_OK ) ) return 0;
        if ( init && unit->trsp_prop.flags & TRANSPORTER ) return 0;
        return 1;
    }
    if ( type == EMBARK_SEA ) {
        if ( unit->sel_prop->flags & FLYING ) return 0;
        if ( unit->sel_prop->flags & SWIMMING ) return 0;
        if ( cur_player->sea_trsp == 0 ) return 0;
        if ( unit->embark != EMBARK_NONE || ( !init && unit->sel_prop->mov == 0 ) ) return 0;
        if ( !init && map[x][y].g_unit ) return 0;
        if ( unit->player->sea_trsp_used >= unit->player->sea_trsp_count ) return 0;
        if ( !init && !unit->unused ) return 0;
        if ( terrain_get_mov( map[x][y].terrain, unit->player->sea_trsp->mov_type, cur_weather ) == 0 ) return 0;
        /* basically we must be close to an harbor but a town that is just
           near the water is also okay because else it would be too
           restrictive. */
        if ( !init ) {
            if ( map[x][y].terrain->flags[cur_weather] & SUPPLY_GROUND )
                return 1;
            for ( i = 0; i < 6; i++ )
                if ( get_close_hex_pos( x, y, i, &nx, &ny ) )
                    if ( map[nx][ny].terrain->flags[cur_weather] & SUPPLY_GROUND )
                        return 1;
        }
        return init;
    }
    return 0;
}
int map_check_unit_debark( Unit *unit, int x, int y, int type, int init )
{
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) return 0;
    if ( type == EMBARK_SEA ) {
        if ( unit->embark != EMBARK_SEA ) return 0;
        if ( !init && map[x][y].g_unit ) return 0;
        if ( !init && !unit->unused ) return 0;
        if ( !init && terrain_get_mov( map[x][y].terrain, unit->prop.mov_type, cur_weather ) == 0 ) return 0;
        return 1;
    }
    if ( type == EMBARK_AIR ) {
        if ( unit->embark != EMBARK_AIR ) return 0;
        if ( !init && map[x][y].g_unit ) return 0;
        if ( !init && !unit->unused ) return 0;
        if ( !init && terrain_get_mov( map[x][y].terrain, unit->prop.mov_type, cur_weather ) == 0 ) return 0;
        if ( !init && !( map[x][y].terrain->flags[cur_weather] & SUPPLY_AIR ) && !( unit->prop.flags & PARACHUTE ) )
            return 0;
        return 1;
    }
    return 0;
}

/*
====================================================================
Embark/debark unit and return if an enemy was spotted.
If 'enemy_spotted' is 0, don't recalculate spot mask.
If unit's coordinates or x and y are out of bounds, the respective
tile is not manipulated.
For air debark (x,y)=(-1,-1) means deploy on airfield, otherwise 
it's a parachutist which means: it can drift away, it can loose 
strength, it may not move.
====================================================================
*/
void map_embark_unit( Unit *unit, int x, int y, int type, int *enemy_spotted )
{
    Map_Tile *tile;
    if ( type == EMBARK_AIR ) {
        /* not marked as used so may debark directly again */
        
        /* abandon ground transporter */
        memset( &unit->trsp_prop, 0, sizeof( Unit_Lib_Entry ) );
        /* set and change to air_tran_prop */
        memcpy( &unit->trsp_prop, cur_player->air_trsp, sizeof( Unit_Lib_Entry ) );
        unit->sel_prop = &unit->trsp_prop;
        unit->embark = EMBARK_AIR;
        /* unit now flies */
        tile = map_tile(x, y);
        if (tile) tile->a_unit = unit;
        tile = map_tile(unit->x, unit->y);
        if (tile) tile->g_unit = 0;
        /* set full move range */
        unit->cur_mov = unit->trsp_prop.mov;
        /* cancel attacks */
        unit->cur_atk_count = 0;
        /* no entrenchment */
        unit->entr = 0;
        /* adjust pic offset */
        unit_adjust_icon( unit );
        /* another air_tran in use */
        cur_player->air_trsp_used++;

        if (enemy_spotted)
            /* in any case your spotting must be updated */
            map_update_spot_mask( unit, enemy_spotted );
        return;
    }
    if ( type == EMBARK_SEA ) {
        /* action taken */
        unit->unused = 0;
        /* abandon ground transporter */
        /* FIXME! This is utterly wrong! */
        memset( &unit->trsp_prop, 0, sizeof( Unit_Lib_Entry ) );
        /* set and change to sea_tran_prop */
        memcpy( &unit->trsp_prop, cur_player->sea_trsp, sizeof( Unit_Lib_Entry ) );
        unit->sel_prop = &unit->trsp_prop;
        unit->embark = EMBARK_SEA;
        /* update position */
        tile = map_tile(unit->x, unit->y);
        if (tile) tile->g_unit = 0;
        unit->x = x; unit->y = y;
        tile = map_tile(x, y);
        if (tile) tile->g_unit = unit;
        /* set full move range */
        unit->cur_mov = unit->trsp_prop.mov;
        /* cancel attacks */
        unit->cur_atk_count = 0;
        /* no entrenchment */
        unit->entr = 0;
        /* adjust pic offset */
        unit_adjust_icon( unit );
        /* another air_tran in use */
        cur_player->sea_trsp_used++;

        if (enemy_spotted)
            /* in any case your spotting must be updated */
            map_update_spot_mask( unit, enemy_spotted );
        return;
    }
}
void map_debark_unit( Unit *unit, int x, int y, int type, int *enemy_spotted )
{
    Map_Tile *tile;
    if ( type == EMBARK_SEA ) {
        /* action taken */
        unit->unused = 0;
        /* change back to unit_prop */
        memset( &unit->trsp_prop, 0, sizeof( Unit_Lib_Entry ) );
        unit->sel_prop = &unit->prop;
        unit->embark = EMBARK_NONE;
        /* set position */
        tile = map_tile(unit->x, unit->y);
        if (tile) tile->g_unit = 0;
        unit->x = x; unit->y = y;
        tile = map_tile(x, y);
        if (tile) tile->g_unit = unit;
        if ( tile && tile->nation ) {
            tile->nation = unit->nation;
            tile->player = unit->player;
        }
        /* no movement allowed */
        unit->cur_mov = 0;
        /* cancel attacks */
        unit->cur_atk_count = 0;
        /* no entrenchment */
        unit->entr = 0;
        /* adjust pic offset */
        unit_adjust_icon( unit );
        /* free occupied sea transporter */
        cur_player->sea_trsp_used--;

        if (enemy_spotted)
            /* in any case your spotting must be updated */
            map_update_spot_mask( unit, enemy_spotted );
        return;
    }
    if ( type == EMBARK_AIR || type == DROP_BY_PARACHUTE ) {
        /* unit may directly move */
        
        /* change back to unit_prop */
        memset( &unit->trsp_prop, 0, sizeof( Unit_Lib_Entry ) );
        unit->sel_prop = &unit->prop;
        unit->embark = EMBARK_NONE;
        /* get down to earth */
        tile = map_tile(unit->x,unit->y);
        if ( tile ) tile->a_unit = 0;
        if (type == EMBARK_AIR)
        {
            if (tile) tile->g_unit = unit;
        }
        else
        {
            /* 70% chance of drifting */
            if (DICE(10)<=7)
            {
                int i,nx,ny,c=0,j;
                for (i=0;i<6;i++)
                    if (get_close_hex_pos(x,y,i,&nx,&ny))
                        if (map[nx][ny].g_unit==0)
                            if (terrain_get_mov(map[nx][ny].terrain,unit->prop.mov_type,cur_weather)!=0)
                                c++;
                j = DICE(c)-1;
                for (i=0,c=0;i<6;i++)
                    if (get_close_hex_pos(x,y,i,&nx,&ny))
                        if (map[nx][ny].g_unit==0)
                            if (terrain_get_mov(map[nx][ny].terrain,unit->prop.mov_type,cur_weather)!=0)
                            {
                                if (j==c)
                                {
                                    unit->x = nx; unit->y = ny;
                                    break;
                                }
                                else
                                    c++;
                            }
            }
            else
            {
                unit->x = x; unit->y = y;
            }
            tile = map_tile(unit->x,unit->y);
            if (tile) tile->g_unit = unit;
        }
        /* chance to die for each parachutist: 6%-exp% */
        if (type == DROP_BY_PARACHUTE)
        {
            int i,c=0,l=6-unit->exp_level;
            for (i=0;i<unit->str;i++)
                if (DICE(100)<=l) 
                    c++;
            unit->str -= c; 
            if (unit->str<=0) unit->str=1; /* have some mercy */
        }
        /* set movement + attacks */
        if (type == EMBARK_AIR)
        {
            unit->cur_mov = unit->sel_prop->mov;
            unit->cur_atk_count = unit->sel_prop->atk_count;
        }
        else
        {
            unit->cur_mov = 0; unit->cur_atk_count = 0; unit->unused = 0;
        }
        /* no entrenchment */
        unit->entr = 0;
        /* adjust pic offset */
        unit_adjust_icon( unit );
        /* free occupied sea transporter */
        cur_player->air_trsp_used--;

        if (enemy_spotted)
            /* in any case your spotting must be updated */
            map_update_spot_mask( unit, enemy_spotted );
        return;
    }
}

/*
====================================================================
Mark this field being a deployment-field for the given player.
====================================================================
*/
void map_set_deploy_field( int mx, int my, int player ) {
    if (!deploy_fields)
        deploy_fields = list_create( LIST_AUTO_DELETE, (void (*)(void*))list_delete );
    else
        list_reset( deploy_fields );
    do {
        MapCoord *pt = 0;
        List *field_list;
        if ( player == 0 ) {
            pt = malloc( sizeof(MapCoord) );
            pt->x = (short)mx; pt->y = (short)my;
        }

        field_list = list_next( deploy_fields );
        if ( !field_list ) {
            field_list = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
            list_add( deploy_fields, field_list );
        }
        if ( pt ) list_add( field_list, pt );
    } while ( player-- );
}

/*
====================================================================
Check whether (mx, my) can serve as a deploy center for the given
unit (assuming it is close to it, which is not checked). If no unit
is given, check whether it is any
====================================================================
*/
static int map_check_deploy_center( Player *player, Unit *unit, int mx, int my )
{
    if (map[mx][my].deploy_center!=1||!map[mx][my].player) return 0;
    if (!player_is_ally(map[mx][my].player,player)) return 0;
    if (unit)
    {
        if (terrain_get_mov(map[mx][my].terrain,unit->sel_prop->mov_type,cur_weather)==0)
            return 0;
        if (unit->sel_prop->flags&FLYING)
            if (!(map[mx][my].terrain->flags[cur_weather]&SUPPLY_AIR))
                return 0;
        if (map[mx][my].nation!=unit->nation) 
            return 0;
    }
    return 1;
}

/*
====================================================================
Add any deploy center and its surrounding to the deploy mask, if it
can supply 'unit'. If 'unit' is not set, add any deploy center.
====================================================================
*/
static void map_add_deploy_centers_to_deploy_mask( Player *player, Unit *unit )
{
    int x, y, i, next_x, next_y;
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if (map_check_deploy_center(player,unit,x,y))
            {
                mask[x][y].deploy = 1;
                for (i=0;i<6;i++)
                    if (get_close_hex_pos( x, y, i, &next_x, &next_y ))
                        mask[next_x][next_y].deploy = 1;
            }
}

/*
====================================================================
Add the default initial deploy mask: all supply centers and their
surrounding (non-airfields for aircrafts too), all own units and
any close hex, if such a hex is not blocked or close to an enemy 
unit, a river or close to an unspotted tile. Ships may not be 
deployed, so water is always off. Screws the deploy mask!
====================================================================
*/
static MapCoord *map_create_new_coord(int x, int y)
{
    MapCoord *pt = calloc(1,sizeof(MapCoord));
    pt->x = x; pt->y = y;
    return pt;
}
static void map_add_default_deploy_fields( Player *player, List *fields )
{
    int i,j,x,y,next_x,next_y,okay;
    Unit *unit;
    list_reset(units);
    while ((unit=list_next(units)))
    {
        if (unit->player == player && unit_supports_deploy(unit))
        {
            for (i=0;i<6;i++)
                if (get_close_hex_pos(unit->x,unit->y,i,&next_x,&next_y))
                {
                    okay = 1;
                    for (j=0;j<6;j++)
                        if (get_close_hex_pos(next_x,next_y,j,&x,&y))
                            if (!mask[x][y].spot||
                                (map[x][y].a_unit&&!player_is_ally(player,map[x][y].a_unit->player))||
                                (map[x][y].g_unit&&!player_is_ally(player,map[x][y].g_unit->player)))
                            {
                                okay = 0; break;
                            }
                    if (map[next_x][next_y].terrain->flags[cur_weather]&RIVER) okay = 0;
                    mask[next_x][next_y].deploy = okay;
                }
        }
    }
    list_reset(units);
    while ((unit=list_next(units))) /* make sure all units can be re-deployed */
        if (unit->player == player && unit_supports_deploy(unit))
            mask[unit->x][unit->y].deploy = 1;
    map_add_deploy_centers_to_deploy_mask(player,0);
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ )
            if (mask[x][y].deploy)
                list_add(fields,map_create_new_coord(x,y));
}

/*
====================================================================
Set deploy mask by player's field list. If first entry is (-1,-1),
create a default mask, using the initial layout of spotting and 
units.
====================================================================
*/
static void map_set_initial_deploy_mask( Player *player )
{
    int i = player_get_index(player);
    List *field_list;
    MapCoord *pt;
    if ( !deploy_fields ) return;
    list_reset( deploy_fields );
    while ( ( field_list = list_next( deploy_fields ) ) && i-- );
    if ( !field_list ) return;

    list_reset( field_list );
    while ( ( pt = list_next( field_list ) ) ) {
        Mask_Tile *tile = map_mask_tile(pt->x, pt->y);
        if (!tile) 
        {
            list_delete_current(field_list);
            map_add_default_deploy_fields(player,field_list);
        }
        else {
            tile->deploy = 1;
            tile->spot = 1;
        }
    }
}

/*
====================================================================
Set the deploy mask for this unit. If 'init', use the initial deploy
mask (or a default one). If not, set the valid deploy centers. In a
second run, remove any tile blocked by an own unit if 'unit' is set.
====================================================================
*/
void map_get_deploy_mask( Player *player, Unit *unit, int init )
{
    int x, y;
    assert( unit || init );
    map_clear_mask( F_DEPLOY );
    if (init)
        map_set_initial_deploy_mask(player);
    else
        map_add_deploy_centers_to_deploy_mask(player, unit);
    if (unit)
    {
        for ( x = 0; x < map_w; x++ )
            for ( y = 0; y < map_h; y++ )
                if (unit->sel_prop->flags&FLYING)
                {
                    if (map[x][y].a_unit) mask[x][y].deploy = 0;
                } else {
                    if (map[x][y].g_unit&&(!init||!map_check_unit_embark(unit,x,y,EMBARK_AIR,1))) 
                        mask[x][y].deploy = 0;
                }
    }
}

/*
====================================================================
Check whether a unit can be undeployed. If 'air_region' check 
aircrafts only, otherwise ground units only.
====================================================================
*/
Unit* map_get_undeploy_unit( int x, int y, int air_region )
{
    if ( air_region ) {
        /* check air */
        if ( map[x][y].a_unit &&  map[x][y].a_unit->fresh_deploy )
            return  map[x][y].a_unit;
        else
            /*if ( map[x][y].g_unit && map[x][y].g_unit->fresh_deploy )
                return  map[x][y].g_unit;
            else*/
                return 0;
    }
    else {
        /* check ground */
        if ( map[x][y].g_unit &&  map[x][y].g_unit->fresh_deploy )
            return  map[x][y].g_unit;
        else
            /*if ( map[x][y].a_unit &&  map[x][y].a_unit->fresh_deploy )
                return  map[x][y].a_unit;
            else*/
                return 0;
    }
}

/*
====================================================================
Check the supply level of tile (mx, my) in the context of 'unit'.
(hex tiles with SUPPLY_GROUND have 100% supply)
====================================================================
*/
int map_get_unit_supply_level( int mx, int my, Unit *unit )
{
    int x, y, w, h, i, j;
    int flag_supply_level, supply_level;
    /* flying and swimming units get a 100% supply if near an airfield or a harbour */
    if ( ( unit->sel_prop->flags & SWIMMING )
         || ( unit->sel_prop->flags & FLYING ) ) {
        supply_level = map_supplied_by_depot( mx, my, unit )*100;
    }
    else {
        /* ground units get a 100% close to a flag and looses about 10% for each title it gets away */
        /* test all flags within a region x-10,y-10,20,20 about their distance */
        /* get region first */
        x = mx - 10; y = my - 10; w = 20; h = 20;
        if ( x < 0 ) { w += x; x = 0; }
        if ( y < 0 ) { h += y; y = 0; }
        if ( x + w > map_w ) w = map_w - x;
        if ( y + h > map_h ) h = map_h - y;
        /* now check flags */
        supply_level = 0;
        for ( i = x; i < x + w; i++ )
            for ( j = y; j < y + h; j++ )
                if ( map[i][j].player && player_is_ally( unit->player, map[i][j].player ) ) {
                    flag_supply_level = get_dist( mx, my, i, j );
                    if ( flag_supply_level < 2 ) flag_supply_level = 100;
                    else {
                        flag_supply_level = 100 - ( flag_supply_level - 1 ) * 10;
                        if ( flag_supply_level < 0 ) flag_supply_level = 0;
                    }
                    if ( flag_supply_level > supply_level )
                        supply_level = flag_supply_level;
                }
    }
    /* air: if hostile influence is 1 supply is 50%, if influence >1 supply is not possible */
    /* ground: if hostile influence is 1 supply is at 75%, if influence >1 supply is at 50% */
    if ( unit->sel_prop->flags & FLYING ) {
        if ( mask[ mx][ my ].air_infl > 1 || mask[ mx][ my ].infl > 1 )
            supply_level = 0;
        else
            if ( mask[ mx][my ].air_infl == 1 || mask[ mx][ my ].infl == 1 )
                supply_level = supply_level / 2;
    }
    else {
        if ( mask[ mx][ my ].infl == 1 )
            supply_level = 3 * supply_level / 4;
        else
            if ( mask[ mx][ my ].infl > 1 )
                supply_level = supply_level / 2;
    }
    return supply_level;
}

/*
====================================================================
Check if this map tile is a supply point for the given unit.
====================================================================
*/
int map_is_allied_depot( Map_Tile *tile, Unit *unit )
{
    if ( tile == 0 ) return 0;
    /* maybe it's an aircraft carrier */
    if ( tile->g_unit )
        if ( tile->g_unit->sel_prop->flags & CARRIER )
            if ( player_is_ally( tile->g_unit->player, unit->player ) )
                if ( unit->sel_prop->flags & CARRIER_OK )
                    return 1;
    /* check for depot */
    if ( tile->player == 0 ) return 0;
    if ( !player_is_ally( unit->player, tile->player ) ) return 0;
    if ( unit->sel_prop->flags & FLYING ) {
        if ( !(tile->terrain->flags[cur_weather] & SUPPLY_AIR) )
            return 0;
    }
    else
        if ( unit->sel_prop->flags & SWIMMING ) {
            if ( !(tile->terrain->flags[cur_weather] & SUPPLY_SHIPS) )
                return 0;
        }
	if (tile->supply_center > 1) // is currently damaged
		return 0;
    return 1;
}
/*
====================================================================
Checks whether this hex (mx, my) is supplied by a depot in the
context of 'unit'.
====================================================================
*/
int map_supplied_by_depot( int mx, int my, Unit *unit )
{
    int i;
    if ( map_is_allied_depot( &map[mx][my], unit ) )
        return 1;
    for ( i = 0; i < 6; i++ )
        if ( map_is_allied_depot( map_get_close_hex( mx, my, i ), unit ) )
            return 1;
    return 0;
}

/*
====================================================================
Get drop zone for unit (all close hexes that are free).
====================================================================
*/
void map_get_dropzone_mask( Unit *unit )
{
    int i,x,y;
    map_clear_mask(F_DEPLOY);
    for (i=0;i<6;i++)
        if (get_close_hex_pos(unit->x,unit->y,i,&x,&y))
            if (map[x][y].g_unit==0)
                if (terrain_get_mov(map[x][y].terrain,unit->prop.mov_type,cur_weather)!=0)
                    mask[x][y].deploy = 1;
    if (map[unit->x][unit->y].g_unit==0)
        if (terrain_get_mov(map[unit->x][unit->y].terrain,unit->prop.mov_type,cur_weather)!=0)
            mask[unit->x][unit->y].deploy = 1; 
}

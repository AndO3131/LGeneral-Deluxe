/***************************************************************************
                          map.h  -  description
                             -------------------
    begin                : Sat Jan 20 2001
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

#ifndef __MAP_H
#define __MAP_H

#include "terrain.h"

enum { FAIR = 0, CLOUDS, RAIN, SNOW };

/*
====================================================================
Map tile
====================================================================
*/
typedef struct {
    char *name;             /* name of this map tile */
    Terrain_Type *terrain;  /* terrain properties */
    int terrain_id;         /* id of terrain properties */
    int image_offset;       /* image offset in prop->image */
    int strat_image_offset; /* offset in the list of strategic tiny terrain images */
    Nation *nation;         /* nation that owns this flag (NULL == no nation) */
    Player *player;         /* dito */
    int obj;                /* military objective ? */
    int deploy_center;      /* deploy allowed? */
    int supply_center;
    Unit *g_unit;           /* ground/naval unit pointer */
    Unit *a_unit;           /* air unit pointer */
} Map_Tile;

/*
====================================================================
To determine various things of the map (deploy, spot, blocked ...)
a map mask is used and these are the flags for it.
====================================================================
*/
enum {
    F_FOG =             ( 1L << 1 ),
    F_SPOT =            ( 1L << 2 ),
    F_IN_RANGE =        ( 1L << 3 ),
    F_MOUNT =           ( 1L << 4 ),
    F_SEA_EMBARK =      ( 1L << 5 ),
    F_AUX =             ( 1L << 6 ),
    F_INFL =            ( 1L << 7 ),
    F_INFL_AIR =        ( 1L << 8 ),
    F_VIS_INFL =        ( 1L << 9 ),
    F_VIS_INFL_AIR =    ( 1L << 10 ),
    F_BLOCKED =         ( 1L << 11 ),
    F_BACKUP =          ( 1L << 12 ),
    F_MERGE_UNIT =      ( 1L << 13),
    F_INVERSE_FOG =     ( 1L << 14 ), /* inversion of F_FOG */
    F_DEPLOY =          ( 1L << 15 ),
    F_CTRL_GRND =       ( 1L << 17 ),
    F_CTRL_AIR =        ( 1L << 18 ),
    F_CTRL_SEA =        ( 1L << 19 ),
    F_MOVE_COST =       ( 1L << 20 ),
    F_DANGER =          ( 1L << 21 ),
    F_SPLIT_UNIT =      ( 1L << 22 ),
    F_DISTANCE =        ( 1L << 23 )
};

/*
====================================================================
Map mask tile.
====================================================================
*/
typedef struct {
    int fog; /* if true the engine covers this tile with fog. if ENGINE_MODIFY_FOG is set
                this fog may change depending on the action (range of unit, merge partners
                etc */
    int spot; /* true if any of your units observes this map tile; you can only attack units
    on a map tile that you spot */
    /* used for a selected unit */
    int in_range; /* this is used for pathfinding; it's -1 if tile isn't in range else it's set to the
    remaining moving points of the unit; enemy influence is not included */
    int distance; /* mere distance to current unit; used for danger mask */
    int moveCost; /* total costs to reach this tile */
    int blocked; /* units can move over there tiles with an allied unit but they must not stop there;
    so allow movment to a tile only if in_range and !blocked */
    int mount; /* true if unit must mount to reach this tile */
    int sea_embark; /* sea embark possible? */
    int infl; /* at the beginning of a player's turn this mask is set; each tile close to a
    hostile unit gets infl increased; if influence is 1 moving costs are doubled; if influence is >=2
    this tile is impassible (unit stops at this tile); if a unit can't see a tile with infl >= 2 and
    tries to move there it will stop on this tile; independed from a unit's moving points passing an
    influenced tile costs all mov-points */
    int vis_infl; /* analogue to infl but only spotted units contribute to this mask; used to setup
    in_range mask */
    int air_infl; /* analouge for flying units */
    int vis_air_infl;
    int aux; /* used to setup any of the upper values */
    int backup; /* used to backup spot mask for undo unit move */
    Unit *merge_unit; /* if not NULL this is a pointer to a unit the one who called map_get_merge_units()
                         may merge with. you'll need to remember the other unit as it is not saved here */
    Unit *split_unit; /* target unit may transfer strength to */
    int  split_okay; /* unit may transfer a new subunit to this tile */
    int deploy; /* deploy mask: 1: unit may deploy their, 0 unit may not deploy their; setup by deploy.c */
    int danger; /* 1: mark this tile as being dangerous to enter */
    /* AI masks */
    int ctrl_grnd; /* mask of controlled area for a player. own units give there positive combat
                      value in move+attack range while enemy scores are substracted. the final
                      value for each tile is relative to the highest absolute control value thus
                      it ranges from -1000 to 1000 */
    int ctrl_air;
    int ctrl_sea; /* each operational region has it's own control mask */
} Mask_Tile;


/*
====================================================================
Load map.
====================================================================
*/
int map_load( char *fname );

/*
====================================================================
Delete map.
====================================================================
*/
void map_delete( );

/*
====================================================================
Get tile at x,y
====================================================================
*/
Map_Tile* map_tile( int x, int y );
Mask_Tile* map_mask_tile( int x, int y );

/*
====================================================================
Clear the passed map mask flags.
====================================================================
*/
void map_clear_mask( int flags );

/*
====================================================================
Swap units. Returns the previous unit or 0 if none.
====================================================================
*/
Unit *map_swap_unit( Unit *unit );

/*
====================================================================
Insert, Remove unit pointer from map.
====================================================================
*/
void map_insert_unit( Unit *unit );
void map_remove_unit( Unit *unit );

/*
====================================================================
Get neighbored tiles clockwise with id between 0 and 5.
====================================================================
*/
Map_Tile* map_get_close_hex( int x, int y, int id );

/*
====================================================================
Add/set spotting of a unit to auxiliary mask
====================================================================
*/
void map_add_unit_spot_mask( Unit *unit );
void map_get_unit_spot_mask( Unit *unit );

/*
====================================================================
Set movement range of a unit to in_range/sea_embark/mount.
====================================================================
*/
void map_get_unit_move_mask( Unit *unit );
void map_clear_unit_move_mask();

/*
====================================================================
Recreates the danger mask for 'unit'.
The fog must be set to the movement range of 'unit' for this
function to work properly.
The movement cost of the mask must have been set for 'unit'.
Returns 1 when at least one tile's danger mask was set, otherwise 0.
====================================================================
*/
int map_get_danger_mask( Unit *unit );

/*
====================================================================
Get a list of way points the unit moves along to it's destination.
This includes check for unseen influence by enemy units (e.g.
Surprise Contact).
====================================================================
*/
typedef struct {
    int x, y;
} Way_Point;
Way_Point* map_get_unit_way_points( Unit *unit, int x, int y, int *count, Unit **ambush_unit );

/*
====================================================================
Backup/restore spot mask to/from backup mask. Used for Undo Turn.
====================================================================
*/
void map_backup_spot_mask();
void map_restore_spot_mask();

/*
====================================================================
Get unit's merge partners and set mask 'merge'.
At maximum MAP_MERGE_UNIT_LIMIT units.
All unused entries in partners are set 0.
====================================================================
*/
enum { MAP_MERGE_UNIT_LIMIT = 6 };
void map_get_merge_units( Unit *unit, Unit **partners, int *count );

/*
====================================================================
Check if unit may transfer strength to unit (if not NULL) or create
a stand alone unit (if unit NULL) on the coordinates.
====================================================================
*/
int map_check_unit_split( Unit *unit, int str, int x, int y, Unit *dest );

/*
====================================================================
Get unit's split partners assuming unit wants to give 'str' strength
points and set mask 'split'. At maximum MAP_SPLIT_UNIT_LIMIT units.
All unused entries in partners are set 0. 'str' must be valid amount,
this is not checked here.
====================================================================
*/
enum { MAP_SPLIT_UNIT_LIMIT = 6 };
void map_get_split_units_and_hexes( Unit *unit, int str, Unit **partners, int *count );

/*
====================================================================
Get a list (vis_units) of all visible units by checking spot mask.
====================================================================
*/
void map_get_vis_units( void );

/*
====================================================================
Draw a map tile terrain to surface. (fogged if mask::fog is set)
====================================================================
*/
void map_draw_terrain( SDL_Surface *surf, int map_x, int map_y, int x, int y );
/*
====================================================================
Draw tile units. If mask::fog is set no units are drawn.
If 'ground' is True the ground unit is drawn as primary
and the air unit is drawn small (and vice versa).
If 'select' is set a selection frame is added.
====================================================================
*/
void map_draw_units( SDL_Surface *surf, int map_x, int map_y, int x, int y, int ground, int select );
/*
====================================================================
Draw danger tile. Expects 'surf' to contain a fully drawn tile at
the given position which will be tinted by overlaying the danger
terrain surface.
====================================================================
*/
void map_apply_danger_to_tile( SDL_Surface *surf, int map_x, int map_y, int x, int y );
/*
====================================================================
Draw terrain and units.
====================================================================
*/
void map_draw_tile( SDL_Surface *surf, int map_x, int map_y, int x, int y, int ground, int select );

/*
====================================================================
Set/update spot mask of by engine's current player.
The update adds the tiles seen by unit.
====================================================================
*/
void map_set_spot_mask();
void map_update_spot_mask( Unit *unit, int *enemy_spotted );

/*
====================================================================
Set mask::fog (which is the actual fog of the engine) to either
spot mask, in_range mask (covers sea_embark), merge mask,
deploy mask.
====================================================================
*/
void map_set_fog( int type );

/*
====================================================================
Set the fog to players spot mask by using mask::aux (not mask::spot)
====================================================================
*/
void map_set_fog_by_player( Player *player );

/*
====================================================================
Check if this map tile is visible to the engine (isn't covered
by mask::fog or mask::spot as modification is allowed and it may be
another's player fog (e.g. one human against cpu))
====================================================================
*/
#define MAP_CHECK_VIS( mapx, mapy ) ( ( !modify_fog && !mask[mapx][mapy].fog ) || ( modify_fog && mask[mapx][mapy].spot ) )

/*
====================================================================
Modify the various influence masks.
====================================================================
*/
void map_add_unit_infl( Unit *unit );
void map_remove_unit_infl( Unit *unit );
void map_remove_vis_unit_infl( Unit *unit );
void map_set_infl_mask();
void map_set_vis_infl_mask();

/*
====================================================================
Check if unit may air/sea embark/debark at x,y.
If 'init' != 0, used relaxed rules for deployment
====================================================================
*/
int map_check_unit_embark( Unit *unit, int x, int y, int type, int init );
int map_check_unit_debark( Unit *unit, int x, int y, int type, int init );

/*
====================================================================
Embark/debark unit and return if an enemy was spotted.
If 'enemy_spotted' is 0, don't recalculate spot mask.
If unit's coordinates or x and y are out of bounds, the respective
tile is not manipulated.
====================================================================
*/
void map_embark_unit( Unit *unit, int x, int y, int type, int *enemy_spotted );
void map_debark_unit( Unit *unit, int x, int y, int type, int *enemy_spotted );

/*
====================================================================
Set the deploy mask for this unit. If 'init', use the initial deploy
mask (or a default one). If not, set the valid deploy centers. In a
second run, remove any tile blocked by an own unit if 'unit' is set.
====================================================================
*/
void map_get_deploy_mask( Player *player, Unit *unit, int init );

/*
====================================================================
Mark this field being a deployment-field for the given player.
====================================================================
*/
void map_set_deploy_field( int mx, int my, int player );

/*
====================================================================
Check whether this field is a deployment-field for the given player.
====================================================================
*/
int map_is_deploy_field( int mx, int my, int player );

/*
====================================================================
Check if unit may be deployed to mx, my or return undeployable unit
there. If 'air_mode' is set the air unit is checked first.
'player' is the index of the player.
====================================================================
*/
int map_check_deploy( Unit *unit, int mx, int my, int player, int init, int air_mode );
Unit* map_get_undeploy_unit( int x, int y, int air_region );

/*
====================================================================
Check the supply level of tile (mx, my) in the context of 'unit'.
(hex tiles with SUPPLY_GROUND have 100% supply)
====================================================================
*/
int map_get_unit_supply_level( int mx, int my, Unit *unit );
/*
====================================================================
Check if this map tile is a supply point for the given unit.
====================================================================
*/
int map_is_allied_depot( Map_Tile *tile, Unit *unit );
/*
====================================================================
Checks whether this hex (mx, my) is supplied by a depot in the
context of 'unit'.
====================================================================
*/
int map_supplied_by_depot( int mx, int my, Unit *unit );

/*
====================================================================
Get drop zone for unit (all close hexes that are free).
====================================================================
*/
void map_get_dropzone_mask( Unit *unit );

#endif

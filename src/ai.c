/***************************************************************************
                          ai.c  -  description
                             -------------------
    begin                : Thu Apr 11 2002
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
#include "date.h"
#include "unit.h"
#include "action.h"
#include "map.h"
#include "ai_tools.h"
#include "ai_group.h"
#include "ai.h"
#include "scenario.h"

#include <assert.h>

/*
====================================================================
Externals
====================================================================
*/
extern Player *cur_player;
extern List *units, *avail_units, *unit_lib;
extern int map_w, map_h;
extern Map_Tile **map;
extern Mask_Tile **mask;
extern int trgt_type_count;
extern int deploy_turn;
extern Nation *nations;
extern Scen_Info *scen_info;
extern Config config;

/*
====================================================================
Internal stuff
====================================================================
*/
enum {
    AI_STATUS_INIT = 0, /* initiate turn */
    AI_STATUS_FINALIZE, /* finalize turn */
    AI_STATUS_DEPLOY,   /* deploy new units */
    AI_STATUS_SUPPLY,   /* supply units that need it */
    AI_STATUS_MERGE,    /* merge damaged units */
    AI_STATUS_GROUP,    /* group and handle other units */
    AI_STATUS_PURCHASE, /* order new units */
    AI_STATUS_END       /* end ai turn */
};
static int ai_status = AI_STATUS_INIT; /* current AI status */
static List *ai_units = 0; /* units that must be processed */
static AI_Group *ai_group = 0; /* for now it's only one group */
static int finalized = 1; /* set to true when finalized */

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Get the supply level that is needed to get the wanted absolute
values where ammo or fuel 0 means not to check this value.
====================================================================
*/
#ifdef _1
static int unit_get_supply_level( Unit *unit, int abs_ammo, int abs_fuel )
{
    int ammo_level = 0, fuel_level = 0, miss_ammo, miss_fuel;
    unit_check_supply( unit, UNIT_SUPPLY_ANYTHING, &miss_ammo, &miss_fuel );
    if ( abs_ammo > 0 && miss_ammo > 0 ) {
        if ( miss_ammo > abs_ammo ) miss_ammo = abs_ammo;
        ammo_level = 100 * unit->prop.ammo / miss_ammo;
    }
    if ( abs_fuel > 0 && miss_fuel > 0 ) {
        if ( miss_fuel > abs_fuel ) miss_fuel = abs_fuel;
        ammo_level = 100 * unit->prop.fuel / miss_fuel;
    }
    if ( fuel_level > ammo_level ) 
        return fuel_level;
    return ammo_level;
}
#endif

/*
====================================================================
Get the distance of 'unit' and position of object of a special
type.
====================================================================
*/
static int ai_check_hex_type( Unit *unit, int type, int x, int y )
{
    switch ( type ) {
        case AI_FIND_DEPOT:
            if ( map_is_allied_depot( &map[x][y], unit ) )
                return 1;
            break;
        case AI_FIND_ENEMY_OBJ:
            if ( !map[x][y].obj ) return 0;
        case AI_FIND_ENEMY_TOWN:
            if ( map[x][y].player && !player_is_ally( unit->player, map[x][y].player ) )
                return 1;
            break;
        case AI_FIND_OWN_OBJ:
            if ( !map[x][y].obj ) return 0;
        case AI_FIND_OWN_TOWN:
            if ( map[x][y].player && player_is_ally( unit->player, map[x][y].player ) )
                return 1;
            break;
    }
    return 0;
}
int ai_get_dist( Unit *unit, int x, int y, int type, int *dx, int *dy, int *dist )
{
    int found = 0, length;
    int i, j;
    *dist = 999;
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ )
            if ( ai_check_hex_type( unit, type, i, j ) ) {
                length = get_dist( i, j, x, y );
                if ( *dist > length ) {
                    *dist = length;
                    *dx = i; *dy = j;
                    found = 1;
                }
            }
    return found;
}

/*
====================================================================
Approximate destination by best move position in range.
====================================================================
*/
static int ai_approximate( Unit *unit, int dx, int dy, int *x, int *y )
{
    int i, j, dist = get_dist( unit->x, unit->y, dx, dy ) + 1;
    *x = unit->x; *y = unit->y;
    map_clear_mask( F_AUX );
    map_get_unit_move_mask( unit );
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ )
            if ( mask[i][j].in_range && !mask[i][j].blocked )
                mask[i][j].aux = get_dist( i, j, dx, dy ) + 1;
    for ( i = 0; i < map_w; i++ )
        for ( j = 0; j < map_h; j++ )
            if ( dist > mask[i][j].aux && mask[i][j].aux > 0 ) {
                dist = mask[i][j].aux;
                *x = i; *y = j;
            }
    return ( *x != unit->x || *y != unit->y );
}

/*
====================================================================
Evaluate all units by not only checking the props but also the 
current values.
====================================================================
*/
static int get_rel( int value, int limit )
{
    return 1000 * value / limit;
}
static void ai_eval_units()
{
    Unit *unit;
    list_reset( units );
    while ( ( unit = list_next( units ) ) ) {
        if ( unit->killed ) continue;
        unit->eval_score = 0;
        if ( unit->prop.ammo > 0 ) {
            if ( unit->cur_ammo >= 5 )
                /* it's extremly unlikely that there'll be more than
                   five attacks on a unit within one turn so we
                   can consider a unit with 5+ ammo 100% ready for 
                   battle */
                unit->eval_score += 1000;
            else
                unit->eval_score += get_rel( unit->cur_ammo, 
                                             MINIMUM( 5, unit->prop.ammo ) );
        }
        if ( unit->prop.fuel > 0 ) {
            if ( (  (unit->sel_prop->flags & FLYING) && unit->cur_fuel >= 20 ) ||
                 ( !(unit->sel_prop->flags & FLYING) && unit->cur_fuel >= 10 ) )
                /* a unit with this range is considered 100% operable */
                unit->eval_score += 1000;
            else {
                if ( unit->sel_prop->flags & FLYING )
                    unit->eval_score += get_rel( unit->cur_fuel, MINIMUM( 20, unit->prop.fuel ) );
                else
                    unit->eval_score += get_rel( unit->cur_fuel, MINIMUM( 10, unit->prop.fuel ) );
            }
        }
        unit->eval_score += unit->exp_level * 250;
        unit->eval_score += unit->str * 200; /* strength is counted doubled */
        /* the unit experience is not counted as normal but gives a bonus
           that may increase the evaluation */
        unit->eval_score /= 2 + (unit->prop.ammo > 0) + (unit->prop.fuel > 0);
        /* this value between 0 and 1000 indicates the readiness of the unit
           and therefore the permillage of eval_score */
        unit->eval_score = unit->eval_score * unit->prop.eval_score / 1000;
    }
}

/*
====================================================================
Set control mask for ground/air/sea for all units. (not only the 
visible ones) Friends give positive and foes give negative score
which is a relative the highest control value and ranges between
-1000 and 1000.
====================================================================
*/
typedef struct {
    Unit *unit;
    int score;
    int op_region; /* 0 - ground, 1 - air, 2 - sea */
} CtrlCtx;
static int ai_eval_ctrl( int x, int y, void *_ctx )
{
    CtrlCtx *ctx = _ctx;
    /* our main function ai_get_ctrl_masks() adds the score
       for all tiles in range and as we only want to add score
       once we have to check only tiles in attack range that
       are not situated in the move range */
    if ( mask[x][y].in_range )
        return 1;
    /* okay, this is fire range but not move range */
    switch ( ctx->op_region ) {
        case 0: mask[x][y].ctrl_grnd += ctx->score; break;
        case 1: mask[x][y].ctrl_air += ctx->score; break;
        case 2: mask[x][y].ctrl_sea += ctx->score; break;
    }
    return 1;
}
void ai_get_ctrl_masks()
{
    CtrlCtx ctx;
    int i, j;
    Unit *unit;
    map_clear_mask( F_CTRL_GRND | F_CTRL_AIR | F_CTRL_SEA );
    list_reset( units );
    while ( ( unit = list_next( units ) ) ) {
        if ( unit->killed ) continue;
        map_get_unit_move_mask( unit );
        /* build context */
        ctx.unit = unit;
        ctx.score = (player_is_ally( unit->player, cur_player ))?unit->eval_score:-unit->eval_score;
        ctx.op_region = (unit->sel_prop->flags&FLYING)?1:(unit->sel_prop->flags&SWIMMING)?2:0;
        /* work through move mask and modify ctrl mask by adding score
           for all tiles in movement and attack range once */
        for ( i  = MAXIMUM( 0, unit->x - unit->cur_mov ); 
              i <= MINIMUM( map_w - 1, unit->x + unit->cur_mov );
              i++ )
            for ( j  = MAXIMUM( 0, unit->y - unit->cur_mov ); 
                  j <= MINIMUM( map_h - 1, unit->y + unit->cur_mov ); 
                  j++ )
                if ( mask[i][j].in_range ) {
                    switch ( ctx.op_region ) {
                        case 0: mask[i][j].ctrl_grnd += ctx.score; break;
                        case 1: mask[i][j].ctrl_air += ctx.score; break;
                        case 2: mask[i][j].ctrl_sea += ctx.score; break;
                    }
                    ai_eval_hexes( i, j, MAXIMUM( 1, unit->sel_prop->rng ), 
                                   ai_eval_ctrl, &ctx );
                }
    }
}

/** Find unit in reduced unit lib entry list @ulist which has flag @flag set
 * and is best regarding ratio of eval score and cost. Return NULL if no such
 * unit found. */
Unit_Lib_Entry *get_cannonfodder( List *ulist, int flag )
{
	Unit_Lib_Entry *e, *u = NULL;
	
	list_reset(ulist);
	while ((e = list_next(ulist))) {
		if (!(e->flags & flag))
			continue;
		if (u == NULL) {
			u = e;
			continue;
		}
		if ((100*e->eval_score)/e->cost > (100*u->eval_score)/u->cost)
			u = e;
	}
	return u;
}
	
/** Find unit in reduced unit lib entry list @ulist which has flag @flag set
 * and is best regarding eval score slightly penalized by high cost.
 * Return NULL if no such unit found. */
Unit_Lib_Entry *get_good_unit( List *ulist, int flag )
{
	Unit_Lib_Entry *e, *u = NULL;
	
	list_reset(ulist);
	while ((e = list_next(ulist))) {
		if (!(e->flags & flag))
			continue;
		if (u == NULL) {
			u = e;
			continue;
		}
		if (e->eval_score-(e->cost/5) > u->eval_score-(u->cost/5))
			u = e;
	}
	return u;
}

/** Purchase new units (shamelessly use functions from purchase_dlg for this).*/
extern int player_get_purchase_unit_limit( Player *player );
extern List *get_purchase_nations( void );
extern List *get_purchasable_unit_lib_entries( const char *nationid, 
				const char *uclassid, const Date *date );
extern int player_can_purchase_unit( Player *p, Unit_Lib_Entry *unit,
							Unit_Lib_Entry *trsp);
extern void player_purchase_unit( Player *player, Nation *nation,
			Unit_Lib_Entry *unit_prop, Unit_Lib_Entry *trsp_prop );
static void ai_purchase_units()
{
	List *nlist = NULL; /* list of player's nations */
	List *ulist = NULL; /* list of purchasable unit lib entries */
	Unit_Lib_Entry *e = NULL;
	Nation *n = NULL;
	int ulimit = player_get_purchase_unit_limit(cur_player);
	struct {
		Unit_Lib_Entry *unit; /* unit to be bought */
		Unit_Lib_Entry *trsp; /* its transporter if not NULL */
		int weight; /* relative weight to other units in list */
	} buy_options[5];
	int num_buy_options;
	int maxeyes, i, j;
	
	AI_DEBUG( 1, "Prestige: %d, Units available: %d\n", 
					cur_player->cur_prestige, ulimit);
	
	/* if no capacity, return */
	if (ulimit == 0)
		return;
	
	ulist = get_purchasable_unit_lib_entries( NULL, NULL, 
						&scen_info->start_date);
	nlist = get_purchase_nations();
	
	/* remove all entries from ulist that do not have one of our nations */
	list_reset(ulist);
	while ((e = list_next(ulist))) {
		if (e->nation == -1) {
			list_delete_current(ulist); /* not purchasable */
			continue;
		}
		list_reset(nlist);
		while ((n = list_next(nlist)))
			if (&nations[e->nation] == n)
				break;
		if (n == NULL) {
			list_delete_current(ulist); /* not our nation */
			continue;
		}
		AI_DEBUG(2, "%s can be purchased (score: %d, cost: %d)\n",
						e->name,e->eval_score,e->cost);
	}
	
	memset(buy_options,0,sizeof(buy_options));
	num_buy_options = 0;
	
	/* FIXME: this certainly has to be more detailled and complex but for
	 * now, keep it simple:
	 * 
	 * for defense: buy cheapiest infantry (30%), antitank (30%), 
	 * airdefense (20%) or tank (20%) 
	 * for attack: buy good tank (40%), infantry with transporter (15%),
	 * good artillery (15%), fighter (10%) or bomber (20%) */
	if (cur_player->strat < 0) {
		buy_options[0].unit = get_cannonfodder( ulist, INFANTRY );
		buy_options[0].weight = 30;
		buy_options[1].unit = get_cannonfodder( ulist, ANTI_TANK );
		buy_options[1].weight = 30;
		buy_options[2].unit = get_cannonfodder( ulist, AIR_DEFENSE );
		buy_options[2].weight = 20;
		buy_options[3].unit = get_cannonfodder( ulist, TANK );
		buy_options[3].weight = 20;
		num_buy_options = 4;
	} else {
		buy_options[0].unit = get_good_unit( ulist, INFANTRY );
		buy_options[0].weight = 15;
		buy_options[1].unit = get_good_unit( ulist, TANK );
		buy_options[1].weight = 40;
		buy_options[2].unit = get_good_unit( ulist, ARTILLERY );
		buy_options[2].weight = 15;
		buy_options[3].unit = get_good_unit( ulist, INTERCEPTOR );
		buy_options[3].weight = 10;
		buy_options[4].unit = get_good_unit( ulist, BOMBER );
		buy_options[4].weight = 20;
		num_buy_options = 5;
	}
	
	/* get "size of dice" :-) */
	maxeyes = 0;
	for (i = 0; i < num_buy_options; i++) {
		maxeyes += buy_options[i].weight;
		AI_DEBUG(1, "Purchase option %d (w=%d): %s%s\n", i+1,
					buy_options[i].weight, 
					buy_options[i].unit?
					buy_options[i].unit->name:"empty",
					buy_options[i].trsp?
					buy_options[i].trsp->name:"");
		if (buy_options[i].unit == NULL) {
			/* this is only the case for the very first scenarios
			 * to catch it, simply fall back to infantry */
			AI_DEBUG(1, "Option %d empty (use option 1)\n", i+1);
			buy_options[i].unit = buy_options[0].unit;
		}
	}
	
	/* try to buy units; if not possible (cost exceeds prestige) do nothing
	 * (= save prestige and try to buy next turn) */
	for (i = 0; i < ulimit; i++) {
		int roll = DICE(maxeyes);
		for (j = 0; j < num_buy_options; j++)
			if (roll > buy_options[j].weight)
				roll -= buy_options[j].weight;
			else 
				break;
		AI_DEBUG(1, "Choosing option %d\n",j+1);
		if (buy_options[j].unit && player_can_purchase_unit( cur_player,
						buy_options[j].unit,
						buy_options[j].trsp)) {
			player_purchase_unit( cur_player, 
				&nations[buy_options[j].unit->nation],
				buy_options[j].unit,buy_options[j].trsp);
			AI_DEBUG(1, "Prestige remaining: %d\n",
						cur_player->cur_prestige);
		} else
			AI_DEBUG(1, "Could not purchase unit?!?\n");
	}
		
	list_delete( nlist );
	list_delete( ulist );
}

/*
====================================================================
Exports
====================================================================
*/

/*
====================================================================
Initiate turn
====================================================================
*/
void ai_init( void )
{
    List *list; /* used to speed up the creation of ai_units */
    Unit *unit;
	
    AI_DEBUG(0, "AI Turn: %s\n", cur_player->name );
    if ( ai_status != AI_STATUS_INIT ) {
        AI_DEBUG(0, "Aborted: Bad AI Status: %i\n", ai_status );
        return;
    }
    
    finalized = 0;
    /* get all cur_player units, those with defensive fire come first */
    list = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    list_reset( units );
    while ( ( unit = list_next( units ) ) )
        if ( unit->player == cur_player )
            list_add( list, unit );
    ai_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    list_reset( list );
    while ( ( unit = list_next( list ) ) )
        if ( unit->sel_prop->flags & ARTILLERY || unit->sel_prop->flags & AIR_DEFENSE ) {
            list_add( ai_units, unit );
            list_delete_item( list, unit );
        }
    list_reset( list );
    while ( ( unit = list_next( list ) ) ) {
        if ( unit->killed ) 
		AI_DEBUG( 0, "!!Unit %s is dead!!\n", unit->name );
        list_add( ai_units, unit );
    }
    list_delete( list );
    list_reset( ai_units );
    AI_DEBUG(1, "Units: %i\n", ai_units->count );
    /* evaluate all units for strategic computations */
    AI_DEBUG( 1, "Evaluating units...\n" );
    ai_eval_units();
    /* build control masks */
    AI_DEBUG( 1, "Building control mask...\n" );
    //ai_get_ctrl_masks();
    /* check new units first */
    ai_status = AI_STATUS_DEPLOY; 
    list_reset( avail_units );
    AI_DEBUG( 0, "AI Initialized\n" );
    AI_DEBUG( 0, "*** DEPLOY ***\n" );
}

/*
====================================================================
Queue next actions (if these actions were handled by the engine
this function is called again and again until the end_turn
action is received).
====================================================================
*/
void ai_run( void )
{
    Unit *partners[MAP_MERGE_UNIT_LIMIT];
    int partner_count;
    int i, j, x, y, dx, dy, dist, found;
    Unit *unit = 0, *best = 0;
    switch ( ai_status ) {
        case AI_STATUS_DEPLOY:
            /* deploy unit? */
            if ( avail_units->count > 0 && ( unit = list_next( avail_units ) ) ) {
                if ( deploy_turn ) {
                    x = unit->x; y = unit->y;
                    assert( x >= 0 && y >= 0 );
                    map_remove_unit( unit );
                    found = 1;
                } else {
                    map_get_deploy_mask(cur_player,unit,0);
                    map_clear_mask( F_AUX );
                    for ( i = 1; i < map_w - 1; i++ )
                        for ( j = 1; j < map_h - 1; j++ )
                            if ( mask[i][j].deploy )
                                if ( ai_get_dist( unit, i, j, AI_FIND_ENEMY_OBJ, &x, &y, &dist ) )
                                    mask[i][j].aux = dist + 1;
                    dist = 1000; found = 0;
                    for ( i = 1; i < map_w - 1; i++ )
                        for ( j = 1; j < map_h - 1; j++ )
                            if ( mask[i][j].aux > 0 && mask[i][j].aux < dist ) {
                                dist = mask[i][j].aux;
                                x = i; y = j;
                                found = 1; /* deploy close to enemy */
                            }
                }
                if ( found ) {
                    action_queue_deploy( unit, x, y );
                    list_reset( avail_units );
                    list_add( ai_units, unit );
                    AI_DEBUG( 1, "%s deployed to %i,%i\n", unit->name, x, y );
                    return;
                }
            }
            else {
                ai_status = deploy_turn ? AI_STATUS_END : AI_STATUS_MERGE;
                list_reset( ai_units );
                AI_DEBUG( 0, deploy_turn ? "*** END TURN ***\n" : 
							"*** MERGE ***\n" );
            }
            break;
        case AI_STATUS_SUPPLY:
            /* get next unit */
            if ( ( unit = list_next( ai_units ) ) == 0 ) {
                ai_status = AI_STATUS_GROUP;
                /* build a group with all units, -1,-1 as destination means it will
                   simply attack/defend the nearest target. later on this should
                   split up into several groups with different target and strategy */
                ai_group = ai_group_create( cur_player->strat, -1, -1 );
                list_reset( ai_units );
                while ( ( unit = list_next( ai_units ) ) )
                    ai_group_add_unit( ai_group, unit );
                AI_DEBUG( 0, "*** MOVE & ATTACK ***\n" );
            }
            else {
                /* check if unit needs supply and remove 
                   it from ai_units if so */
                if ( ( unit_low_fuel( unit ) || unit_low_ammo( unit ) ) ) {
                    if ( unit->supply_level > 0 ) {
                        action_queue_supply( unit );
                        list_delete_item( ai_units, unit );
                        AI_DEBUG( 1, "%s supplies\n", unit->name );
                        break;
                    }
                    else {
                        AI_DEBUG( 1, "%s searches depot\n", unit->name );
                        if ( ai_get_dist( unit, unit->x, unit->y, AI_FIND_DEPOT,
                                          &dx, &dy, &dist ) )
                            if ( ai_approximate( unit, dx, dy, &x, &y ) ) {
                                action_queue_move( unit, x, y );
                                list_delete_item( ai_units, unit );
                                AI_DEBUG( 1, "%s moves to %i,%i\n", unit->name, x, y );
                                break;
                            }
                    }
                }
            }
            break;
        case AI_STATUS_MERGE:
            if ( ( unit = list_next( ai_units ) ) ) {
                map_get_merge_units( unit, partners, &partner_count );
                best = 0; /* merge with the one that has the most strength points */
                for ( i = 0; i < partner_count; i++ )
                    if ( best == 0 )
                        best = partners[i];
                    else
                        if ( best->str < partners[i]->str )
                            best = partners[i];
                if ( best ) {
                    AI_DEBUG( 1, "%s merges with %s\n", unit->name, best->name );
                    action_queue_merge( unit, best );
                    /* both units are handled now */
                    list_delete_item( ai_units, unit );
                    list_delete_item( ai_units, best ); 
                }
            }
            else {
                ai_status = AI_STATUS_SUPPLY;
                list_reset( ai_units );
                AI_DEBUG( 0, "*** SUPPLY ***\n" );
            }
            break;
        case AI_STATUS_GROUP:
            if ( !ai_group_handle_next_unit( ai_group ) ) {
                ai_group_delete( ai_group );
                if (config.purchase) {
                    ai_status = AI_STATUS_PURCHASE;
                    AI_DEBUG( 0, "*** PURCHASE ***\n" );
                } else {
                    ai_status = AI_STATUS_END;
                    AI_DEBUG( 0, "*** END TURN ***\n" );
                }
            }
            break;
        case AI_STATUS_PURCHASE:
            ai_purchase_units();
            ai_status = AI_STATUS_END;
            AI_DEBUG( 0, "*** END TURN ***\n" );
            break;
        case AI_STATUS_END:
            action_queue_end_turn();
            ai_status = AI_STATUS_FINALIZE;
            break;
    }
}

/*
====================================================================
Undo the steps (e.g. memory allocation) made in ai_init()
====================================================================
*/
void ai_finalize( void )
{
    AI_DEBUG(2, "%s entered\n",__FUNCTION__);
    if ( finalized )
        return;
    AI_DEBUG(2, "Really finalized\n");
    list_delete( ai_units );
    AI_DEBUG( 0, "AI Finalized\n" );
    ai_status = AI_STATUS_INIT;
    finalized = 1;
}

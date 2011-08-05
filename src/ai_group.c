/***************************************************************************
                          ai_group.c  -  description
                             -------------------
    begin                : Fri Jan 19 2001
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
#include "unit.h"
#include "action.h"
#include "map.h"
#include "ai_tools.h"
#include "ai_group.h"

/*
====================================================================
Externals
====================================================================
*/
extern Player *cur_player;
extern List *units, *avail_units;
extern int map_w, map_h;
extern Map_Tile **map;
extern Mask_Tile **mask;
extern int trgt_type_count;
extern int cur_weather;
extern Config config;

/*
====================================================================
LOCALS
====================================================================
*/

/* OH NO, A HACK! */
extern int ai_get_dist( Unit *unit, int x, int y, int type, int *dx, int *dy, int *dist );

/*
====================================================================
Check if there is an unspotted tile or an enemy within range 6
that may move close enough to attack. Flying units are not counted
as these may move very far anyway.
====================================================================
*/
typedef struct {
    Player *player;
    int unit_x, unit_y;
    int unsafe;
} MountCtx;
static int hex_is_safe( int x, int y, void *_ctx )
{
    MountCtx *ctx = _ctx;
    if ( !mask[x][y].spot ) {
        ctx->unsafe = 1;
        return 0;
    }
    if ( map[x][y].g_unit )
    if ( !player_is_ally( ctx->player, map[x][y].g_unit->player ) )
    if ( map[x][y].g_unit->sel_prop->mov >= get_dist( ctx->unit_x, ctx->unit_y, x, y ) - 1 ) {
        ctx->unsafe = 1;
        return 0;
    }
    return 1;
}
static int ai_unsafe_mount( Unit *unit, int x, int y ) 
{
    MountCtx ctx = { unit->player, x, y, 0 };
    ai_eval_hexes( x, y, 6, hex_is_safe, &ctx );
    return ctx.unsafe;
}

/*
====================================================================
Count the number of defensive supporters.
====================================================================
*/
typedef struct {
    Unit *unit;
    int count;
} DefCtx;
static int hex_df_unit( int x, int y, void *_ctx )
{
    DefCtx *ctx = _ctx;
    if ( map[x][y].g_unit ) {
        if ( ctx->unit->sel_prop->flags & FLYING ) {
            if ( map[x][y].g_unit->sel_prop->flags & AIR_DEFENSE )
                ctx->count++;
        }
        else {
            if ( map[x][y].g_unit->sel_prop->flags & ARTILLERY )
                ctx->count++;
        }
    }
    return 1;
}
static void ai_count_df_units( Unit *unit, int x, int y, int *result )
{
    DefCtx ctx = { unit, 0 };
    *result = 0;
    if ( unit->sel_prop->flags & ARTILLERY )
        return;
    ai_eval_hexes( x, y, 3, hex_df_unit, &ctx );
    /* only three defenders are taken in account */
    if ( *result > 3 )
        *result = 3;
}

/*
====================================================================
Gather all valid targets of a unit.
====================================================================
*/
typedef struct {
    Unit *unit;
    List *targets;
} GatherCtx;
static int hex_add_targets( int x, int y, void *_ctx )
{
    GatherCtx *ctx = _ctx;
    if ( mask[x][y].spot ) {
        if ( map[x][y].a_unit && unit_check_attack( ctx->unit, map[x][y].a_unit, UNIT_ACTIVE_ATTACK ) )
            list_add( ctx->targets, map[x][y].a_unit );
        if ( map[x][y].g_unit && unit_check_attack( ctx->unit, map[x][y].g_unit, UNIT_ACTIVE_ATTACK ) )
            list_add( ctx->targets, map[x][y].g_unit );
    }
    return 1;
}
static List* ai_gather_targets( Unit *unit, int x, int y )
{
    GatherCtx ctx;
    ctx.unit = unit;
    ctx.targets= list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    ai_eval_hexes( x, y, unit->sel_prop->rng + 1, hex_add_targets, &ctx );
    return ctx.targets;
}


/*
====================================================================
Evaluate a unit's attack against target.
  score_base: basic score for attacking
  score_rugged: score added for each rugged def point (0-100)
                of target
  score_kill: score unit receives for each (expected) point of
              strength damage done to target
  score_loss: score that is substracted per strength point
              unit is expected to loose
The final score is stored to 'result' and True if returned if the
attack may be performed else False.
====================================================================
*/
static int unit_evaluate_attack( Unit *unit, Unit *target, int score_base, int score_rugged, int score_kill, int score_loss, int *result )
{
    int unit_dam = 0, target_dam = 0, rugged_def = 0;
    if ( !unit_check_attack( unit, target, UNIT_ACTIVE_ATTACK ) ) return 0;
    unit_get_expected_losses( unit, target, &unit_dam, &target_dam );
    if ( unit_check_rugged_def( unit, target ) )
        rugged_def = unit_get_rugged_def_chance( unit, target );
    if ( rugged_def < 0 ) rugged_def = 0;
    *result = score_base + rugged_def * score_rugged + target_dam * score_kill + unit_dam * score_loss;
    AI_DEBUG( 2, "  %s: %s: bas:%i, rug:%i, kil:%i, los: %i = %i\n", unit->name, target->name,
            score_base,
            rugged_def * score_rugged,
            target_dam * score_kill,
            unit_dam * score_loss, *result );
    /* if target is a df unit give a small bonus */
    if ( target->sel_prop->flags & ARTILLERY || target->sel_prop->flags & AIR_DEFENSE )
        *result += score_kill;
    return 1;
}

/*
====================================================================
Get the best target for unit if any.
====================================================================
*/
static int ai_get_best_target( Unit *unit, int x, int y, AI_Group *group, Unit **target, int *score )
{
    int old_x = unit->x, old_y = unit->y;
    int pos_targets;
    Unit *entry;
    List *targets;
    int score_atk_base, score_rugged, score_kill, score_loss;
    
    /* scores */
    score_atk_base = 20 + group->order * 10;
    score_rugged   = -1;
    score_kill     = ( group->order + 3 ) * 10;
    score_loss     = ( 2 - group->order ) * -10;
    
    unit->x = x; unit->y = y;
    *target = 0; *score = -999999;
    /* if the transporter is needed attacking is suicide */
    if ( mask[x][y].mount && unit->trsp_prop.id )
        return 0;
    /* gather targets */
    targets = ai_gather_targets( unit, x, y );
    /* evaluate all attacks */
    if ( targets ) {
        list_reset( targets ); 
        while ( ( entry = list_next( targets ) ) )
            if ( !unit_evaluate_attack( unit, entry, score_atk_base, score_rugged, score_kill, score_loss, &entry->target_score ) )
                list_delete_item( targets, entry ); /* erroneous entry: can't be attacked */
        /* check whether any positive targets exist */
        pos_targets = 0;
        list_reset( targets ); 
        while ( ( entry = list_next( targets ) ) )
            if ( entry->target_score > 0 ) {
                pos_targets = 1;
                break;
            }
        /* get best target */
        list_reset( targets ); 
        while ( ( entry = list_next( targets ) ) ) {
            /* if unit is on an objective or center of interest give a bonus
            as this tile must be captured by all means. but only do so if there
            is no other target with a positive value */
            if ( !pos_targets )
            if ( ( entry->x == group->x && entry->y == group->y ) || map[entry->x][entry->y].obj ) {
                entry->target_score += 100;
                AI_DEBUG( 2, "    + 100 for %s: capture by all means\n", entry->name );
            }
                
            if ( entry->target_score > *score ) {
                *target = entry;
                *score = entry->target_score;
            }
        }
        list_delete( targets );
    }
    unit->x = old_x; unit->y = old_y;
    return (*target) != 0;
}

/*
====================================================================
Evaluate position for a unit by checking the group context. 
Return True if this evaluation is valid. The results are stored
to 'eval'.
====================================================================
*/
typedef struct {
    Unit *unit; /* unit that's checked */
    AI_Group *group;
    int x, y; /* position that was evaluated */
    int mov_score; /* result for moving */
    Unit *target; /* if set atk_result is relevant */
    int atk_score; /* result including attack evaluation */
} AI_Eval;
static AI_Eval *ai_create_eval( Unit* unit, AI_Group *group, int x, int y )
{
    AI_Eval *eval = calloc( 1, sizeof( AI_Eval ) );
    eval->unit = unit; eval->group = group;
    eval->x = x; eval->y = y;
    return eval;
}
int ai_evaluate_hex( AI_Eval *eval )
{
    int result;
    int i, nx, ny, ox, oy,odist, covered, j, nx2, ny2;
    eval->target = 0;
    eval->mov_score = eval->atk_score = 0;
    /* terrain modifications which only apply for ground units */
    if ( !(eval->unit->sel_prop->flags & FLYING ) ) {
        /* entrenchment bonus. infantry receives more than others. */
        eval->mov_score += ((eval->unit->sel_prop->flags&INFANTRY)?2:1) *
                           ( map[eval->x][eval->y].terrain->min_entr * 2 + 
                             map[eval->x][eval->y].terrain->max_entr );
        /* if the unit looses initiative on this terrain we give a malus */
        if ( map[eval->x][eval->y].terrain->max_ini < eval->unit->sel_prop->ini )
            eval->mov_score -= 5 * ( eval->unit->sel_prop->ini - 
                                      map[eval->x][eval->y].terrain->max_ini );
        /* rivers should be avoided */
        if ( map[eval->x][eval->y].terrain->flags[cur_weather] & RIVER )
            eval->mov_score -= 50;
        if ( map[eval->x][eval->y].terrain->flags[cur_weather] & SWAMP )
            eval->mov_score -= 30;
        /* inf_close_def will benefit an infantry while disadvantaging
           other units */
        if ( map[eval->x][eval->y].terrain->flags[cur_weather] & INF_CLOSE_DEF ) {
            if ( eval->unit->sel_prop->flags & INFANTRY )
                eval->mov_score += 30;
            else
                eval->mov_score -= 20;
        }
        /* if this is a mount position and an enemy or fog is less than
           6 tiles away we give a big malus */
        if ( mask[eval->x][eval->y].mount )
            if ( ai_unsafe_mount( eval->unit, eval->x, eval->y ) )
                eval->mov_score -= 1000;
        /* conquering a flag gives a bonus */
        if ( map[eval->x][eval->y].player )
            if ( !player_is_ally( eval->unit->player, map[eval->x][eval->y].player ) )
                if ( map[eval->x][eval->y].g_unit == 0 ) {
                    eval->mov_score += 600;
                    if ( map[eval->x][eval->y].obj )
                        eval->mov_score += 600;
                }
        /* if this position allows debarking or is just one hex away
           this tile receives a big bonus. */
        if ( eval->unit->embark == EMBARK_SEA ) {
            if ( map_check_unit_debark( eval->unit, eval->x, eval->y, EMBARK_SEA, 0 ) )
                eval->mov_score += 1000;
            else
                for ( i = 0; i < 6; i++ )
                    if ( get_close_hex_pos( eval->x, eval->y, i, &nx, &ny ) )
                        if ( map_check_unit_debark( eval->unit, nx, ny, EMBARK_SEA, 0 ) ) {
                            eval->mov_score += 500;
                            break;
                        }
        }
    }
    /* modifications on flying units */
    if ( eval->unit->sel_prop->flags & FLYING ) {
        /* if interceptor covers an uncovered bomber on this tile give bonus */
        if ( eval->unit->sel_prop->flags & INTERCEPTOR ) {
            for ( i = 0; i < 6; i++ )
                if ( get_close_hex_pos( eval->x, eval->y, i, &nx, &ny ) )
                if ( map[nx][ny].a_unit )
                if ( player_is_ally( cur_player, map[nx][ny].a_unit->player ) )
                if ( map[nx][ny].a_unit->sel_prop->flags & BOMBER ) {
                    covered = 0;
                    for ( j = 0; j < 6; j++ )
                        if ( get_close_hex_pos( nx, ny, j, &nx2, &ny2 ) )
                        if ( map[nx2][ny2].a_unit )
                        if ( player_is_ally( cur_player, map[nx2][ny2].a_unit->player ) )
                        if ( map[nx2][ny2].a_unit->sel_prop->flags & INTERCEPTOR ) 
                        if ( map[nx2][ny2].a_unit != eval->unit ) {
                            covered = 1;
                            break;
                        }
                    if ( !covered )
                        eval->mov_score += 2000; /* 100 equals one tile of getting
                                                    to center of interest which must
                                                    be overcome */
                }
        }
    }
    /* each group has a 'center of interest'. getting closer
       to this center is honored. */
    if ( eval->group->x == -1 ) {
        /* proceed to the nearest flag */
        if ( !(eval->unit->sel_prop->flags & FLYING ) ) {
            if ( eval->group->order > 0 ) {
                if ( ai_get_dist( eval->unit, eval->x, eval->y, AI_FIND_ENEMY_OBJ, &ox, &oy, &odist ) )
                    eval->mov_score -= odist * 100;
            }
            else
                if ( eval->group->order < 0 ) {
                    if ( ai_get_dist( eval->unit, eval->x, eval->y, AI_FIND_OWN_OBJ, &ox, &oy, &odist ) )
                        eval->mov_score -= odist * 100;
                }
        }
    }
    else
        eval->mov_score -= 100 * get_dist( eval->x, eval->y, eval->group->x, eval->group->y );
    /* defensive support */
    ai_count_df_units( eval->unit, eval->x, eval->y, &result );
    if ( result > 2 ) result = 2; /* senseful limit */
    eval->mov_score += result * 10;
    /* check for the best target and save the result to atk_score */
    eval->atk_score = eval->mov_score;
    if ( !mask[eval->x][eval->y].mount )
    if ( !( eval->unit->sel_prop->flags & ATTACK_FIRST ) || 
          ( eval->unit->x == eval->x && eval->unit->y == eval->y ) )
    if ( ai_get_best_target( eval->unit, eval->x, eval->y, eval->group, &eval->target, &result ) )
        eval->atk_score += result;
    return 1;
}

/*
====================================================================
Choose and store the best tactical action of a unit (found by use of
ai_evaluate_hex). If there is none AI_SUPPLY is stored.
====================================================================
*/
void ai_handle_unit( Unit *unit, AI_Group *group )
{
    int x, y, nx, ny, i, action = 0;
    List *list = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    AI_Eval *eval;
    Unit *target = 0;
    int score = -999999;
    /* get move mask */
    map_get_unit_move_mask( unit );
    x = unit->x; y = unit->y; target = 0;
    /* evaluate all positions */
    list_add( list, ai_create_eval( unit, group, unit->x, unit->y ) );
    while ( list->count > 0 ) {
        eval = list_dequeue( list );
        if ( ai_evaluate_hex( eval ) ) {
            /* movement evaluation */
            if ( eval->mov_score > score ) {
                score = eval->mov_score;
                target = 0;
                x = eval->x; y = eval->y;
            }
            /* movement + attack evaluation. ignore for attack_first
             * units which already fired */
            if ( !(unit->sel_prop->flags & ATTACK_FIRST) )
            if ( eval->target && eval->atk_score > score ) {
                score = eval->atk_score;
                target = eval->target;
                x = eval->x; y = eval->y;
            }
        }
        /* store next hex tiles */
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( eval->x, eval->y, i, &nx, &ny ) )
                if ( ( mask[nx][ny].in_range && !mask[nx][ny].blocked ) || mask[nx][ny].sea_embark ) {
                    mask[nx][ny].in_range = 0; 
                    mask[nx][ny].sea_embark = 0;
                    list_add( list, ai_create_eval( unit, group, nx, ny ) );
                }
        free( eval );
    }
    list_delete( list );
    /* check result and store appropiate action */
    if ( unit->x != x || unit->y != y ) {
        if ( map_check_unit_debark( unit, x, y, EMBARK_SEA, 0 ) ) {
            action_queue_debark_sea( unit, x, y ); action = 1;
            AI_DEBUG( 1, "%s debarks at %i,%i\n", unit->name, x, y );
        }
        else {
            action_queue_move( unit, x, y ); action = 1;
            AI_DEBUG( 1, "%s moves to %i,%i\n", unit->name, x, y );
        }
    }
    if ( target ) {
        action_queue_attack( unit, target ); action = 1;
        AI_DEBUG( 1, "%s attacks %s\n", unit->name, target->name );
    }
    if ( !action ) {
        action_queue_supply( unit );
        AI_DEBUG( 1, "%s supplies: %i,%i\n", unit->name, unit->cur_ammo, unit->cur_fuel );
    }
}

/*
====================================================================
Get the best target and attack by range. Do not try to move the 
unit yet. If there is no target at all do nothing.
====================================================================
*/
void ai_fire_artillery( Unit *unit, AI_Group *group )
{
    AI_Eval *eval = ai_create_eval( unit, group, unit->x, unit->y );
    if ( ai_evaluate_hex( eval ) && eval->target )
    {
        action_queue_attack( unit, eval->target );
        AI_DEBUG( 1, "%s attacks first %s\n", unit->name, eval->target->name );
    }
    free( eval );
}

/*
====================================================================
PUBLICS
====================================================================
*/

/*
====================================================================
Create/Delete a group
====================================================================
*/
AI_Group *ai_group_create( int order, int x, int y )
{
    AI_Group *group = calloc( 1, sizeof( AI_Group ) );
    group->state = GS_ART_FIRE; 
    group->order = order;
    group->x = x; group->y = y;
    group->units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    list_reset( group->units );
    return group;
}
void ai_group_add_unit( AI_Group *group, Unit *unit )
{
    /* sort unit into units list in the order in which units are
     * supposed to move. artillery fire is first but moves last
     * thus artillery comes last in this list. that it fires first
     * is handled by ai_group_handle_next_unit. list order is: 
     * bombers, ground units, fighters, artillery */
    if ( unit->prop.flags & ARTILLERY || 
         unit->prop.flags & AIR_DEFENSE )
        list_add( group->units, unit );
    else
    if ( unit->prop.class == 9 || unit->prop.class == 10 )
    {
        /* tactical and high level bomber */
        list_insert( group->units, unit, 0 );
        group->bomber_count++;
    }
    else
    if ( unit->prop.flags & FLYING )
    {
        /* airborne ground units are not in this section ... */
        list_insert( group->units, unit, 
                     group->bomber_count + group->ground_count );
        group->aircraft_count++;
    }
    else
    {
        /* everything else: ships and ground units */
        list_insert( group->units, unit, group->bomber_count );
        group->ground_count++;
    }
    /* HACK: set hold_pos flag for those units that should not move due
       to high entrenchment or being close to artillery and such; but these
       units will attack, too and may move if's really worth it */
    if (!(unit->sel_prop->flags&FLYING))
    if (!(unit->sel_prop->flags&SWIMMING))
    {
        int i,nx,ny,no_move = 0;
        if (map[unit->x][unit->y].obj) no_move = 1;
        if (group->order<0)
        {
            if (map[unit->x][unit->y].nation) no_move = 1;
            if (unit->entr>=6) no_move = 1;
            if (unit->sel_prop->flags&ARTILLERY) no_move = 1;
            if (unit->sel_prop->flags&AIR_DEFENSE) no_move = 1;
            for (i=0;i<6;i++) 
                if (get_close_hex_pos(unit->x,unit->y,i,&nx,&ny))
                    if (map[nx][ny].g_unit)
                    {
                        if (map[nx][ny].g_unit->sel_prop->flags&ARTILLERY)
                        {
                            no_move = 1;
                            break;
                        }
                        if (map[nx][ny].g_unit->sel_prop->flags&AIR_DEFENSE)
                        {
                            no_move = 1;
                            break;
                        }
                    }
        }
        if (no_move) unit->cur_mov = 0;
    }
}
void ai_group_delete_unit( AI_Group *group, Unit *unit )
{
    /* remove unit */
    int contained_unit = list_delete_item( group->units, unit );
    if ( !contained_unit ) return;
    
    /* update respective counter */
    if ( unit->prop.flags & ARTILLERY || 
         unit->prop.flags & AIR_DEFENSE )
        /* nothing to be done */;
    else
    if ( unit->prop.class == 9 || unit->prop.class == 10 )
    {
        /* tactical and high level bomber */
        group->bomber_count--;
    }
    else
    if ( unit->prop.flags & FLYING )
    {
        /* airborne ground units are not in this section ... */
        group->aircraft_count--;
    }
    else
    {
        /* everything else: ships and ground units */
        group->ground_count--;
    }
}
void ai_group_delete( void *ptr )
{
    AI_Group *group = ptr;
    if ( group ) {
        if ( group->units )
            list_delete( group->units );
        free( group );
    }
}
/*
====================================================================
Handle next unit of a group to follow order. Stores all nescessary 
unit actions. If group is completely handled, it returns False.
====================================================================
*/
int ai_group_handle_next_unit( AI_Group *group )
{
    Unit *unit = list_next( group->units );
    if ( unit == 0 ) 
    {
        if ( group->state == GS_MOVE )
            return 0;
        else
        {
            group->state = GS_MOVE;
            list_reset( group->units );
            unit = list_next( group->units );
            if ( unit == 0 ) return 0;
        }
    }
    if ( unit == 0 ) {
        AI_DEBUG(0,"ERROR: %s: null unit detected\n",__FUNCTION__);
	return 0;
    }
    /* Unit is dead? Can only be attacker that was killed by defender */
    if ( unit->killed ) {
      AI_DEBUG(0, "Removing killed attacker %s(%d,%d) from group\n", unit->name, unit->x, unit->y);
      ai_group_delete_unit( group, unit );
      return ai_group_handle_next_unit( group );
    }
    if ( group->state == GS_ART_FIRE )
    {
        if ( unit->sel_prop->flags & ATTACK_FIRST )
            ai_fire_artillery( unit, group ); /* does not check optimal 
                                                 movement but simply 
                                                 fires */
    }
    else
        ai_handle_unit( unit, group ); /* checks to the full tactical
                                          extend */
    return 1;
}

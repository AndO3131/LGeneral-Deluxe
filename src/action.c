/***************************************************************************
                          action.c  -  description
                             -------------------
    begin                : Mon Apr 1 2002
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

static List *actions = 0;

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Create basic action.
====================================================================
*/
static Action *action_create( int type ) 
{
    Action *action = calloc( 1, sizeof( Action ) );
    action->type = type;
    return action;
}
/*
====================================================================
Queue an action.
====================================================================
*/
static void action_queue( Action *action )
{
    list_add( actions, action );
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Create/delete engine action queue
====================================================================
*/
void actions_create()
{
    actions_delete();
    actions = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
}
void actions_delete()
{
    if ( actions ) {
        actions_clear();
        list_delete( actions );
        actions = 0;
    }
}

/*
====================================================================
Get next action or clear all actions. The returned action struct
must be cleared by engine after usage.
====================================================================
*/
Action* actions_dequeue()
{
    return list_dequeue( actions );
}
void actions_clear()
{
    Action *action = 0;
    while ( ( action = list_dequeue( actions ) ) )
        free( action );
}

/*
====================================================================
Returns topmost action in queue or 0 if none available.
====================================================================
*/
Action *actions_top(void)
{
    return list_last( actions );
}

/*
====================================================================
Remove the last action in queue (cancelled confirmation)
====================================================================
*/
void action_remove_last()
{
    Action *action = 0;
    if ( actions->count == 0 ) return;
    action = list_last( actions );
    list_delete_item( actions, action );
    free( action );
}

/*
====================================================================
Get number of queued actions
====================================================================
*/
int actions_count()
{
    return actions->count;
}

/*
====================================================================
Create an engine action and automatically queue it. The engine
will perform security checks before handling an action to prevent
illegal actions.
====================================================================
*/
void action_queue_none()
{
    Action *action = action_create( ACTION_NONE );
    action_queue( action );
}
void action_queue_end_turn()
{
    Action *action = action_create( ACTION_END_TURN );
    action_queue( action );
}
void action_queue_move( Unit *unit, int x, int y )
{
    Action *action = action_create( ACTION_MOVE );
    action->unit = unit;
    action->x = x; action->y = y;
    action_queue( action );
}
void action_queue_attack( Unit *unit, Unit *target )
{
    Action *action = action_create( ACTION_ATTACK );
    action->unit = unit;
    action->target = target;
    action_queue( action );
}
void action_queue_strategic_attack( Unit *unit)
{
    Action *action = action_create( ACTION_STRATEGIC_ATTACK );
    action->unit = unit;
    action_queue( action );
}
void action_queue_supply( Unit *unit )
{
    Action *action = action_create( ACTION_SUPPLY );
    action->unit = unit;
    action_queue( action );
}
void action_queue_embark_sea( Unit *unit, int x, int y )
{
    Action *action = action_create( ACTION_EMBARK_SEA );
    action->unit = unit;
    action->x = x; action->y = y;
    action_queue( action );
}
void action_queue_debark_sea( Unit *unit, int x, int y )
{
    Action *action = action_create( ACTION_DEBARK_SEA );
    action->unit = unit;
    action->x = x; action->y = y;
    action_queue( action );
}
void action_queue_embark_air( Unit *unit, int x, int y )
{
    Action *action = action_create( ACTION_EMBARK_AIR );
    action->unit = unit;
    action->x = x; action->y = y;
    action_queue( action );
}
void action_queue_debark_air( Unit *unit, int x, int y, int normal_landing )
{
    Action *action = action_create( ACTION_DEBARK_AIR );
    action->unit = unit;
    action->x = x; action->y = y;
    action->id = normal_landing;
    action_queue( action );
}
void action_queue_merge( Unit *unit, Unit *partner )
{
    Action *action = action_create( ACTION_MERGE );
    action->unit = unit;
    action->target = partner;
    action_queue( action );
}
void action_queue_split( Unit *unit, int str, int x, int y, Unit *partner )
{
    Action *action = action_create( ACTION_SPLIT );
    action->unit = unit;
    action->target = partner;
    action->x = x; action->y = y;
    action->str = str;
    action_queue( action );
}
void action_queue_disband( Unit *unit )
{
    Action *action = action_create( ACTION_DISBAND );
    action->unit = unit;
    action_queue( action );
}
void action_queue_deploy( Unit *unit, int x, int y )
{
    Action *action = action_create( ACTION_DEPLOY );
    action->unit = unit;
    action->x = x; 
    action->y = y;
    action_queue( action );
}
void action_queue_draw_map()
{
    Action *action = action_create( ACTION_DRAW_MAP );
    action_queue( action );
}
void action_queue_set_spot_mask()
{
    Action *action = action_create( ACTION_SET_SPOT_MASK );
    action_queue( action );
}
void action_queue_set_vmode( int w, int h, int fullscreen )
{
    Action *action = action_create( ACTION_SET_VMODE );
    action->w = w; action->h = h;
    action->full = fullscreen;
    action_queue( action );
}
void action_queue_quit()
{
    Action *action = action_create( ACTION_QUIT );
    action_queue( action );
}
void action_queue_restart()
{
    Action *action = action_create( ACTION_RESTART );
    action_queue( action );
}
void action_queue_load( int id )
{
    Action *action = action_create( ACTION_LOAD );
    action->id = id;
    action_queue( action );
}
void action_queue_overwrite( int id )
{
    Action *action = action_create( ACTION_OVERWRITE );
    action->id = id;
    action_queue( action );
}
void action_queue_start_scen()
{
    Action *action = action_create( ACTION_START_SCEN );
    action_queue( action );
}
void action_queue_start_camp()
{
    Action *action = action_create( ACTION_START_CAMP );
    action_queue( action );
}

/***************************************************************************
                          action.h  -  description
                             -------------------
    begin                : Mon Apr 1 2002
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

#ifndef __ACTION_H
#define __ACTION_H

/*
====================================================================
Engine actions
====================================================================
*/
enum {
    ACTION_NONE = 0,
    ACTION_END_TURN,
    ACTION_MOVE,
    ACTION_ATTACK,
    ACTION_SUPPLY,
    ACTION_EMBARK_SEA,
    ACTION_DEBARK_SEA,
    ACTION_EMBARK_AIR,
    ACTION_DEBARK_AIR,
    ACTION_MERGE,
    ACTION_SPLIT,
    ACTION_REPLACE,
    ACTION_ELITE_REPLACE,
    ACTION_DISBAND,
    ACTION_DEPLOY,
    ACTION_DRAW_MAP,
    ACTION_SET_SPOT_MASK,
    ACTION_SET_VMODE,
    ACTION_QUIT,
    ACTION_RESTART,
    ACTION_LOAD,
    ACTION_OVERWRITE,
    ACTION_START_SCEN,
    ACTION_START_CAMP,
    ACTION_CHANGE_MOD
};
typedef struct {
    int type;       /* type as above */
    Unit *unit;     /* unit performing the action */
    Unit *target;   /* target if attack */
    int x, y;       /* dest position if movement */
    int w, h, full; /* video mode settings */
    int id;         /* id used by debark air unit */
    char *name;     /* slot name if any */
    int str;        /* strength of split */
} Action;

/*
====================================================================
Create/delete engine action queue
====================================================================
*/
void actions_create();
void actions_delete();

/*
====================================================================
Get next action or clear all actions. The returned action struct
must be cleared by engine after usage.
====================================================================
*/
Action* actions_dequeue();
void actions_clear();

/*
====================================================================
Remove the last action in queue (cancelled confirmation)
====================================================================
*/
void action_remove_last();

/*
====================================================================
Returns topmost action in queue or 0 if none available.
====================================================================
*/
Action *actions_top(void);

/*
====================================================================
Get number of queued actions
====================================================================
*/
int actions_count();

/*
====================================================================
Create an engine action and automatically queue it. The engine
will perform security checks before handling an action to prevent
illegal actions.
====================================================================
*/
void action_queue_none();
void action_queue_end_turn();
void action_queue_move( Unit *unit, int x, int y );
void action_queue_attack( Unit *unit, Unit *target );
void action_queue_supply( Unit *unit );
void action_queue_embark_sea( Unit *unit, int x, int y );
void action_queue_debark_sea( Unit *unit, int x, int y );
void action_queue_embark_air( Unit *unit, int x, int y );
void action_queue_debark_air( Unit *unit, int x, int y, int normal_landing );
void action_queue_merge( Unit *unit, Unit *partner );
void action_queue_split( Unit *unit, int str, int x, int y, Unit *partner );
void action_queue_replace( Unit *unit );
void action_queue_elite_replace( Unit *unit );
void action_queue_disband( Unit *unit );
void action_queue_deploy( Unit *unit, int x, int y );
void action_queue_draw_map();
void action_queue_set_spot_mask();
void action_queue_set_vmode( int w, int h, int fullscreen );
void action_queue_quit();
void action_queue_restart();
void action_queue_load( char *name );
void action_queue_overwrite( char *name );
void action_queue_start_scen();
void action_queue_start_camp();
void action_queue_change_mod();

#endif

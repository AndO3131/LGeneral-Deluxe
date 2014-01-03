/***************************************************************************
                          player.h  -  description
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

#ifndef __PLAYER_H
#define __PLAYER_H

#include "list.h"
#include "unit_lib.h"

/*
====================================================================
Player info. The players are created by scenario but managed
by this module.
====================================================================
*/
enum {
    PLAYER_CTRL_NOBODY = 0,
    PLAYER_CTRL_HUMAN,
    PLAYER_CTRL_CPU
};
typedef struct {
    char *id;           /* identification */
    char *name;         /* real name */
    int ctrl;           /* controlled by human or CPU */
    char *ai_fname;     /* dll with AI routines */
    int strat;          /* strategy: -2 very defensive to 2 very aggressive */
    int strength_row;   /* number of strength row in strength icons */
    Nation **nations;   /* list of pointers to nations controlled by this player */
    int nation_count;   /* number of nations controlled */
    List *allies;       /* list of the player's allies */
    int core_limit;     /* max number of core units (placed + ordered) */
    int unit_limit;     /* max number of units (placed + ordered) */
    int air_trsp_count; /* number of air transporters */
    int sea_trsp_count; /* number of sea transporters */
    Unit_Lib_Entry *air_trsp; /* default air transporter */
    Unit_Lib_Entry *sea_trsp; /* default sea transporter */
    int air_trsp_used;  /* number of air transporters in use */
    int sea_trsp_used;  /* dito */
    int orient;         /* initial orientation */
    int cur_prestige;       /* current amount of prestige */
    int *prestige_per_turn; /* amount added in the beginning of each turn */
    int no_init_deploy; /* whether current scenario provided any initial 
                           hex tiles where deployment is okay */
    /* ai callbacks loaded from module ai_fname */
    void (*ai_init)(void);
    void (*ai_run)(void);
    void (*ai_finalize)(void);
#ifdef USE_DL    
    void *ai_mod_handle; /* handle to the ai module */
#endif
} Player;

/*
====================================================================
Add player to player list.
====================================================================
*/
void player_add( Player *player );

/*
====================================================================
Delete player entry. Pass as void* for compatibility with
list_delete
====================================================================
*/
void player_delete( void *ptr );

/*
====================================================================
Delete all players.
====================================================================
*/
void players_delete();

/*
====================================================================
Get very first player.
====================================================================
*/
Player* players_get_first();

/*
====================================================================
Get next player in the cycle and also set new_turn true if all
players have finished their turn action and it's time for 
a new turn.
====================================================================
*/
Player *players_get_next( int *new_turn );

/*
====================================================================
Check who would be the next player but do not choose him.
====================================================================
*/
Player *players_test_next();

/*
====================================================================
Set and get player as current by index.
====================================================================
*/
Player *players_set_current( int index );

/*
====================================================================
Check if these two players are allies.
====================================================================
*/
int player_is_ally( Player *player, Player *second );

/*
====================================================================
Get the player who controls nation
====================================================================
*/
Player *player_get_by_nation( Nation *nation );

/*
====================================================================
Get player with this id string.
====================================================================
*/
Player *player_get_by_id( char *id );

/*
====================================================================
Get player with this index
====================================================================
*/
Player *player_get_by_index( int index );

/*
====================================================================
Get player index in list
====================================================================
*/
int player_get_index( Player *player );

#endif

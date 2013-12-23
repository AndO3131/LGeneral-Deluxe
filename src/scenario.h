/***************************************************************************
                          scenario.h  -  description
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

#ifndef __SCENARIO_H
#define __SCENARIO_H

struct _Unit;

#define DEBUG_CAMPAIGN 1

/*
====================================================================
Engine setup. 'name' is the name of the scenario or savegame 
file. 'type' specifies what to do with the remaining data in this
struct:
  INIT_CAMP: load whole campaign and use default values of the
             scenarios
  INIT_SCEN: use setup info to overwrite scenario's defaults
  DEFAULT:   use default values and set 'setup'
  LOAD:      load game and set 'setup'
  CAMP_BRIEFING: show campaign briefing dialog
  RUN_TITLE: show title screen and run the menu
'ctrl' is the player control (PLAYER_CTRL_HUMAN, PLAYER_CTRL_CPU)  
====================================================================
*/
enum {
    SETUP_UNKNOWN = 0,
    SETUP_INIT_CAMP,
    SETUP_INIT_SCEN,
    SETUP_LOAD_GAME,
    SETUP_DEFAULT_SCEN,
    SETUP_CAMP_BRIEFING,
    SETUP_RUN_TITLE
};
typedef struct {
    char fname[256];    /* resource file for loading type */
    int  type;
    char *load_name; /* in case of LOAD_GAME this is the slot id */
    /* campaign specific information, must be set for SETUP_CAMP_INIT */
    const char *scen_state; /* scenario state to begin campaign with */
    /* scenario specific information which is loaded by scen_load_info() */
    int  player_count;
    int  *ctrl;
    char **modules;
    char **names;
} Setup;

/*
====================================================================
Scenario Info
====================================================================
*/
typedef struct {
    char *fname;    /* scenario knows it's own file_name in the scenario path */
    char *name;     /* scenario's name */
    char *desc;     /* description */
    char *authors;  /* a string with all author names */
    Date start_date;/* starting date of scenario */ 
    int turn_limit;     /* scenario is finished after this number of turns at the latest */
    int days_per_turn;
    int turns_per_day;  /* the date string of a turn is computed from these to values 
                           and the inital date */
    int player_count;   /* number of players */
} Scen_Info;

/*
====================================================================
Victory conditions
====================================================================
*/
enum { 
    VCOND_CHECK_EVERY_TURN = 0,
    VCOND_CHECK_LAST_TURN
};
enum {
    VSUBCOND_CTRL_ALL_HEXES = 0,
    VSUBCOND_CTRL_HEX,
    VSUBCOND_TURNS_LEFT,
    VSUBCOND_CTRL_HEX_NUM,
    VSUBCOND_UNITS_KILLED,
    VSUBCOND_UNITS_SAVED
};
typedef struct {
    int type;           /* type as above */
    Player *player;     /* player this condition is checked for */
    int x,y;            /* special */
    int count;           
    char tag[32];       /* tag of unit group */
} VSubCond;
typedef struct {
    VSubCond *subconds_or;    /* sub conditions linked with logical or */
    VSubCond *subconds_and;   /* sub conditions linked with logical and */
    int sub_or_count;
    int sub_and_count;
    char result[64];
    char message[128];
} VCond;

/*
====================================================================
Load a scenario. 
====================================================================
*/
int scen_load( char *fname );

/*
====================================================================
Load a scenario description (newly allocated string)
and setup the setup :) except the type which is set when the 
engine performs the load action.
====================================================================
*/
int scen_load_info( char *info, char *fname );

/*
====================================================================
Fill the scenario part in 'setup' with the loaded player 
information.
====================================================================
*/
void scen_set_setup();

/*
====================================================================
Clear the scenario stuff pointers in 'setup' 
(loaded by scen_load_info())
====================================================================
*/
void scen_clear_setup();

/*
====================================================================
Delete scenario
====================================================================
*/
void scen_delete();

/*
====================================================================
Check if unit is destroyed, set the current attack and movement.
If SCEN_PREP_UNIT_FIRST is passed the entrenchment is not modified.
====================================================================
*/
enum {
    SCEN_PREP_UNIT_FIRST = 0,
    SCEN_PREP_UNIT_NORMAL,
    SCEN_PREP_UNIT_DEPLOY
};
void scen_prep_unit( Unit *unit, int type );

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
int scen_check_result( int after_last_turn );
/*
====================================================================
Return True if scenario is done.
====================================================================
*/
int scen_done();
/*
====================================================================
Return result string.
====================================================================
*/
char *scen_get_result();
/*
====================================================================
Return result message
====================================================================
*/
char *scen_get_result_message();
/*
====================================================================
Clear result and message
====================================================================
*/
void scen_clear_result();

/*
====================================================================
Check the supply level of a unit. (hex tiles with SUPPLY_GROUND
have 100% supply.
====================================================================
*/
void scen_adjust_unit_supply_level( Unit *unit );

/*
====================================================================
Get current weather/forecast
====================================================================
*/
int scen_get_weather( void );
int scen_get_forecast( void );

/*
====================================================================
Get date string of current date.
====================================================================
*/
void scen_get_date( char *date_str );

/*
====================================================================
Get/Add casualties for unit class and player.
====================================================================
*/
int scen_get_casualties( int player, int class );
int scen_inc_casualties( int player, int class );
/*
====================================================================
Add casualties for unit. Regard unit and transport classes.
====================================================================
*/
int scen_inc_casualties_for_unit( struct _Unit *unit );

#endif

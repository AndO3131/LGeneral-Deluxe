/***************************************************************************
                          unit.h  -  description
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

#ifndef __UNIT_H
#define __UNIT_H

#include "unit_lib.h"
#include "nation.h"
#include "player.h"
#include "terrain.h"

//#define DEBUG_ATTACK

/*
====================================================================
Embark types for a unit.
====================================================================
*/
enum {
    EMBARK_NONE = 0,
    EMBARK_GROUND,
    EMBARK_SEA,
    EMBARK_AIR,
    DROP_BY_PARACHUTE
};

/*
====================================================================
Looking direction of units (used as icon id).
====================================================================
*/
enum {
    UNIT_ORIENT_RIGHT = 0,
    UNIT_ORIENT_LEFT,
    UNIT_ORIENT_UP = 0,
    UNIT_ORIENT_RIGHT_UP,
    UNIT_ORIENT_RIGHT_DOWN,
    UNIT_ORIENT_DOWN,
    UNIT_ORIENT_LEFT_DOWN,
    UNIT_ORIENT_LEFT_UP
};

/* unit life bar stuff */
/* there aren't used colors but bitmaps with small colored tiles */
enum {
    BAR_WIDTH = 31,
    BAR_HEIGHT = 4,
    BAR_TILE_WIDTH = 3,
    BAR_TILE_HEIGHT = 4
};

/* core and auxiliary flags */
enum{
    AUXILIARY = 0,
    CORE,
    STARTING_CORE
};

/*
====================================================================
Tactical unit.
We allow unit merging so the basic properties may change and
are therefore no pointers into the library but shallow copies
(this means an entry's id, name, icons, sounds ARE pointers 
and will not be touched).
To determine wether a unit has a transporter unit->trsp_prop->id
is checked. ( 0 = no transporter, 1 = has transporter )
====================================================================
*/
typedef struct _Unit {
    Unit_Lib_Entry prop;        /* properties */
    Unit_Lib_Entry trsp_prop;   /* transporters properties */
    Unit_Lib_Entry land_trsp_prop; /* land transporters properties (used during sea transport) */
    Unit_Lib_Entry *sel_prop;   /* selected props: either prop or trsp_prop */
    struct _Unit *backup;       /* used to backup various unit values that may temporaly change (e.g. expected losses) */
    char name[24];              /* unit name */
    Player *player;             /* controlling player */
    Nation *nation;             /* nation unit belongs to */
    Terrain_Type *terrain;      /* terrain the unit is currently on */
    int x, y;                   /* map position */
    int str;                    /* strength */
    int max_str;                /* max strength unit can replace */
    int cur_str_repl;           /* strength unit can currently replace */
    int repl_exp_cost;          /* experience lost if replaced */
    int repl_prestige_cost;     /* prestige to pay if replaced */
    int entr;                   /* entrenchment */
    int exp;                    /* experience */
    int exp_level;              /* exp level computed from exp */
    int delay;                  /* delay in turns before unit becomes avaiable
                                   as reinforcement */
    int embark;                 /* embark type */
    int orient;                 /* current orientation */
    int icon_offset;            /* offset in unit's sel_prop->icon */
    int icon_tiny_offset;       /* offset in sep_prop->tiny_icon */
    int supply_level;           /* in percent; military targets are centers of supply */
    int cur_fuel;               /* current fuel */
    int cur_ammo;               /* current ammo */
    int cur_mov;                /* movement points this turn */
    int cur_atk_count;          /* number of attacks this turn */
    int unused;                 /* unit didn't take an action this turn so far */
    int damage_bar_width;       /* current life bar width in map->life_icons */
    int damage_bar_offset;      /* offset in map->damage_icons */
    int suppr;                  /* unit suppression for a single fight 
                                   (caused by artillery, cleared after fight) */
    int turn_suppr;             /* unit suppression for whole turn
                                   caused by tactical bombing, low fuel(< 20) or 
                                   low ammo (< 2) */
    int is_guarding;            /* do not include to list when cycling units */
    int killed;                 /* 1: remove from map & delete this unit
                                   2: remove from map only
                                   3: delete only */
    int fresh_deploy;           /* if this is true this unit was deployed in this turn
                                   and as long as it is unused it may be undeployed in the 
                                   same turn */
    char tag[32];               /* if tag is set the unit belongs to a unit group that
                                   is monitored by the victory conditions units_killed()
                                   and units_saved() */
    int core;                   /* 1: CORE unit
                                   2: AUXILIARY unit */
    int eval_score;             /* between 0 - 1000 indicating the worth of the unit */
    /* AI eval */
    int target_score;           /* when targets of an AI unit are gathered this value is set
                                   to the result score for attack of unit on this target */
    char star[6][20];           /* contains battle honors info */
} __attribute((packed)) Unit;

/*
====================================================================
Create a unit by passing a Unit struct with the following stuff set:
  x, y, str, entr, exp, delay, orient, nation, player.
This function will use the passed values to create a Unit struct
with all values set then.
====================================================================
*/
Unit *unit_create( Unit_Lib_Entry *prop, Unit_Lib_Entry *trsp_prop,
                   Unit_Lib_Entry *land_trsp_prop, Unit *base );

/*
====================================================================
Delete a unit. Pass the pointer as void* to allow usage as 
callback for a list.
====================================================================
*/
void unit_delete( void *ptr );

/*
====================================================================
Give unit a generic name.
====================================================================
*/
void unit_set_generic_name( Unit *unit, int number, const char *stem );

/*
====================================================================
Update unit icon according to it's orientation.
====================================================================
*/
void unit_adjust_icon( Unit *unit );

/*
====================================================================
Adjust orientation (and adjust icon) of unit if looking towards x,y.
====================================================================
*/
void unit_adjust_orient( Unit *unit, int x, int y );

/*
====================================================================
Check if unit can supply something (ammo, fuel, anything) and 
return the amount that is supplyable.
====================================================================
*/
enum {
    UNIT_SUPPLY_AMMO = 1,
    UNIT_SUPPLY_FUEL,
    UNIT_SUPPLY_ANYTHING,
    UNIT_SUPPLY_ALL
};
int unit_check_supply( Unit *unit, int type, int *missing_ammo, int *missing_fuel );

/*
====================================================================
Supply percentage of maximum fuel/ammo/both
Return True if unit was supplied.
====================================================================
*/
int unit_supply_intern( Unit *unit, int type );
int unit_supply( Unit *unit, int type );

/*
====================================================================
Check if a unit uses fuel in it's current state (embarked or not).
====================================================================
*/
int unit_check_fuel_usage( Unit *unit );

/*
====================================================================
Add experience and compute experience level. 
Return True if levelup.
====================================================================
*/
int unit_add_exp( Unit *unit, int exp );

/*
====================================================================
Mount/unmount unit to ground transporter.
====================================================================
*/
void unit_mount( Unit *unit );
void unit_unmount( Unit *unit );

/*
====================================================================
Check if units are close to each other. This means on neighbored
hex tiles.
====================================================================
*/
int unit_is_close( Unit *unit, Unit *target );

/*
====================================================================
Check if unit may activly attack (unit initiated attack) or
passivly attack (target initated attack, unit defenses) the target.
====================================================================
*/
enum {
    UNIT_ACTIVE_ATTACK = 0,
    UNIT_PASSIVE_ATTACK,
    UNIT_DEFENSIVE_ATTACK
};
int unit_check_attack( Unit *unit, Unit *target, int type );

/*
====================================================================
Compute damage/supression the target takes when unit attacks
the target. No properties will be changed. If 'real' is set
the dices are rolled else it's a stochastical prediction.
====================================================================
*/
void unit_get_damage( Unit *aggressor, Unit *unit, Unit *target, 
                      int type, 
                      int real, int rugged_def,
                      int *damage, int *suppr );

/*
====================================================================
Execute a single fight (no defensive fire check) with random values.
unit_surprise_attack() handles an attack with a surprising target
(e.g. Out Of The Sun)
If a rugged defense occured in a normal fight (surprise_attack is
always rugged) 'rugged_def' is set.
Ammo is decreased and experience gained.
unit_normal_attack() accepts UNIT_ACTIVE_ATTACK or 
UNIT_DEFENSIVE_ATTACK as 'type' depending if this unit supports
or activly attacks.
====================================================================
*/
enum {
    AR_NONE                  = 0,            /* nothing special */
    AR_UNIT_ATTACK_BROKEN_UP = ( 1L << 1),   /* target stroke first and unit broke up attack */
    AR_UNIT_SUPPRESSED       = ( 1L << 2),   /* unit alive but fully suppressed */
    AR_TARGET_SUPPRESSED     = ( 1L << 3),   /* dito */
    AR_UNIT_KILLED           = ( 1L << 4),   /* real strength is 0 */
    AR_TARGET_KILLED         = ( 1L << 5),   /* dito */
    AR_RUGGED_DEFENSE        = ( 1L << 6),   /* target made rugged defense */
    AR_EVADED                = ( 1L << 7),   /* unit evaded */
};
int unit_normal_attack( Unit *unit, Unit *target, int type );
int unit_surprise_attack( Unit *unit, Unit *target );

/*
====================================================================
Go through a complete battle unit vs. target including known(!)
defensive support stuff and with no random modifications.
Return the final damage taken by both units.
As the terrain may have influence the id of the terrain the battle
takes place (defending unit's hex) is provided.
====================================================================
*/
void unit_get_expected_losses( Unit *unit, Unit *target, int *unit_damage, int *target_damage );

/*
====================================================================
This function checks 'units' for supporters of 'target'
that will give defensive fire to before the real battle
'unit' vs 'target' takes place. These units are put to 'df_units'
(which is not created here)
====================================================================
*/
void unit_get_df_units( Unit *unit, Unit *target, List *units, List *df_units );

/*
====================================================================
Check if unit can get replacements. Type is REPLACEMENTS or
ELITE_REPLACEMENTS.
====================================================================
*/
enum {
    REPLACEMENTS = 0,
    ELITE_REPLACEMENTS
};
int unit_check_replacements( Unit *unit, int type );

/*
====================================================================
Get strength unit recieves through replacements. Type is
REPLACEMENTS or ELITE_REPLACEMENTS.
====================================================================
*/
int unit_get_replacement_strength( Unit *unit, int type );

/*
====================================================================
Get replacements for unit. Type is REPLACEMENTS or
ELITE_REPLACEMENTS.
====================================================================
*/
void unit_replace( Unit *unit, int type );

/*
====================================================================
Check if these two units are allowed to merge with each other.
====================================================================
*/
int unit_check_merge( Unit *unit, Unit *source );

/*
====================================================================
Get the maximum strength the unit can give for a split in its
current state. Unit must have at least strength 3 remaining.
====================================================================
*/
int unit_get_split_strength( Unit *unit );

/*
====================================================================
Merge these two units: unit is the new unit and source must be
removed from map and memory after this function was called.
====================================================================
*/
void unit_merge( Unit *unit, Unit *source );

/*
====================================================================
Return True if unit uses a ground transporter.
====================================================================
*/
int unit_check_ground_trsp( Unit *unit );

/*
====================================================================
Backup unit to its backup pointer (shallow copy)
====================================================================
*/
void unit_backup( Unit *unit );
void unit_restore( Unit *unit );

/*
====================================================================
Check if target may do rugged defense
====================================================================
*/
int unit_check_rugged_def( Unit *unit, Unit *target );

/*
====================================================================
Compute the rugged defense chance.
====================================================================
*/
int unit_get_rugged_def_chance( Unit *unit, Unit *target );

/*
====================================================================
Calculate the used fuel quantity. 'cost' is the base fuel cost to be
deducted by terrain movement. The cost will be adjusted as needed.
====================================================================
*/
int unit_calc_fuel_usage( Unit *unit, int cost );

/*
====================================================================
Update unit bar.
====================================================================
*/
void unit_update_bar( Unit *unit );

/*
====================================================================
Disable all actions.
====================================================================
*/
void unit_set_as_used( Unit *unit );

/*
====================================================================
Duplicate the unit, generating new name (for splitting) or without
new name (for campaign restart info).
====================================================================
*/
Unit *unit_duplicate( Unit *unit, int generate_new_name );

/*
====================================================================
Check if unit has low ammo or fuel.
====================================================================
*/
int unit_low_fuel( Unit *unit );
int unit_low_ammo( Unit *unit );

/*
====================================================================
Check whether unit can be considered for deployment.
====================================================================
*/
int unit_supports_deploy( Unit *unit );

/*
====================================================================
Resets unit attributes (prepare unit for next scenario).
====================================================================
*/
void unit_reset_attributes( Unit *unit );

#endif

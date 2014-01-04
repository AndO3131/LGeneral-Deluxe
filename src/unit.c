/***************************************************************************
                          unit.c  -  description
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
#include "list.h"
#include "unit.h"
#include "localize.h"
#include "map.h"
#include "campaign.h"

/*
====================================================================
Externals
====================================================================
*/
extern int scen_get_weather( void );
extern Sdl sdl;
extern Config config;
extern int trgt_type_count;
extern List *vis_units; /* units known zo current player */
extern List *units;
extern int cur_weather;
extern Weather_Type *weather_types;
extern Unit *cur_target;
extern Map_Tile **map;
extern int camp_loaded;
extern Camp_Entry *camp_cur_scen;

/*
====================================================================
Locals
====================================================================
*/

//#define DEBUG_ATTACK

/*
====================================================================
Update unit's bar info according to strength.
====================================================================
*/
static void update_bar( Unit *unit )
{
    /* bar width */
    unit->damage_bar_width = unit->str * BAR_TILE_WIDTH;
    if ( unit->damage_bar_width == 0 && unit->str > 0 )
        unit->damage_bar_width = BAR_TILE_WIDTH;
    /* bar color is defined by vertical offset in map->life_icons */
    if ( unit->str > 4 )
        unit->damage_bar_offset = 0;
    else
        if ( unit->str > 2 )
            unit->damage_bar_offset = BAR_TILE_HEIGHT;
        else
            unit->damage_bar_offset = BAR_TILE_HEIGHT * 2;
}

/*
====================================================================
Get the current unit strength which is:
  max { 0, unit->str - unit->suppr - unit->turn_suppr }
====================================================================
*/
static int unit_get_cur_str( Unit *unit )
{
    int cur_str = unit->str - unit->suppr - unit->turn_suppr;
    if ( cur_str < 0 ) cur_str = 0;
    return cur_str;
}

/*
====================================================================
Apply suppression and damage to unit. Return the remaining 
actual strength.
If attacker is a bomber the suppression is counted as turn
suppression.
====================================================================
*/
static int unit_apply_damage( Unit *unit, int damage, int suppr, Unit *attacker )
{
    unit->str -= damage;
    if ( unit->str < 0 ) {
        unit->str = 0;
        return 0;
    }
    if ( attacker && attacker->sel_prop->flags & TURN_SUPPR ) {
        unit->turn_suppr += suppr;
        if ( unit->str - unit->turn_suppr - unit->suppr < 0 ) {
            unit->turn_suppr = unit->str - unit->suppr;
            return 0;
        }
    }
    else {
        unit->suppr += suppr;
        if ( unit->str - unit->turn_suppr - unit->suppr < 0 ) {
            unit->suppr = unit->str - unit->turn_suppr;
            return 0;
        }
    }
    return unit_get_cur_str( unit );
}

/** Add prestige for player of unit @unit, based on properties of attacked 
 * unit @target and the damage @target_dam inflicted to it. */
static void unit_gain_prestige( Unit *unit, const Unit *target, int target_dam )
{
	int gain = 0;
	
	/* I played around with PG a little but did not figure out how the
	 * formula works. Some points seem to hold true though:
	 * - own damage/unit loss does not give prestige penalty
	 *   (therefore we don't consider unit damage here)
	 * - if target is damaged but not destroyed then some random prestige 
	 *   is gained based on cost, experience and damage
	 * - if target is destroyed then more prestige is gained based on
	 *   cost and experience with just a little random modification
	 *
	 * FIXME: the formula used when unit is destroyed seems to be quite
	 * good, but the one for mere damage not so much especially when
	 * experience increases: rolling the dice more times does not increase
	 * chance very much for low cost units so we give less prestige for 
	 * cheap, experienced units compared to PG... */ 

	if (target->str == 0) {
		/* if unit destroyed: ((1/24 of cost) * exp) +- 10% */
		gain = ((target->exp_level + 1) * target->prop.cost * 10 + 
							120/*round up*/) / 240;
		gain = ((111 - DICE(21)) * gain) / 100;
		if (gain == 0)
			gain = 1;
	} else {
		/* if damaged: try half damage multiplied by experience throws
		 * on 50-dice against one tenth of cost. at maximum four times 
		 * experience can be gained */
		int throws = ((target_dam + 1) / 2) * (target->exp_level+1);
		int i, limit = 4 * (target->exp_level+1) - 1;
		for (i = 0; i < throws; i++)
			if( DICE(50) < target->prop.cost/10 ) {
				gain++;
				if (gain == limit)
					break;
			}
	}
	
	unit->player->cur_prestige += gain;
	//printf("%s attacked %s (%d damage%s): prestige +%d\n",
	//			unit->name, target->name, target_dam,
	//			(target->str==0)?", killed":"", gain);
}

/*
====================================================================
Execute a single fight (no defensive fire check) with random
values. (only if 'luck' is set)
If 'force_rugged is set'. Rugged defense will be forced.
====================================================================
*/
enum { 
    ATK_BOTH_STRIKE = 0,
    ATK_UNIT_FIRST,
    ATK_TARGET_FIRST,
    ATK_NO_STRIKE
};
static int unit_attack( Unit *unit, Unit *target, int type, int real, int force_rugged )
{
    int unit_old_str = unit->str;//, target_old_str = target->str;
    int unit_old_ini = unit->sel_prop->ini, target_old_ini = target->sel_prop->ini;
    int unit_dam = 0, unit_suppr = 0, target_dam = 0, target_suppr = 0;
    int rugged_chance, rugged_def = 0;
    int exp_mod;
    int ret = AR_NONE; /* clear flags */
    int strike;
    /* check if rugged defense occurs */
    if ( real && type == UNIT_ACTIVE_ATTACK )
        if ( unit_check_rugged_def( unit, target ) || ( force_rugged && unit_is_close( unit, target ) ) ) {
            rugged_chance = unit_get_rugged_def_chance( unit, target );
            if ( DICE(100) <= rugged_chance || force_rugged )
                rugged_def = 1;
        }
    /* PG's formula for initiative is
       min { base initiative, terrain max initiative } +
       ( exp_level + 1 ) / 2 + D3 */
    /* against aircrafts the initiative is used since terrain does not matter */
    /* target's terrain is used for fight */
    if ( !(unit->sel_prop->flags & FLYING) && !(target->sel_prop->flags & FLYING) )
    {
        unit->sel_prop->ini = MINIMUM( unit->sel_prop->ini, target->terrain->max_ini );
        target->sel_prop->ini = MINIMUM( target->sel_prop->ini, target->terrain->max_ini );
    }
    unit->sel_prop->ini += ( unit->exp_level + 1  ) / 2;
    target->sel_prop->ini += ( target->exp_level + 1  ) / 2;
    /* special initiative rules:
       antitank inits attack tank|recon: atk 0, def 99
       tank inits attack against anti-tank: atk 0, def 99
       defensive fire: atk 99, def 0
       submarine attacks: atk 99, def 0
       ranged attack: atk 99, def 0
       rugged defense: atk 0
       air unit attacks air defense: atk = def 
       non-art vs art: atk 0, def 99 */
    if ( unit->sel_prop->flags & ANTI_TANK )
        if ( target->sel_prop->flags & TANK ) {
            unit->sel_prop->ini = 0;
            target->sel_prop->ini = 99;
        }
    if ( (unit->sel_prop->flags&DIVING) || 
         (unit->sel_prop->flags&ARTILLERY) || 
         (unit->sel_prop->flags&AIR_DEFENSE) || 
         type == UNIT_DEFENSIVE_ATTACK
    ) {
        unit->sel_prop->ini = 99;
        target->sel_prop->ini = 0;
    }
    if ( unit->sel_prop->flags & FLYING )
        if ( target->sel_prop->flags & AIR_DEFENSE )
            unit->sel_prop->ini = target->sel_prop->ini;
    if ( rugged_def )
        unit->sel_prop->ini = 0;
    if ( force_rugged )
        target->sel_prop->ini = 99;
    /* the dice is rolled after these changes */
    if ( real ) {
        unit->sel_prop->ini += DICE(3);
        target->sel_prop->ini += DICE(3);
    }
#ifdef DEBUG_ATTACK
    if ( real ) {
        printf( "%s Initiative: %i\n", unit->name, unit->sel_prop->ini );
        printf( "%s Initiative: %i\n", target->name, target->sel_prop->ini );
        if ( unit_check_rugged_def( unit, target ) )
            printf( "\nRugged Defense: %s (%i%%)\n",
                    rugged_def ? "yes" : "no",
                    unit_get_rugged_def_chance( unit, target ) );
    }
#endif
    /* in a real combat a submarine may evade */
    if ( real && type == UNIT_ACTIVE_ATTACK && ( target->sel_prop->flags & DIVING ) ) { 
        if ( DICE(10) <= 7 + ( target->exp_level - unit->exp_level ) / 2 )
        {
            strike = ATK_NO_STRIKE;
            ret |= AR_EVADED;
        }
        else
            strike = ATK_UNIT_FIRST;
#ifdef DEBUG_ATTACK
        printf ( "\nSubmarine Evasion: %s (%i%%)\n", 
                 (strike==ATK_NO_STRIKE)?"yes":"no",
                 10 * (7 + ( target->exp_level - unit->exp_level ) / 2) );
#endif
    }
    else
    /* who is first? */
    if ( unit->sel_prop->ini == target->sel_prop->ini )
        strike = ATK_BOTH_STRIKE;
    else
        if ( unit->sel_prop->ini > target->sel_prop->ini )
            strike = ATK_UNIT_FIRST;
        else
            strike = ATK_TARGET_FIRST;
    /* the one with the highest initiative begins first if not defensive fire or artillery */
    if ( strike == ATK_BOTH_STRIKE ) {
        /* both strike at the same time */
        unit_get_damage( unit, unit, target, type, real, rugged_def, &target_dam, &target_suppr );
        if ( unit_check_attack( target, unit, UNIT_PASSIVE_ATTACK ) )
            unit_get_damage( unit, target, unit, UNIT_PASSIVE_ATTACK, real, rugged_def, &unit_dam, &unit_suppr );
        unit_apply_damage( target, target_dam, target_suppr, unit );
        unit_apply_damage( unit, unit_dam, unit_suppr, target );
    }
    else
        if ( strike == ATK_UNIT_FIRST ) {
            /* unit strikes first */
            unit_get_damage( unit, unit, target, type, real, rugged_def, &target_dam, &target_suppr );
            if ( unit_apply_damage( target, target_dam, target_suppr, unit ) )
                if ( unit_check_attack( target, unit, UNIT_PASSIVE_ATTACK ) && type != UNIT_DEFENSIVE_ATTACK ) {
                    unit_get_damage( unit, target, unit, UNIT_PASSIVE_ATTACK, real, rugged_def, &unit_dam, &unit_suppr );
                    unit_apply_damage( unit, unit_dam, unit_suppr, target );
                }
        }
        else 
            if ( strike == ATK_TARGET_FIRST ) {
                /* target strikes first */
                if ( unit_check_attack( target, unit, UNIT_PASSIVE_ATTACK ) ) {
                    unit_get_damage( unit, target, unit, UNIT_PASSIVE_ATTACK, real, rugged_def, &unit_dam, &unit_suppr );
                    if ( !unit_apply_damage( unit, unit_dam, unit_suppr, target ) )
                        ret |= AR_UNIT_ATTACK_BROKEN_UP;
                }
                if ( unit_get_cur_str( unit ) > 0 ) {
                    unit_get_damage( unit, unit, target, type, real, rugged_def, &target_dam, &target_suppr );
                    unit_apply_damage( target, target_dam, target_suppr, unit );
                }
            }
    /* check return value */
    if ( unit->str == 0 )
        ret |= AR_UNIT_KILLED;
    else
        if ( unit_get_cur_str( unit ) == 0 )
            ret |= AR_UNIT_SUPPRESSED;
    if ( target->str == 0 )
        ret |= AR_TARGET_KILLED;
    else
        if ( unit_get_cur_str( target ) == 0 )
            ret |= AR_TARGET_SUPPRESSED;
    if ( rugged_def )
        ret |= AR_RUGGED_DEFENSE;
    if ( real ) {
        /* cost ammo */
        if ( config.supply ) {
            //if (DICE(10)<=target_old_str)
                unit->cur_ammo--;
            if ( unit_check_attack( target, unit, UNIT_PASSIVE_ATTACK ) && target->cur_ammo > 0 )
                //if (DICE(10)<=unit_old_str)
                    target->cur_ammo--;
        }
        /* costs attack */
        if ( unit->cur_atk_count > 0 ) unit->cur_atk_count--;
        /* target: loose entrenchment if damage was taken or with a unit->str*10% chance */
        if ( target->entr > 0 ) 
            if (target_dam > 0 || DICE(10)<=unit_old_str)
                target->entr--;
        /* attacker looses entrenchment if it got hurt */
        if ( unit->entr > 0 && unit_dam > 0 )
            unit->entr--;
        /* gain experience */
        exp_mod = target->exp_level - unit->exp_level;
        if ( exp_mod < 1 ) exp_mod = 1;
        unit_add_exp( unit, exp_mod * target_dam + unit_dam );
        exp_mod = unit->exp_level - target->exp_level;
        if ( exp_mod < 1 ) exp_mod = 1;
        unit_add_exp( target, exp_mod * unit_dam + target_dam );
        if ( unit_is_close( unit, target ) ) {
            unit_add_exp( unit, 10 );
            unit_add_exp( target, 10 );
        }
	/* gain prestige */
	unit_gain_prestige( unit, target, target_dam );
	unit_gain_prestige( target, unit, unit_dam );
        /* adjust life bars */
        update_bar( unit );
        update_bar( target );
    }
    unit->sel_prop->ini = unit_old_ini;
    target->sel_prop->ini = target_old_ini;
    return ret;
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Create a unit by passing a Unit struct with the following stuff set:
  name, x, y, str, entr, exp_level, delay, orient, nation, player.
This function will use the passed values to create a Unit struct
with all values set then.
====================================================================
*/
Unit *unit_create( Unit_Lib_Entry *prop, Unit_Lib_Entry *trsp_prop,
                   Unit_Lib_Entry *land_trsp_prop, Unit *base )
{
    Unit *unit = 0;
    if ( prop == 0 ) return 0;
    unit = calloc( 1, sizeof( Unit ) );
    /* shallow copy of properties */
    memcpy( &unit->prop, prop, sizeof( Unit_Lib_Entry ) );
    unit->sel_prop = &unit->prop; 
    unit->embark = EMBARK_NONE;
    /* assign the passed transporter without any check */
    if ( trsp_prop && !( prop->flags & FLYING ) && !( prop->flags & SWIMMING ) ) {
        memcpy( &unit->trsp_prop, trsp_prop, sizeof( Unit_Lib_Entry ) );
        /* a sea/air ground transporter is active per default */
        if ( trsp_prop->flags & SWIMMING ) {
            unit->embark = EMBARK_SEA;
            unit->sel_prop = &unit->trsp_prop;
            if ( land_trsp_prop )
                memcpy( &unit->land_trsp_prop, land_trsp_prop, sizeof( Unit_Lib_Entry ) );
        }
        if ( trsp_prop->flags & FLYING ) {
            unit->embark = EMBARK_AIR;
            unit->sel_prop = &unit->trsp_prop;
        }
    }
    /* copy the base values */
    unit->delay = base->delay;
    unit->x = base->x; unit->y = base->y;
    unit->str = base->str; unit->entr = base->entr;
    unit->max_str = base->max_str;
//    unit->cur_str_repl = 0;
    unit->player = base->player;
    unit->nation = base->nation;
    strcpy_lt( unit->name, base->name, 20 );
    unit_add_exp( unit, base->exp_level * 100 );
    unit->orient = base->orient;
    unit_adjust_icon( unit );
    unit->unused = 1;
    unit->supply_level = 100;
    unit->cur_ammo = unit->prop.ammo;
    unit->cur_fuel = unit->prop.fuel;
    unit->core = base->core;
    if ( unit->cur_fuel == 0 && unit->trsp_prop.id && unit->trsp_prop.fuel > 0 )
        unit->cur_fuel = unit->trsp_prop.fuel;
    strcpy_lt( unit->tag, base->tag, 31 );
    /* update life bar properties */
    update_bar( unit );
    /* allocate backup mem */
    unit->backup = calloc( 1, sizeof( Unit ) );
    return unit;
}

/*
====================================================================
Delete a unit. Pass the pointer as void* to allow usage as 
callback for a list.
====================================================================
*/
void unit_delete( void *ptr )
{
    Unit *unit = (Unit*)ptr;
    if ( unit == 0 ) return;
    if ( unit->backup ) free( unit->backup );
    free( unit );
}

/*
====================================================================
Give unit a generic name.
====================================================================
*/
void unit_set_generic_name( Unit *unit, int number, const char *stem )
{
    char numbuf[8];
    
    locale_write_ordinal_number(numbuf, sizeof numbuf, number);
    snprintf(unit->name, 24, "%s %s", numbuf, stem);
}

/*
====================================================================
Update unit icon according to it's orientation.
====================================================================
*/
void unit_adjust_icon( Unit *unit )
{
    unit->icon_offset = unit->sel_prop->icon_w * unit->orient;
    unit->icon_tiny_offset = unit->sel_prop->icon_tiny_w * unit->orient;
}

/*
====================================================================
Adjust orientation (and adjust icon) of unit if looking towards x,y.
====================================================================
*/
void unit_adjust_orient( Unit *unit, int x, int y )
{
    if ( unit->prop.icon_type != UNIT_ICON_ALL_DIRS ) {
        if ( x < unit->x )  {
            unit->orient = UNIT_ORIENT_LEFT;
            unit->icon_offset = unit->sel_prop->icon_w;
            unit->icon_tiny_offset = unit->sel_prop->icon_tiny_w;
        }
        else
            if ( x > unit->x ) {
                unit->orient = UNIT_ORIENT_RIGHT;
                unit->icon_offset = 0;
                unit->icon_tiny_offset = 0;
            }
    }
    else {
        /* not implemented yet */
    }
}

/*
====================================================================
Check if unit can supply something (ammo, fuel, anything) and 
return the amount that is supplyable.
====================================================================
*/
int unit_check_supply( Unit *unit, int type, int *missing_ammo, int *missing_fuel )
{
    int ret = 0;
    int max_fuel = unit->sel_prop->fuel;
    if ( missing_ammo )
        *missing_ammo = 0;
    if ( missing_fuel )
        *missing_fuel = 0;
    /* no supply near or already moved? */
    if ( unit->embark == EMBARK_SEA || unit->embark == EMBARK_AIR ) return 0;
    if ( unit->supply_level == 0 ) return 0;
    if ( !unit->unused ) return 0;
    /* supply ammo? */
    if ( type == UNIT_SUPPLY_AMMO || type == UNIT_SUPPLY_ANYTHING )
        if ( unit->cur_ammo < unit->prop.ammo ) {
            ret = 1;
            if ( missing_ammo )
                *missing_ammo = unit->prop.ammo - unit->cur_ammo;
        }
    if ( type == UNIT_SUPPLY_AMMO ) return ret;
    /* if we have a ground transporter assigned we need to use it's fuel as max */
    if ( unit_check_fuel_usage( unit ) && max_fuel == 0 )
        max_fuel = unit->trsp_prop.fuel;
    /* supply fuel? */
    if ( type == UNIT_SUPPLY_FUEL || type == UNIT_SUPPLY_ANYTHING )
        if ( unit->cur_fuel < max_fuel ) {
            ret = 1;
            if ( missing_fuel )
                *missing_fuel = max_fuel - unit->cur_fuel;
        }
    return ret;
}

/*
====================================================================
Supply percentage of maximum fuel/ammo/both.
_intern does not block movement etc.
Return True if unit was supplied.
====================================================================
*/
int unit_supply_intern( Unit *unit, int type )
{
    int amount_ammo, amount_fuel, max, supply_amount;
    int supplied = 0;
    /* ammo */
    if ( type == UNIT_SUPPLY_AMMO || type == UNIT_SUPPLY_ALL )
    if ( unit_check_supply( unit, UNIT_SUPPLY_AMMO, &amount_ammo, &amount_fuel ) ) {
        max = unit->cur_ammo + amount_ammo ;
        supply_amount = unit->supply_level * max / 100;
        if ( supply_amount == 0 ) supply_amount = 1; /* at least one */
        unit->cur_ammo += supply_amount;
        if ( unit->cur_ammo > max ) unit->cur_ammo = max;
        supplied = 1;
    }
    /* fuel */
    if ( type == UNIT_SUPPLY_FUEL || type == UNIT_SUPPLY_ALL )
    if ( unit_check_supply( unit, UNIT_SUPPLY_FUEL, &amount_ammo, &amount_fuel ) ) {
        max = unit->cur_fuel + amount_fuel ;
        supply_amount = unit->supply_level * max / 100;
        if ( supply_amount == 0 ) supply_amount = 1; /* at least one */
        unit->cur_fuel += supply_amount;
        if ( unit->cur_fuel > max ) unit->cur_fuel = max;
        supplied = 1;
    }
    return supplied;
}
int unit_supply( Unit *unit, int type )
{
    int supplied = unit_supply_intern(unit,type);
    if (supplied) {
        /* no other actions allowed */
        unit->unused = 0; unit->cur_mov = 0; unit->cur_atk_count = 0;
    }
    return supplied;
}

/*
====================================================================
Check if a unit uses fuel in it's current state (embarked or not).
====================================================================
*/
int unit_check_fuel_usage( Unit *unit )
{
    if ( unit->embark == EMBARK_SEA || unit->embark == EMBARK_AIR ) return 0;
    if ( unit->prop.fuel > 0 ) return 1;
    if ( unit->trsp_prop.id && unit->trsp_prop.fuel > 0 ) return 1;
    return 0;
}

/*
====================================================================
Add experience and compute experience level.
Return True if levelup.
====================================================================
*/
int unit_add_exp( Unit *unit, int exp )
{
    int old_level = unit->exp_level;
    unit->exp += exp;
    if ( unit->exp >= 500 ) unit->exp = 500;
    unit->exp_level = unit->exp / 100;
    if ( old_level < unit->exp_level && config.use_core_units && (camp_loaded != NO_CAMPAIGN) && unit->core)
    {
        int i = 0;
        while ( strcmp(unit->star[i],"") )
            i++;
        strcpy_lt(unit->star[i], camp_cur_scen->scen,20);
    }
    return ( old_level < unit->exp_level );
}

/*
====================================================================
Mount/unmount unit to ground transporter.
====================================================================
*/
void unit_mount( Unit *unit )
{
    if ( unit->trsp_prop.id == 0 || unit->embark != EMBARK_NONE ) return;
    /* set prop pointer */
    unit->sel_prop = &unit->trsp_prop;
    unit->embark = EMBARK_GROUND;
    /* adjust pic offset */
    unit_adjust_icon( unit );
    /* no entrenchment when mounting */
    unit->entr = 0;
}
void unit_unmount( Unit *unit )
{
    if ( unit->embark != EMBARK_GROUND ) return;
    /* set prop pointer */
    unit->sel_prop = &unit->prop;
    unit->embark = EMBARK_NONE;
    /* adjust pic offset */
    unit_adjust_icon( unit );
    /* no entrenchment when mounting */
    unit->entr = 0;
}

/*
====================================================================
Check if units are close to each other. This means on neighbored
hex tiles.
====================================================================
*/
int unit_is_close( Unit *unit, Unit *target )
{
    return is_close( unit->x, unit->y, target->x, target->y );
}

/*
====================================================================
Check if unit may activly attack (unit initiated attack) or
passivly attack (target initated attack, unit defenses) the target.
====================================================================
*/
int unit_check_attack( Unit *unit, Unit *target, int type )
{
    if ( target == 0 || unit == target ) return 0;
    if ( player_is_ally( unit->player, target->player ) ) return 0;
    if ( unit->sel_prop->flags & FLYING && !( target->sel_prop->flags & FLYING ) )
        if ( unit->sel_prop->rng == 0 )
            if ( unit->x != target->x || unit->y != target->y )
                return 0; /* range 0 means above unit for an aircraft */
    /* if the target flys and the unit is ground with a range of 0 the aircraft
       may only be harmed when above unit */
    if ( !(unit->sel_prop->flags & FLYING) && ( target->sel_prop->flags & FLYING ) )
        if ( unit->sel_prop->rng == 0 )
            if ( unit->x != target->x || unit->y != target->y )
                return 0;
    /* only destroyers may harm submarines */
    if ( target->sel_prop->flags & DIVING && !( unit->sel_prop->flags & DESTROYER ) ) return 0;
    if ( weather_types[cur_weather].flags & NO_AIR_ATTACK ) {
        if ( unit->sel_prop->flags & FLYING ) return 0;
        if ( target->sel_prop->flags & FLYING ) return 0;
    }
    if ( type == UNIT_ACTIVE_ATTACK ) {
        /* agressor */
        if ( unit->cur_ammo <= 0 ) return 0;
        if ( unit->sel_prop->atks[target->sel_prop->trgt_type] <= 0 ) return 0;
        if ( unit->cur_atk_count == 0 ) return 0;
        if ( !unit_is_close( unit, target ) && get_dist( unit->x, unit->y, target->x, target->y ) > unit->sel_prop->rng ) return 0;
    }
    else
    if ( type == UNIT_DEFENSIVE_ATTACK ) {
        /* defensive fire */
        if ( unit->sel_prop->atks[target->sel_prop->trgt_type] <= 0 ) return 0;
        if ( unit->cur_ammo <= 0 ) return 0;
        if ( ( unit->sel_prop->flags & ( INTERCEPTOR | ARTILLERY | AIR_DEFENSE ) ) == 0 ) return 0;
        if ( target->sel_prop->flags & ( ARTILLERY | AIR_DEFENSE | SWIMMING ) ) return 0;
        if ( unit->sel_prop->flags & INTERCEPTOR ) {
            /* the interceptor is propably not beside the attacker so the range check is different
             * can't be done here because the unit the target attacks isn't passed so 
             *  unit_get_df_units() must have a look itself 
             */
        }
        else
            if ( get_dist( unit->x, unit->y, target->x, target->y ) > unit->sel_prop->rng ) return 0;
    }
    else {
        /* counter-attack */
        if ( unit->cur_ammo <= 0 ) return 0;
        if ( !unit_is_close( unit, target ) && get_dist( unit->x, unit->y, target->x, target->y ) > unit->sel_prop->rng ) return 0;
        if ( unit->sel_prop->atks[target->sel_prop->trgt_type] == 0 ) return 0;
        /* artillery may only defend against close units */
        if ( unit->sel_prop->flags & ARTILLERY )
            if ( !unit_is_close( unit, target ) )
                return 0;
        /* you may defend against artillery only when close */
        if ( target->sel_prop->flags & ARTILLERY )
            if ( !unit_is_close( unit, target ) )
                return 0;
    }
    return 1;
}

/*
====================================================================
Compute damage/supression the target takes when unit attacks
the target. No properties will be changed. If 'real' is set
the dices are rolled else it's a stochastical prediction. 
'aggressor' is the unit that initiated the attack, either 'unit'
or 'target'. It is not always 'unit' as 'unit' and 'target are 
switched for get_damage depending on whether there is a striking
back and who had the highest initiative.
====================================================================
*/
void unit_get_damage( Unit *aggressor, Unit *unit, Unit *target, 
                      int type, 
                      int real, int rugged_def,
                      int *damage, int *suppr )
{
    int atk_strength, max_roll, min_roll, die_mod;
    int atk_grade, def_grade, diff, result;
    float suppr_chance, kill_chance;
    /* use PG's formula to compute the attack/defense grade*/
    /* basic attack */
    atk_grade = abs( unit->sel_prop->atks[target->sel_prop->trgt_type] );
#ifdef DEBUG_ATTACK
    if ( real ) printf( "\n%s attacks:\n", unit->name );
    if ( real ) printf( "  base:   %2i\n", atk_grade );
    if ( real ) printf( "  exp:    +%i\n", unit->exp_level);
#endif
    /* experience */
    atk_grade += unit->exp_level;
    /* target on a river? */
    if ( !(target->sel_prop->flags & FLYING ) )
    if ( target->terrain->flags[cur_weather] & RIVER ) {
        atk_grade += 4;
#ifdef DEBUG_ATTACK
        if ( real ) printf( "  river:  +4\n" );
#endif
    }
    /* counterattack of rugged defense unit? */
    if ( type == UNIT_PASSIVE_ATTACK && rugged_def ) {
        atk_grade += 4;
#ifdef DEBUG_ATTACK
            if ( real ) printf( "  rugdef: +4\n" );
#endif
    }
#ifdef DEBUG_ATTACK
    if ( real ) printf( "---\n%s defends:\n", target->name );
#endif
    /* basic defense */
    if ( unit->sel_prop->flags & FLYING )
        def_grade = target->sel_prop->def_air;
    else {
        def_grade = target->sel_prop->def_grnd;
        /* apply close defense? */
        if ( unit->sel_prop->flags & INFANTRY )
            if ( !( target->sel_prop->flags & INFANTRY ) )
                if ( !( target->sel_prop->flags & FLYING ) )
                    if ( !( target->sel_prop->flags & SWIMMING ) )
                    {
                        if ( target == aggressor )
                        if ( unit->terrain->flags[cur_weather]&INF_CLOSE_DEF )
                            def_grade = target->sel_prop->def_cls;
                        if ( unit == aggressor )
                        if ( target->terrain->flags[cur_weather]&INF_CLOSE_DEF )
                            def_grade = target->sel_prop->def_cls;
                    }
    }
#ifdef DEBUG_ATTACK
    if ( real ) printf( "  base:   %2i\n", def_grade );
    if ( real ) printf( "  exp:    +%i\n", target->exp_level );
#endif
    /* experience */
    def_grade += target->exp_level;
    /* attacker on a river or swamp? */
    if ( !(unit->sel_prop->flags & FLYING) )
    if ( !(unit->sel_prop->flags & SWIMMING) )
    if ( !(target->sel_prop->flags & FLYING) )
    {
        if ( unit->terrain->flags[cur_weather] & SWAMP ) 
        {
            def_grade += 4;
#ifdef DEBUG_ATTACK
            if ( real ) printf( "  swamp:  +4\n" );
#endif
        } else
        if ( unit->terrain->flags[cur_weather] & RIVER ) {
            def_grade += 4;
#ifdef DEBUG_ATTACK
            if ( real ) printf( "  river:  +4\n" );
#endif
        }
    }
    /* rugged defense? */
    if ( type == UNIT_ACTIVE_ATTACK && rugged_def ) {
        def_grade += 4;
#ifdef DEBUG_ATTACK
        if ( real ) printf( "  rugdef: +4\n" );
#endif
    }
    /* entrenchment */
    if ( unit->sel_prop->flags & IGNORE_ENTR )
        def_grade += 0;
    else {
        if ( unit->sel_prop->flags & INFANTRY )
            def_grade += target->entr / 2;
        else
            def_grade += target->entr;
#ifdef DEBUG_ATTACK
        if ( real ) printf( "  entr:   +%i\n", 
                (unit->sel_prop->flags & INFANTRY) ? target->entr / 2 : target->entr );
#endif
    }
    /* naval vs ground unit */
    if ( !(unit->sel_prop->flags & SWIMMING ) )
        if ( !(unit->sel_prop->flags & FLYING) )
            if ( target->sel_prop->flags & SWIMMING ) {
                def_grade += 8;
#ifdef DEBUG_ATTACK
                if ( real ) printf( "  naval: +8\n" );
#endif
            }
    /* bad weather? */
    if ( unit->sel_prop->rng > 0 )
        if ( weather_types[cur_weather].flags & BAD_SIGHT ) {
            def_grade += 3;
#ifdef DEBUG_ATTACK
            if ( real ) printf( "  sight: +3\n" );
#endif
        }
    /* initiating attack against artillery? */
    if ( type == UNIT_PASSIVE_ATTACK )
        if ( unit->sel_prop->flags & ARTILLERY ) {
            def_grade += 3;
#ifdef DEBUG_ATTACK
            if ( real ) printf( "  def vs art: +3\n" );
#endif
        }
    /* infantry versus anti_tank? */
    if ( target->sel_prop->flags & INFANTRY )
        if ( unit->sel_prop->flags & ANTI_TANK ) {
            def_grade += 2;
#ifdef DEBUG_ATTACK
            if ( real ) printf( "  antitnk:+2\n" );
#endif
        }
    /* no fuel makes attacker less effective */
    if ( unit_check_fuel_usage( unit ) && unit->cur_fuel == 0 )
    {
        def_grade += 4;
#ifdef DEBUG_ATTACK
            if ( real ) printf( "  lowfuel:+4\n" );
#endif
    }
    /* attacker strength */
    atk_strength = unit_get_cur_str( unit );
#ifdef DEBUG_ATTACK
    if ( real && atk_strength != unit_get_cur_str( unit ) )
        printf( "---\n%s with half strength\n", unit->name );
#endif
    /*  PG's formula:
        get difference between attack and defense
        strike for each strength point with 
          if ( diff <= 4 ) 
              D20 + diff
          else
              D20 + 4 + 0.4 * ( diff - 4 )
        suppr_fire flag set: 1-10 miss, 11-18 suppr, 19+ kill
        normal: 1-10 miss, 11-12 suppr, 13+ kill */
    diff = atk_grade - def_grade; if ( diff < -7 ) diff = -7;
    *damage = 0; *suppr = 0;
#ifdef DEBUG_ATTACK
    if ( real ) {
        printf( "---\n%i x %i --> %i x %i\n", 
                atk_strength, atk_grade, unit_get_cur_str( target ), def_grade );
    }
#endif
    /* get the chances for suppression and kills (computed here
       to use also for debug info */
    suppr_chance = kill_chance = 0;
    die_mod = ( diff <= 4 ? diff : 4 + 2 * ( diff - 4 ) / 5 );
    min_roll = 1 + die_mod; max_roll = 20 + die_mod;
    /* get chances for suppression and kills */
    if ( unit->sel_prop->flags & SUPPR_FIRE ) {
        int limit = (type==UNIT_DEFENSIVE_ATTACK)?20:18;
        if (limit-min_roll>=0)
            suppr_chance = 0.05*(MINIMUM(limit,max_roll)-MAXIMUM(11,min_roll)+1);
        if (max_roll>limit)
            kill_chance = 0.05*(max_roll-MAXIMUM(limit+1,min_roll)+1);
    }
    else {
        if (12-min_roll>=0)
            suppr_chance = 0.05*(MINIMUM(12,max_roll)-MAXIMUM(11,min_roll)+1);
        if (max_roll>12)
            kill_chance = 0.05*(max_roll-MAXIMUM(13,min_roll)+1);
    }
    if (suppr_chance<0) suppr_chance=0; if (kill_chance<0) kill_chance=0;
    if ( real ) {
#ifdef DEBUG_ATTACK
        printf( "Roll: D20 + %i (Kill: %i%%, Suppr: %i%%)\n", 
                diff <= 4 ? diff : 4 + 2 * ( diff - 4 ) / 5,
                (int)(100 * kill_chance), (int)(100 * suppr_chance) );
#endif
        while ( atk_strength-- > 0 ) {
            if ( diff <= 4 )
                result = DICE(20) + diff;
            else
                result = DICE(20) + 4 + 2 * ( diff - 4 ) / 5;
            if ( unit->sel_prop->flags & SUPPR_FIRE ) {
                int limit = (type==UNIT_DEFENSIVE_ATTACK)?20:18;
                if ( result >= 11 && result <= limit )
                    (*suppr)++;
                else
                    if ( result >= limit+1 )
                        (*damage)++;
            }
            else {
                if ( result >= 11 && result <= 12 )
                    (*suppr)++;
                else
                    if ( result >= 13 )
                        (*damage)++;
            }
        }
#ifdef DEBUG_ATTACK
        printf( "Kills: %i, Suppression: %i\n\n", *damage, *suppr );
#endif
    }
    else {
        *suppr = (int)(suppr_chance * atk_strength);
        *damage = (int)(kill_chance * atk_strength);
    }
}

/*
====================================================================
Execute a single fight (no defensive fire check) with random values.
unit_surprise_attack() handles an attack with a surprising target
(e.g. Out Of The Sun)
If a rugged defense occured in a normal fight (surprise_attack is
always rugged) 'rugged_def' is set.
====================================================================
*/
int unit_normal_attack( Unit *unit, Unit *target, int type )
{
    return unit_attack( unit, target, type, 1, 0 );
}
int unit_surprise_attack( Unit *unit, Unit *target )
{
    return unit_attack( unit, target, UNIT_ACTIVE_ATTACK, 1, 1 );
}

/*
====================================================================
Go through a complete battle unit vs. target including known(!)
defensive support stuff and with no random modifications.
Return the final damage taken by both units.
As the terrain may have influence the id of the terrain the battle
takes place (defending unit's hex) is provided.
====================================================================
*/
void unit_get_expected_losses( Unit *unit, Unit *target, int *unit_damage, int *target_damage )
{
    int damage, suppr;
    Unit *df;
    List *df_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
#ifdef DEBUG_ATTACK
    printf( "***********************\n" );
#endif    
    unit_get_df_units( unit, target, vis_units, df_units );
    unit_backup( unit ); unit_backup( target );
    /* let defensive fire go to work (no chance to defend against this) */
    list_reset( df_units );
    while ( ( df = list_next( df_units ) ) ) {
        unit_get_damage( unit, df, unit, UNIT_DEFENSIVE_ATTACK, 0, 0, &damage, &suppr );
        if ( !unit_apply_damage( unit, damage, suppr, 0 ) ) break;
    }
    /* actual fight if attack has strength remaining */
    if ( unit_get_cur_str( unit ) > 0 )
        unit_attack( unit, target, UNIT_ACTIVE_ATTACK, 0, 0 );
    /* get done damage */
    *unit_damage = unit->str;
    *target_damage = target->str;
    unit_restore( unit ); unit_restore( target );
    *unit_damage = unit->str - *unit_damage;
    *target_damage = target->str - *target_damage;
    list_delete( df_units );
}

/*
====================================================================
This function checks 'units' for supporters of 'target'
that will give defensive fire to before the real battle
'unit' vs 'target' takes place. These units are put to 'df_units'
(which is not created here)

Only adjacent units are defended and only ONE (randomly chosen) 
supporter will actually fire as long as it does not have the flag
'full_def_str'. (This means, all units with that flag plus one 
normal supporter will fire.)
====================================================================
*/
void unit_get_df_units( Unit *unit, Unit *target, List *units, List *df_units )
{
    Unit *entry;
    list_clear( df_units );
    if ( unit->sel_prop->flags & FLYING ) {
        list_reset( units );
        while ( ( entry = list_next( units ) ) ) {
            if ( entry->killed > 1 ) continue;
            if ( entry == target ) continue;
            if ( entry == unit ) continue;
            /* bombers -- intercepting impossibly covered by unit_check_attack() */
            if ( !(target->sel_prop->flags & INTERCEPTOR) )
                if ( unit_is_close( target, entry ) )
                    if ( entry->sel_prop->flags & INTERCEPTOR )
                        if ( player_is_ally( entry->player, target->player ) )
                            if ( entry->cur_ammo > 0 ) {
                                list_add( df_units, entry );
                                continue;
                            }
            /* air-defense */
            if ( entry->sel_prop->flags & AIR_DEFENSE )
                /* FlaK will not give support when an air-to-air attack is
                 * taking place. First, in reality it would prove distastrous,
                 * second, Panzer General doesn't allow it, either.
                 */
                if ( !(target->sel_prop->flags & FLYING) )
                    if ( unit_is_close( target, entry ) ) /* adjacent help only */
                        if ( unit_check_attack( entry, unit, UNIT_DEFENSIVE_ATTACK ) )
                            list_add( df_units, entry );
        }
    }
    else if ( unit->sel_prop->rng==0 ) { 
        /* artillery for melee combat; if unit attacks ranged, there is no 
           support */
        list_reset( units );
        while ( ( entry = list_next( units ) ) ) {
            if ( entry->killed > 1 ) continue;
            if ( entry == target ) continue;
            if ( entry == unit ) continue;
            /* HACK: An artillery with range 1 cannot support adjacent units but
               should do so. So we allow to give defensive fire on a range of 2
               like a normal artillery */
            if ( entry->sel_prop->flags & ARTILLERY && entry->sel_prop->rng == 1 )
                if ( unit_is_close( target, entry ) )
                    if ( player_is_ally( entry->player, target->player ) )
                        if ( entry->cur_ammo > 0 ) {
                            list_add( df_units, entry );
                            continue;
                        }
            /* normal artillery */
            if ( entry->sel_prop->flags & ARTILLERY )
                if ( unit_is_close( target, entry ) ) /* adjacent help only */
                    if ( unit_check_attack( entry, unit, UNIT_DEFENSIVE_ATTACK ) )
                        list_add( df_units, entry );
        }
    }
    /* randomly remove all support but one */
    if (df_units->count>0)
    {
        entry = list_get(df_units,rand()%df_units->count);
        list_clear(df_units);
        list_add( df_units, entry );
    }
}

/*
====================================================================
Check if these two units are allowed to merge with each other.
====================================================================
*/
int unit_check_merge( Unit *unit, Unit *source )
{
    /* units must not be sea/air embarked */
    if ( unit->embark != EMBARK_NONE || source->embark != EMBARK_NONE ) return 0;
    /* same class */
    if ( unit->prop.class != source->prop.class ) return 0;
    /* same player */
    if ( !player_is_ally( unit->player, source->player ) ) return 0;
    /* first unit must not have moved so far */
    if ( !unit->unused ) return 0;
    /* both units must have same movement type */
    if ( unit->prop.mov_type != source->prop.mov_type ) return 0;
    /* the unit strength must not exceed limit */
    if ( unit->str + source->str > 13 ) return 0;
    /* fortresses (unit-class 7) could not merge */
    if ( unit->prop.class == 7 ) return 0;
    /* artillery with different ranges may not merge */
    if (unit->prop.flags&ARTILLERY && unit->prop.rng!=source->prop.rng) return 0;
    /* not failed so far: allow merge */
    return 1;
}

/*
====================================================================
Check if unit can get replacements. type is REPLACEMENTS or
ELITE_REPLACEMENTS
====================================================================
*/
int unit_check_replacements( Unit *unit, int type )
{
    /* unit must not be sea/air embarked */
    if ( unit->embark != EMBARK_NONE ) return 0;
    /* unit must not have moved so far */
    if ( !unit->unused ) return 0;
    /* the unit strength must not exceed limit */
    if (type == REPLACEMENTS)
    {
        if ( unit->str >= unit->max_str ) return 0;
    }
    else
    {
        if ( unit->str >= unit->max_str + unit->exp_level ) return 0;
    }
    /* unit is airborne and not on airfield */
    if ( (unit->prop.flags & FLYING) && ( map_is_allied_depot(&map[unit->x][unit->y],unit) == 0) ) return 0;
    /* unit is naval and not in port */
    if ( (unit->prop.flags & SWIMMING) && ( map_is_allied_depot(&map[unit->x][unit->y],unit) == 0) ) return 0;
    /* unit recieves 0 strength by calculations */
    if ( unit_get_replacement_strength(unit,type) < 1 ) return 0;
    /* not failed so far: allow replacements */
    return 1;
}

/*
====================================================================
Get the maximum strength the unit can give for a split in its
current state. Unit must have at least strength 3 remaining.
====================================================================
*/
int unit_get_split_strength( Unit *unit )
{
    if ( unit->embark != EMBARK_NONE ) return 0;
    if ( !unit->unused ) return 0;
    if ( unit->str <= 4 ) return 0;
    if ( unit->prop.class == 7 ) return 0; /* fortress */
    return unit->str - 4;
}

/*
====================================================================
Merge these two units: unit is the new unit and source must be
removed from map and memory after this function was called.
====================================================================
*/
void unit_merge( Unit *unit, Unit *source )
{
    /* units relative weight */
    float weight1, weight2, total;
    int i, neg;
    /* compute weight */
    weight1 = unit->str; weight2 = source->str;
    total = unit->str + source->str;
    /* adjust so weight1 + weigth2 = 1 */
    weight1 /= total; weight2 /= total;
    /* no other actions allowed */
    unit->unused = 0; unit->cur_mov = 0; unit->cur_atk_count = 0;
    /* update cost since used for gaining prestige */
    unit->prop.cost = (unit->prop.cost * unit->str + 
					source->prop.cost * source->str) / 
					(unit->str + source->str);
    /* repair damage */
    unit->str += source->str;
    /* reorganization costs some entrenchment: the new units are assumed to have
       entrenchment 0 since they come. new entr is rounded weighted sum */
    unit->entr = floor((float)unit->entr*weight1+0.5); /* + 0 * weight2 */
    /* update experience */
    i = (int)( weight1 * unit->exp + weight2 * source->exp );
    unit->exp = 0; unit_add_exp( unit, i );
    /* update unit::prop */
    /* related initiative */
    unit->prop.ini = (int)( weight1 * unit->prop.ini + weight2 * source->prop.ini );
    /* minimum movement */
    if ( source->prop.mov < unit->prop.mov )
        unit->prop.mov = source->prop.mov;
    /* maximum spotting */
    if ( source->prop.spt > unit->prop.spt )
        unit->prop.spt = source->prop.spt;
    /* maximum range */
    if ( source->prop.rng > unit->prop.rng )
        unit->prop.rng = source->prop.rng;
    /* relative attack count */
    unit->prop.atk_count = (int)( weight1 * unit->prop.atk_count + weight2 * source->prop.atk_count );
    if ( unit->prop.atk_count == 0 ) unit->prop.atk_count = 1;
    /* relative attacks */
    /* if attack is negative simply use absolute value; only restore negative if both units are negative */
    for ( i = 0; i < trgt_type_count; i++ ) {
        neg = ( unit->prop.atks[i] < 0 && source->prop.atks[i] < 0 );
        unit->prop.atks[i] = (int)( weight1 * abs( unit->prop.atks[i] ) + weight2 * ( source->prop.atks[i] ) );
        if ( neg ) unit->prop.atks[i] *= -1;
    }
    /* relative defence */
    unit->prop.def_grnd = (int)( weight1 * unit->prop.def_grnd + weight2 * source->prop.def_grnd );
    unit->prop.def_air = (int)( weight1 * unit->prop.def_air + weight2 * source->prop.def_air );
    unit->prop.def_cls = (int)( weight1 * unit->prop.def_cls + weight2 * source->prop.def_cls );
    /* relative ammo */
    unit->prop.ammo = (int)( weight1 * unit->prop.ammo + weight2 * source->prop.ammo );
    unit->cur_ammo = (int)( weight1 * unit->cur_ammo + weight2 * source->cur_ammo );
    /* relative fuel */
    unit->prop.fuel = (int)( weight1 * unit->prop.fuel + weight2 * source->prop.fuel );
    unit->cur_fuel = (int)( weight1 * unit->cur_fuel + weight2 * source->cur_fuel );
    /* merge flags */
    unit->prop.flags |= source->prop.flags;
    /* sounds, picture are kept */
    /* unit::trans_prop isn't updated so far: */
    /* transporter of first unit is kept if any else second unit's transporter is used */
    if ( unit->trsp_prop.id == 0 && source->trsp_prop.id ) {
        memcpy( &unit->trsp_prop, &source->trsp_prop, sizeof( Unit_Lib_Entry ) );
        /* as this must be a ground transporter copy current fuel value */
        unit->cur_fuel = source->cur_fuel;
    }
    update_bar( unit );
}

/*
====================================================================
Get strength unit recieves through replacements. Type is
REPLACEMENTS or ELITE_REPLACEMENTS.
====================================================================
*/
int unit_get_replacement_strength( Unit *unit, int type )
{
    int replacement_number = 0, replacement_cost;
    Unit *entry;
    List *close_units;
    close_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    list_reset( units );

    /* searching adjacent enemy units influence */
    while ( ( entry = list_next( units ) ) ) {
        if ( entry->killed > 1 ) continue;
        if ( entry == unit ) continue;
        if ( unit_is_close( unit, entry ) )
            if ( !player_is_ally( entry->player, unit->player ) )
                list_add( close_units, entry );
    }

    /* for elite replacements overstrength */
    if ( (unit->max_str <= unit->str) && (unit->str < unit->max_str + unit->exp_level ) )
        replacement_number = 1;
    else
    /* for naval units */
        if (unit->prop.flags & SWIMMING)
            if ( (unit->prop.flags & DIVING) || (unit->prop.flags & DESTROYER) )
                replacement_number = MINIMUM( 2, unit->max_str - unit->str );
            else
                replacement_number = MINIMUM( 1, unit->max_str - unit->str );
        else
            replacement_number = unit->max_str - unit->str;

    replacement_number = (int)replacement_number * (3 - close_units->count) / 3;
    list_delete(close_units);

    /* desert terrain influence */
    if ( *(map[unit->x][unit->y].terrain->flags) & DESERT ){
        if ( replacement_number / 4 < 1 )
            replacement_number = 1;
        else
            replacement_number = replacement_number / 4;
    }

    /* evaluate replacement cost */
    replacement_number++;
    do
    {
        replacement_number--;
        replacement_cost = replacement_number * (unit->prop.cost + unit->trsp_prop.cost) / 12;
        if ( type == REPLACEMENTS )
            replacement_cost = replacement_cost / 4;
    }
    while (replacement_cost > unit->player->cur_prestige);

    unit->cur_str_repl = replacement_number;
    unit->repl_prestige_cost = replacement_cost;

    /* evaluate experience lost */
    unit->repl_exp_cost = unit->exp - (10 - unit->cur_str_repl) * unit->exp / 10;
    return replacement_number;
}

/*
====================================================================
Get replacements for unit. Type is REPLACEMENTS or
ELITE_REPLACEMENTS.
====================================================================
*/
void unit_replace( Unit *unit, int type )
{
    unit->str = unit->str + unit->cur_str_repl;
    /* evaluate replacement cost */
    unit->player->cur_prestige -= unit->repl_prestige_cost;
    /* evaluate experience cost */
    if ( type == REPLACEMENTS )
    {
        unit_add_exp( unit, -unit->repl_exp_cost );
    }

    /* supply unit */
    unit_supply_intern(unit, UNIT_SUPPLY_ALL);

    /* no other actions allowed */
    unit_set_as_used(unit);

    update_bar( unit );
}

/*
====================================================================
Return True if unit uses a ground transporter.
====================================================================
*/
int unit_check_ground_trsp( Unit *unit )
{
    if ( unit->trsp_prop.id == 0 ) return 0;
    if ( unit->trsp_prop.flags & FLYING ) return 0;
    if ( unit->trsp_prop.flags & SWIMMING ) return 0;
    return 1;
}

/*
====================================================================
Backup unit to its backup pointer (shallow copy)
====================================================================
*/
void unit_backup( Unit *unit )
{
    memcpy( unit->backup, unit, sizeof( Unit ) );
}
void unit_restore( Unit *unit )
{
    if ( unit->backup->prop.id != 0 ) {
        memcpy( unit, unit->backup, sizeof( Unit ) );
        memset( unit->backup, 0, sizeof( Unit ) );
    }
    else
        fprintf( stderr, "%s: can't restore backup: not set\n", unit->name );
}

/*
====================================================================
Check if target may do rugged defense
====================================================================
*/
int unit_check_rugged_def( Unit *unit, Unit *target )
{
    if ( ( unit->sel_prop->flags & FLYING ) || ( target->sel_prop->flags & FLYING ) )
        return 0;
    if ( ( unit->sel_prop->flags & SWIMMING ) || ( target->sel_prop->flags & SWIMMING ) )
        return 0;
    if (unit->sel_prop->flags & ARTILLERY ) return 0; /* no rugged def against range attack */
    if (unit->sel_prop->flags&IGNORE_ENTR) return 0; /* no rugged def for pioneers and such */
    if ( !unit_is_close( unit, target ) ) return 0;
    if ( target->entr == 0 ) return 0;
    return 1;
}

/*
====================================================================
Compute the targets rugged defense chance.
====================================================================
*/
int unit_get_rugged_def_chance( Unit *unit, Unit *target )
{
    /* PG's formula is
       5% * def_entr * 
       ( (def_exp_level + 2) / (atk_exp_level + 2) ) *
       ( (def_entr_rate + 1) / (atk_entr_rate + 1) ) */
    return (int)( 5.0 * target->entr *
           ( (float)(target->exp_level + 2) / (unit->exp_level + 2) ) *
           ( (float)(target->sel_prop->entr_rate + 1) / (unit->sel_prop->entr_rate + 1) ) );
}

/*
====================================================================
Calculate the used fuel quantity. 'cost' is the base fuel cost to be
deducted by terrain movement. The cost will be adjusted as needed.
====================================================================
*/
int unit_calc_fuel_usage( Unit *unit, int cost )
{
    int used = cost;
    
    /* air units use up *at least* the half of their initial movement points.
     */
    if ( unit->sel_prop->flags & FLYING ) {
        int half = unit->sel_prop->mov / 2;
        if ( used < half ) used = half;
    }
    
    /* ground units face a penalty during bad weather */
    if ( !(unit->sel_prop->flags & SWIMMING)
         && !(unit->sel_prop->flags & FLYING)
         && weather_types[scen_get_weather()].flags & DOUBLE_FUEL_COST )
        used *= 2;
    return used;
}

/*
====================================================================
Update unit bar.
====================================================================
*/
void unit_update_bar( Unit *unit )
{
    update_bar(unit);
}

/*
====================================================================
Disable all actions.
====================================================================
*/
void unit_set_as_used( Unit *unit )
{
    unit->unused = 0; 
    unit->cur_mov = 0; 
    unit->cur_atk_count = 0;
}

/*
====================================================================
Duplicate the unit, generating new name (for splitting) or without
new name (for campaign restart info).
====================================================================
*/
Unit *unit_duplicate( Unit *unit, int generate_new_name )
{
    Unit *new = calloc(1,sizeof(Unit));
    memcpy(new,unit,sizeof(Unit));
    if (generate_new_name)
        unit_set_generic_name(new, units->count + 1, unit->prop.name);
    if (unit->sel_prop==&unit->prop)
        new->sel_prop=&new->prop;
    else
        new->sel_prop=&new->trsp_prop;
    new->backup = calloc( 1, sizeof( Unit ) );
    /* terrain can't be updated here */
    return new;
}

/*
====================================================================
Check if unit has low ammo or fuel.
====================================================================
*/
int unit_low_fuel( Unit *unit )
{
    if ( !unit_check_fuel_usage( unit ) )
        return 0;
    if ( unit->sel_prop->flags & FLYING ) {
        if ( unit->cur_fuel <= 20 )
            return 1;
        return 0;
    }
    if ( unit->cur_fuel <= 10 )
        return 1;
    return 0;
}
int unit_low_ammo( Unit *unit )
{
    /* a unit is low on ammo if it has less than twenty percent of its
     * class' ammo supply left, or less than two quantities,
     * whatever value is lower
     */
    int percentage = unit->sel_prop->ammo / 5;
    return unit->embark == EMBARK_NONE && unit->cur_ammo <= MINIMUM( percentage, 2 );
}

/*
====================================================================
Check whether unit can be considered for deployment.
====================================================================
*/
int unit_supports_deploy( Unit *unit )
{
    return !(unit->prop.flags & SWIMMING) /* ships and */
           && !(unit->killed) /* killed units and */
           && unit->prop.mov > 0; /* fortresses cannot be deployed */
}

/*
====================================================================
Resets unit attributes (prepare unit for next scenario).
====================================================================
*/
void unit_reset_attributes( Unit *unit )
{
    if (unit->str < unit->max_str)
        unit->str = unit->max_str;
    unit->entr = 0;
    unit->cur_fuel = unit->prop.fuel;
    if ( unit->cur_fuel == 0 && unit->trsp_prop.id && unit->trsp_prop.fuel > 0 )
        unit->cur_fuel = unit->trsp_prop.fuel;
    unit->cur_ammo = unit->prop.ammo;
    unit->killed = 0;
    unit_unmount(unit);
    unit->cur_mov = 1;
    unit->cur_atk_count = 1;
    /* update life bar properties */
    update_bar( unit );
}

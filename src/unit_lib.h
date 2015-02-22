/***************************************************************************
                          lgeneral.h  -  description
                             -------------------
    begin                : Sat Mar 16 2002
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

#ifndef __UNIT_LIB_H
#define __UNIT_LIB_H

/*
====================================================================
Unit flags.
====================================================================
*/
enum {
    SWIMMING       = ( 1L << 1 ),   /* ship */
    DIVING         = ( 1L << 2 ),   /* aircraft */
    FLYING         = ( 1L << 3 ),   /* submarine */
    PARACHUTE      = ( 1L << 4 ),   /* may air debark anywhere */
    TRANSPORTER    = ( 1L << 7 ),   /* is a transporter */
    RECON          = ( 1L << 8 ),   /* multiple movements a round */
    ARTILLERY      = ( 1L << 9 ),   /* defensive fire */
    INTERCEPTOR    = ( 1L << 10 ),  /* protects close bombers */
    AIR_DEFENSE    = ( 1L << 11 ),  /* defensive fire */
    BRIDGE_ENG     = ( 1L << 12 ),  /* builds a bridge over rivers */
    INFANTRY       = ( 1L << 13 ),  /* is infantry */
    AIR_TRSP_OK    = ( 1L << 14 ),  /* may use air transporter */
    DESTROYER      = ( 1L << 15 ),  /* may attack submarines */
    IGNORE_ENTR    = ( 1L << 16 ),  /* ignore entrenchment of target */
    CARRIER        = ( 1L << 17 ),  /* aircraft carrier */
    CARRIER_OK     = ( 1L << 18 ),  /* may supply at aircraft carrier */
    BOMBER         = ( 1L << 19 ),  /* receives protection by interceptors */
    ATTACK_FIRST   = ( 1L << 20 ),  /* unit looses it's attack when moving */
    LOW_ENTR_RATE  = ( 1L << 21 ),  /* entrenchment rate is 1 */
    TANK           = ( 1L << 22 ),  /* is a tank */
    ANTI_TANK      = ( 1L << 23 ),  /* anti-tank (bonus against tanks) */
    SUPPR_FIRE     = ( 1L << 24 ),  /* unit primarily causes suppression when firing */
    TURN_SUPPR     = ( 1L << 25 ),  /* causes lasting suppression */
    JET            = ( 1L << 26 ),  /* airplane is a jet */
    GROUND_TRSP_OK = ( 1L << 27 ),  /* may have transporter (for purchase) */
	CARPET_BOMBING = ( 1L << 28 )   /* may temporarily destroy cities/airfields */
};

/*
====================================================================
Target types, movement types, unit classes
These may only be loaded if unit_lib_main_loaded is False.
====================================================================
*/
typedef struct {
    char *id;
    char *name;
} Trgt_Type;
typedef struct {
    char *id;
    char *name;
#ifdef WITH_SOUND    
    Wav *wav_move;
#endif    
} Mov_Type;
typedef struct {
    char *id;
    char *name;
#define UC_PT_NONE   0
#define UC_PT_NORMAL 1
#define UC_PT_TRSP   2
    int  purchase;
} Unit_Class;

/*
====================================================================
Unit map tile info icons (strength, move, attack ...)
====================================================================
*/
typedef struct {
    SDL_Surface *str;
    int str_w, str_h;
    SDL_Surface *atk;
    SDL_Surface *mov;
    SDL_Surface *guard;
} Unit_Info_Icons;
/*
====================================================================
Unit icon styles.
  SINGLE:   unit looks left and is mirrored to look right
  FIXED:    unit has fixed looking direction
  ALL_DIRS: unit provides an icon (horizontal arranged) for each
            looking direction.
====================================================================
*/
enum {
    UNIT_ICON_SINGLE = 0,
    UNIT_ICON_FIXED,
    UNIT_ICON_ALL_DIRS
};

/*
====================================================================
As we allow unit merging it must be possible to modify a shallow
copy of Unit_Lib_Entry (id, name, icons, sounds are kept). This 
means to have attacks as a pointer is quite bad. So we limit this
array to a maxium target type count.
====================================================================
*/
enum { TARGET_TYPE_LIMIT = 10 };

/*
====================================================================
Unit lib entry.
====================================================================
*/
typedef struct {
    char *id;       /* identification of this entry */
    char *name;     /* name */
    int nation;     /* nation */
    int class;      /* unit class */
    int trgt_type;  /* target type */
    int ini;        /* inititative */
    int mov;        /* movement */
    int mov_type;   /* movement type */
    int spt;        /* spotting */
    int rng;        /* attack range */
    int atk_count;  /* number of attacks per turn */
    int atks[TARGET_TYPE_LIMIT]; /* attack values (number is determined by global target_type_count) */
    int def_grnd;   /* ground defense (non-flying units) */
    int def_air;    /* air defense */
    int def_cls;    /* close defense (infantry against non-infantry) */
    int entr_rate;  /* default is 2, if flag LOW_ENTR_RATE is set it's only 1 and
                       if INFANTRY is set it's 3, this modifies the rugged defense
                       chance */
    int ammo;       /* max ammunition */
    int fuel;       /* max fuel (0 if not used) */
    SDL_Surface *icon;      /* tactical icon */
    SDL_Surface *icon_tiny; /* half the size; used to display air and ground unit at one tile */
    int icon_type;          /* either single or all_dirs */
    int icon_w, icon_h;     /* single icon size */
    int icon_tiny_w, icon_tiny_h; /* single icon size */
    int flags;
    int start_year, start_month, last_year; /* time of usage */
    int cost; /* purchase cost in prestige points */

#ifdef WITH_SOUND
    int wav_alloc;          /* if this flag is set wav_move must be freed else it's a pointer */
    Wav *wav_move;  /* pointer to the unit class default sound if wav_alloc is not set */
#endif
    int eval_score; /* between 0 - 1000 indicating the worth of the unit relative the
                       best one */
} __attribute((packed)) Unit_Lib_Entry;


/*
====================================================================
Load a unit library. If UNIT_LIB_MAIN is passed target_types,
mov_types and unit classes will be loaded (may only happen once)
====================================================================
*/
enum {
    UNIT_LIB_ADD = 0,
    UNIT_LIB_MAIN
};
int unit_lib_load( char *fname, int main );

/*
====================================================================
Delete unit library.
====================================================================
*/
void unit_lib_delete( void );

/*
====================================================================
Find unit lib entry by id string.
====================================================================
*/
Unit_Lib_Entry* unit_lib_find( char *id );

/*
====================================================================
Evaluate the unit. This score will become a relative one whereas
the best rating will be 1000. Each operational region 
ground/sea/air will have its own reference.
This evaluation is PG specific.
====================================================================
*/
void unit_lib_eval_unit( Unit_Lib_Entry *unit );

#endif

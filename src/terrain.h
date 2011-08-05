/***************************************************************************
                          terrain.h  -  description
                             -------------------
    begin                : Wed Mar 17 2002
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

#ifndef __TERRAIN_H
#define __TERRAIN_H

/*
====================================================================
Weather flags.
====================================================================
*/
enum {
    NO_AIR_ATTACK  = ( 1L << 1 ), /* flying units can't and can't be
                                     attacked */
    DOUBLE_FUEL_COST = ( 1L << 2 ), /* guess what! */
    CUT_STRENGTH = ( 1L << 3) /* cut strength in half due to bad weather */
}; 
/*
====================================================================
Weather type.
====================================================================
*/
typedef struct {
    char *id;
    char *name;
    int flags;
} Weather_Type;

/*
====================================================================
Fog alpha
====================================================================
*/
enum { FOG_ALPHA = 64, DANGER_ALPHA = 128 };

/*
====================================================================
Terrain flags
====================================================================
*/
enum {
    INF_CLOSE_DEF  = ( 1L << 1 ), /* if there's a fight inf against non-inf
                                     on this terrain the non-inf unit must use
                                     it's close defense value */
    NO_SPOTTING    = ( 1L << 2 ), /* you can't see on this tile except being on it
                                     or close to it */
    RIVER          = ( 1L << 3 ), /* engineers can build a bridge over this tile */
    SUPPLY_AIR     = ( 1L << 4 ), /* flying units may supply */
    SUPPLY_GROUND  = ( 1L << 5 ), /* ground units may supply */
    SUPPLY_SHIPS   = ( 1L << 6 ), /* swimming units may supply */
    BAD_SIGHT      = ( 1L << 7 ), /* ranged attack is harder */
    SWAMP          = ( 1L << 8 )  /* attack penalty */
};
/*
====================================================================
Terrain type.
In this case the id is a single character used to determine the
terrain type in a map.
mov, spt and flags may be different for each weather type
====================================================================
*/
typedef struct {
    char id;
    char *name;
    SDL_Surface **images;
    SDL_Surface **images_fogged;
    int *mov;
    int *spt;
    int min_entr;
    int max_entr;
    int max_ini;
    int *flags;
} Terrain_Type;

/*
====================================================================
Terrain icons
====================================================================
*/
typedef struct {
    SDL_Surface *fog;       /* mask used to create fog */
    SDL_Surface *danger;    /* mask used to create danger zone */
    SDL_Surface *grid;      /* contains the grid */
    SDL_Surface *select;    /* selecting frame picture */
    Anim *cross;            /* crosshair animation */
    Anim *expl1, *expl2;    /* explosion animation (attacker, defender)*/
#ifdef WITH_SOUND
    Wav *wav_expl;   /* explosion */
    Wav *wav_select; /* tile selection */
#endif
} Terrain_Icons;

/*
====================================================================
Load terrain types, weather information and hex tile icons.
====================================================================
*/
int terrain_load( char *fname );

/*
====================================================================
Delete terrain types & co
====================================================================
*/
void terrain_delete( void );

/*
====================================================================
Get the movement cost for a terrain type by passing movement
type id and weather id in addition.
Return -1 if all movement points are consumed
Return 0  if movement is impossible
Return cost else.
====================================================================
*/
int terrain_get_mov( Terrain_Type *type, int mov_type, int weather );

#endif

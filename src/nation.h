/***************************************************************************
                          nation.h  -  description
                             -------------------
    begin                : Wed Jan 24 2001
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

#ifndef __NATION_H
#define __NATION_H

/*
====================================================================
A nation provides a full name and a flag icon for units
(which belong to a specific nation).
====================================================================
*/
typedef struct {
    char *id;
    char *name;
    int  flag_offset;
    int  no_purchase; /* whether nation has units to purchase; 
                         updated when scenario is loaded */
} Nation;

/*
====================================================================
Load nations database.
====================================================================
*/
int nations_load( char *fname );
/*
====================================================================
Read nations from path/fname.
====================================================================
*/
int nations_load_ndb( char *fname, char *path );
/*
====================================================================
Read nations from path/fname in CSV format.
====================================================================
*/
int nations_load_lgdndb( char *fname, char *path );
/*
====================================================================
Delete nations.
====================================================================
*/
void nations_delete( void );

/*
====================================================================
Search for a nation by id string. If this fails 0 is returned.
====================================================================
*/
Nation* nation_find( char *id );

/*
====================================================================
Search for a nation by position in list. If this fails 0 is returned.
====================================================================
*/
Nation* nation_find_by_id( int id );

/*
====================================================================
Get nation index (position in list)
====================================================================
*/
int nation_get_index( Nation *nation );

/*
====================================================================
Draw flag icon to surface.
  NATION_DRAW_FLAG_NORMAL: simply draw icon.
  NATION_DRAW_FLAG_OBJ:    add a golden frame to mark as military
                           objective
====================================================================
*/
enum { NATION_DRAW_FLAG_NORMAL = 0, NATION_DRAW_FLAG_OBJ };
void nation_draw_flag( Nation *nation, SDL_Surface *surf, int x, int y, int obj );

/*
====================================================================
Get a specific pixel value in a nation's flag.
====================================================================
*/
Uint32 nation_get_flag_pixel( Nation *nation, int x, int y );

#endif

/***************************************************************************
                          slot.h  -  description
                             -------------------
    begin                : Sat Jun 23 2001
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

#ifndef __SLOT_H
#define __SLOT_H

enum { SLOT_COUNT = 11 };

/*
====================================================================
Check the save directory for saved games and add them to the 
slot list else setup a new entry: '_index_ <empty>'
====================================================================
*/
void slots_init();

/*
====================================================================
Get full slot name from id.
====================================================================
*/
char *slot_get_name( int id );

/*
====================================================================
Get slot's file name. This slot name may be passed to
slot_load/save().
====================================================================
*/
char *slot_get_fname( int id );

/*
====================================================================
Save/load game
====================================================================
*/
int slot_save( int id, char *name );
int slot_load( int id );

/*
====================================================================
Return True if slot is loadable.
====================================================================
*/
int slot_is_valid( int id );

#endif

/***************************************************************************
                          slot.h  -  description
                             -------------------
    begin                : Sat Jun 23 2001
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

#ifndef __SLOT_H
#define __SLOT_H

enum { SLOT_COUNT = 11 };

/*
====================================================================
Save/load game
====================================================================
*/
int slot_save( char *name );
int slot_load( char *name );

#endif

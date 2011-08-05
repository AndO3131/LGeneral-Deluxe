/***************************************************************************
                          maps.h -  description
                             -------------------
    begin                : Tue Mar 12 2002
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

#ifndef __MAPS_H
#define __MAPS_H

/*
====================================================================
If map_id is -1 convert all maps found in 'source_path'.
If map_id is >= 0 this is a single custom map with the data in
the current directory.
If MAPNAMES.STR is not provided in the current working directory
it is looked up in 'source_path' thus the defaults are used.
====================================================================
*/
int maps_convert( int map_id );

#endif

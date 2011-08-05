/***************************************************************************
                          strat_map.h  -  description
                             -------------------
    begin                : Fri Apr 5 2002
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

#ifndef __STRATMAP_H
#define __STRATMAP_H

/*
====================================================================
Is called after scenario was loaded in init_engine() and creates
the strategic map tile pictures, flags and the strat_map itself +
unit_layer
====================================================================
*/
void strat_map_create();

/*
====================================================================
Clean up stuff that was allocated by strat_map_create()
====================================================================
*/
void strat_map_delete();

/*
====================================================================
Update/draw the bitmap containing the strategic map.
====================================================================
*/
void strat_map_update_terrain_layer();
void strat_map_update_unit_layer();
void strat_map_draw();
    
/*
====================================================================
Handle motion input. If mouse pointer is on a map tile x,y and true
is returned.
====================================================================
*/
int strat_map_get_pos( int cx, int cy, int *x, int *y );

/*
====================================================================
Blink the dots that show unmoved units.
====================================================================
*/
void strat_map_blink();

#endif

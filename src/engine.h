/***************************************************************************
                          engine.h  -  description
                             -------------------
    begin                : Sat Feb 3 2001
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

#ifndef __ENGINE_H
#define __ENGINE_H

/*
====================================================================
Select this unit and unselect old selection if nescessary.
Clear the selection if NULL is passed as unit.
====================================================================
*/
void engine_select_unit( Unit *unit );

/*
====================================================================
If x,y is not on screen center this map tile and check if 
screencopy is possible (but only if use_sc is True)
====================================================================
*/
int engine_focus( int x, int y, int use_sc );

/*
====================================================================
Create engine (load resources that are not modified by scenario)
====================================================================
*/
int engine_create();
void engine_delete();

/*
====================================================================
Initiate engine by loading scenario either as saved game or
new scenario by the global 'setup'.
====================================================================
*/
int engine_init();
void engine_shutdown();

/*
====================================================================
Run the engine (starts with the title screen)
====================================================================
*/
void engine_run();

#endif

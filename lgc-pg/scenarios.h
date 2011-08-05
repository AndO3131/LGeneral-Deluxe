/***************************************************************************
                          scenarios.h -  description
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

#ifndef __SCENARIOS_H
#define __SCENARIOS_H

/*
====================================================================
If scen_id == -1 convert all scenarios found in 'source_path'.
If scen_id >= 0 convert single scenario from current working 
directory.
====================================================================
*/
int scenarios_convert( int scen_id );

#endif

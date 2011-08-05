/***************************************************************************
                          scenario.h  -  description
                             -------------------
    begin                : Fri Jan 19 2001
    copyright            : (C) 2001 by Michael Speck
                           (C) 2005 by Leo Savernik
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

#ifndef __SCENARIO_H
#define __SCENARIO_H

struct PData;
struct Translateables;

/** Checks whether this parse-tree resembles a scenario. */
int scen_detect( struct PData *pd );

/** Extracts translateables from a scenario into translateable-set. */
int scen_extract( struct PData *, struct Translateables * );

#endif

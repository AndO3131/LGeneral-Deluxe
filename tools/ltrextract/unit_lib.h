/***************************************************************************
                          lgeneral.h  -  description
                             -------------------
    begin                : Sat Mar 16 2002
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

#ifndef __UNIT_LIB_H
#define __UNIT_LIB_H

struct PData;
struct Translateables;

/** Checks whether this parse-tree resembles a unit_lib description. */
int unit_lib_detect( struct PData *pd );

/** Extracts translateables from unit_lib into translateable-set. */
int unit_lib_extract( struct PData *pd, struct Translateables *xl );

#endif

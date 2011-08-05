/***************************************************************************
                          units.h -  description
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

#ifndef __UNITS_H
#define __UNITS_H

/* PANZEQUP may have up to this number of units */
#define UDB_LIMIT 1000

/*
====================================================================
Check if 'source_path' contains a file PANZEQUP.EQP
====================================================================
*/
int units_find_panzequp();

/*
====================================================================
Check if 'source_path' contains a file TACICONS.SHP
====================================================================
*/
int units_find_tacicons();

/*
====================================================================
Convert unit database.
'tac_icons' is file name of the tactical icons.
====================================================================
*/
int units_convert_database( char *tac_icons );

/*
====================================================================
Convert unit graphics.
'tac_icons' is file name of the tactical icons.
====================================================================
*/
int units_convert_graphics( char *tac_icons );

#endif

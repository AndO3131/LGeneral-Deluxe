/***************************************************************************
                          terrain.c  -  description
                             -------------------
    begin                : Wed Mar 17 2002
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

#include "terrain.h"

#include "list.h"
#include "parser.h"
#include "util.h"

int terrain_detect( struct PData *pd ) {
  int dummy;
  if ( !parser_get_int( pd, "hex_width", &dummy ) ) return 0;
  if ( !parser_get_int( pd, "hex_height", &dummy ) ) return 0;
  if ( !parser_get_int( pd, "hex_x_offset", &dummy ) ) return 0;
  if ( !parser_get_int( pd, "hex_y_offset", &dummy ) ) return 0;
  return 1;
}

int terrain_extract( struct PData *pd, struct Translateables *xl )
{
    PData *sub, *subsub;
    List *entries;
    /* get weather */
    if ( !parser_get_entries( pd, "weather", &entries ) ) goto parser_failure;
    list_reset( entries );
    while ( ( sub = list_next( entries ) ) ) {
        if ( parser_get_pdata( sub, "name", &subsub ) ) {
           translateables_add_pdata(xl, subsub);
        }
    }
    /* terrain icons */
    /* terrain sounds */
    /* terrain types */
    if ( !parser_get_entries( pd, "terrain", &entries ) ) goto parser_failure;
    list_reset( entries );
    while ( ( sub = list_next( entries ) ) ) {
        /* id */
        /* name */
        if ( parser_get_pdata( sub, "name", &subsub ) ) {
            translateables_add_pdata(xl, subsub);
        }
        /* fog image */
        /* spot cost */
        /* entrenchment */
        /* initiative modification */
        /* flags */
        /* next terrain */
        /* LOG */
    }
    /* LOG */
    return 1;
parser_failure:
    fprintf( stderr, "%s: %s\n", __FUNCTION__, parser_get_error() );
    return 0;
}


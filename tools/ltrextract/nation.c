/***************************************************************************
                          nation.c  -  description
                             -------------------
    begin                : Wed Jan 24 2001
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

#include "map.h"

#include "list.h"
#include "parser.h"
#include "util.h"

int nation_detect( struct PData *pd ) {
    int dummy;
    List *entries;
    if ( !parser_get_int( pd, "icon_width", &dummy ) ) return 0;
    if ( !parser_get_int( pd, "icon_height", &dummy ) ) return 0;
    if ( !parser_get_entries( pd, "nations", &entries ) ) return 0;
    return 1;
}

int nation_extract( struct PData *pd, struct Translateables *xl )
{
    PData *sub;
    List *entries;
    /* icon size */
    /* icons */
    /* nations */
    if ( !parser_get_entries( pd, "nations", &entries ) ) goto parser_failure;
    list_reset( entries );
    while ( ( sub = list_next( entries ) ) ) {
        if ( parser_get_pdata( sub, "name", &sub ) ) {
            translateables_add_pdata(xl, sub);
        }
    }
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
    return 0;
}


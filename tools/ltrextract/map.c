/***************************************************************************
                          map.c  -  description
                             -------------------
    begin                : Mon Jan 22 2001
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

int map_detect( struct PData *pd ) {
    int dummy; char *str;
    if ( !parser_get_int( pd, "width", &dummy ) ) return 0;
    if ( !parser_get_int( pd, "height", &dummy ) ) return 0;
    if ( !parser_get_value( pd, "terrain_db", &str, 0 ) ) return 0;
    if ( !parser_get_value( pd, "tiles", &str, 0 ) ) return 0;
    return 1;
}

int map_extract( struct PData *pd, struct Translateables *xl )
{
    PData *sub;
    /* map size */
    /* load terrains */
    /* allocate map memory */
    /* map itself */
    /* map names */
    if ( parser_get_pdata( pd, "names", &sub ) ) {
        translateables_add_pdata(xl, sub);
    }
    return 1;
#ifdef __NOTUSED
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
    return 0;
#endif
}


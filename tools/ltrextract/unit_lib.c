/***************************************************************************
                          unit_lib.c  -  description
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

#include "unit_lib.h"

#include "list.h"
#include "parser.h"
#include "util.h"

int unit_lib_detect( struct PData *pd ) {
    List *entries;
    if ( !parser_get_entries( pd, "unit_lib", &entries ) ) return 0;
    return 1;
}

int unit_lib_extract( struct PData *pd, struct Translateables *xl )
{
    List *entries;
    PData *sub, *subsub;
    /* if main read target types & co */
    /* target types */
    if ( parser_get_entries( pd, "target_types", &entries ) ) {
        list_reset( entries );
        while ( ( sub = list_next( entries ) ) ) {
            if ( !parser_get_pdata( sub, "name", &subsub ) ) goto parser_failure;
            translateables_add_pdata(xl, subsub);
        }
    }
    /* movement types */
    if ( parser_get_entries( pd, "move_types", &entries ) ) {
        list_reset( entries );
        while ( ( sub = list_next( entries ) ) ) {
            if ( !parser_get_pdata( sub, "name", &subsub ) ) goto parser_failure;
            translateables_add_pdata(xl, subsub);
        }
    }
    /* unit classes */
    if ( parser_get_entries( pd, "unit_classes", &entries ) ) {
        list_reset( entries );
        while ( ( sub = list_next( entries ) ) ) {
            if ( !parser_get_pdata( sub, "name", &subsub ) ) goto parser_failure;
            translateables_add_pdata(xl, subsub);
        }
    }
    /* unit map tile icons */
    /* icons */
    /* unit lib entries */
    if ( !parser_get_entries( pd, "unit_lib", &entries ) ) goto parser_failure;
    list_reset( entries );
    while ( ( sub = list_next( entries ) ) ) {
        /* read unit entry */
        /* identification */
        /* name */
        if ( !parser_get_pdata( sub, "name", &subsub ) ) goto parser_failure;
        translateables_add_pdata(xl, subsub);
        /* class id */
        /* target type id */
        /* initiative */
        /* spotting */
        /* movement */
        /* move type id */
        /* fuel */
        /* range */
        /* ammo */
        /* attack count */
        /* attack values */
        /* ground defense */
        /* air defense */
        /* close defense */
        /* flags */
        /* set the entrenchment rate */
        /* time period of usage */
        /* icon */
        /* icon id */
        /* icon_type */
        /* get position and size in icons surface */
        /* picture is copied from unit_pics first
         * if picture_type is not ALL_DIRS, picture is a single picture looking to the right;
         * add a flipped picture looking to the left 
         */
        /* read sounds -- well as there are none so far ... */
        /* add unit to database */
        /* absolute evaluation */
    }
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
    return 0;
}


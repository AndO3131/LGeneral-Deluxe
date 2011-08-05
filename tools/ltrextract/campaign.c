/***************************************************************************
                          campaign.c  -  description
                             -------------------
    begin                : Fri Apr 5 2002
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

#include "campaign.h"

#include "list.h"
#include "parser.h"
#include "util.h"

static void camp_add_suitable_translateable(struct Translateables *xl, PData *pd) {
    /* ignore references (starting with '@') */
    if (!pd->values || !pd->values->count
        || ((const char *)list_first(pd->values))[0] == '@')
        return;
    translateables_add_pdata(xl, pd);
}

int camp_detect( struct PData *pd ) {
    List *entries;
    if ( !parser_get_entries( pd, "scenarios", &entries ) ) return 0;
    return 1;
}

int camp_extract( struct PData *pd, struct Translateables *xl )
{
    PData *sub, *subsub;
    List *entries;
    /* name, desc, authors */
    if ( !parser_get_pdata( pd, "name", &sub ) ) goto parser_failure;
    translateables_add_pdata(xl, sub);
    if ( !parser_get_pdata( pd, "desc", &sub ) ) goto parser_failure;
    translateables_add_pdata(xl, sub);
    if ( !parser_get_pdata( pd, "authors", &sub ) ) goto parser_failure;
    translateables_add_pdata(xl, sub);
    /* entries */
    if ( !parser_get_entries( pd, "scenarios", &entries ) ) goto parser_failure;
    list_reset( entries );
    while ( ( sub = list_next( entries ) ) ) {
        List *next_entries;
        if ( parser_get_pdata( sub, "title", &subsub ) )
            camp_add_suitable_translateable(xl, subsub);
        if ( !parser_get_pdata( sub, "briefing", &subsub ) ) goto parser_failure;
        camp_add_suitable_translateable(xl, subsub);
        if ( parser_get_entries( sub, "debriefings", &next_entries )
              || parser_get_entries( sub, "options", &next_entries ) ) {
            list_reset( next_entries );
            while ( ( subsub = list_next( next_entries ) ) ) {
                camp_add_suitable_translateable(xl, subsub);
            }
        }
    }
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
    return 0;
}


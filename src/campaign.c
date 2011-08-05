/***************************************************************************
                          campaign.c  -  description
                             -------------------
    begin                : Fri Apr 5 2002
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
 
#include "lgeneral.h" 
#include "parser.h"
#include "list.h"
#include "unit.h"
#include "date.h"
#include "scenario.h"
#include "campaign.h"
#include "localize.h"

/*
====================================================================
Externals
====================================================================
*/
extern Setup setup;

int camp_loaded = 0;
char *camp_fname = 0;
char *camp_name = 0;
char *camp_desc = 0;
char *camp_authors = 0;
List *camp_entries = 0; /* scenario entries */
Camp_Entry *camp_cur_scen = 0;
static char *camp_first;

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Delete campaign entry.
====================================================================
*/
static void camp_delete_entry( void *ptr )
{
    Camp_Entry *entry = (Camp_Entry*)ptr;
    if ( entry ) {
        if ( entry->id ) free( entry->id );
        if ( entry->scen ) free( entry->scen );
        if ( entry->brief ) free( entry->brief );
        if ( entry->nexts ) list_delete( entry->nexts );
        if ( entry->descs ) list_delete( entry->descs );
        free( entry );
    }
}

/* check whether all next entries have a matching scenario entry */
static void camp_verify_tree()
{
    int i;
    Camp_Entry *entry = 0;
    char *next = 0, *ptr;

    for ( i = 0; i < camp_entries->count; i++ )
    {
        entry = list_get( camp_entries, i );
        if ( entry->nexts && entry->nexts->count > 0 )
        {
            list_reset( entry->nexts );
            while ( (next = list_next(entry->nexts)) )
            {
                ptr = strchr( next, '>' ); 
                if ( ptr ) 
                    ptr++; 
                else 
                    ptr = next;
                if ( camp_get_entry(ptr) == 0 ) 
                    printf( "  (is a 'next' entry in scenario %s)\n", entry->id );
            }
        }
    }
}

/**
 * resolve a reference to a value defined in another scenario state
 * @param entries parse tree containing scenario states
 * @param key name of entry
 * @param scenstat name of scenario-state from which we resolve
 * @return the resolved value or 0 if the value cannot be resolved
 * resolved.
 */
static const char *camp_resolve_ref(PData *entries, const char *key, const char *scen_stat)
{
    const char *cur_scen_stat = scen_stat;
    char *value = (char *)scen_stat;
    do {
        PData *scen_entry;
        if (!parser_get_pdata(entries, cur_scen_stat, &scen_entry)) break;
        if (!parser_get_value(scen_entry, key, &value, 0)) {
            /* if key of referenced entry is not found, it is usually an
               error -> notify user. */
            if (cur_scen_stat != scen_stat) {
                fprintf(stderr, tr("%s: %d: warning: entry '%s' not found in '%s/%s'\n"),
                        parser_get_filename(scen_entry),
                        parser_get_linenumber(scen_entry),
                        key, entries->name, cur_scen_stat);
            }
            return 0;
        }
        
        /* is it a value? */
        if (value[0] != '@') break; /* yes, return it outright */
        
        /* otherwise, dereference it */
        cur_scen_stat = value + 1;
        
        /* check for cycles and bail out if detected */
        if (strcmp(cur_scen_stat, scen_stat) == 0) {
            fprintf(stderr, "%s: cycle detected: %s\n", __FUNCTION__, value);
            break;
        }
    } while (1);
    return value;
}

/** resolve key within entries and translate the result wrt domain */
inline static char *camp_resolve_ref_localized(PData *entries, const char *key, const char *scen_stat, const char *domain)
{
    const char *res = camp_resolve_ref(entries, key, scen_stat);
    return res ? strdup(trd(domain, res)) : 0;
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Load campaign.
====================================================================
*/
int camp_load( const char *fname )
{
    Camp_Entry *centry = 0;
    PData *pd, *scen_ent, *sub, *pdent, *subsub;
    List *entries, *next_entries;
    char path[512], str[1024];
    char *result, *next_scen;
    char *domain = 0;
    camp_delete();
    sprintf( path, "%s/campaigns/%s", get_gamedir(), fname );
    camp_fname = strdup( fname );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    /* name, desc, authors */
    if ( !parser_get_localized_string( pd, "name", domain, &camp_name ) ) goto parser_failure;
    if ( !parser_get_localized_string( pd, "desc", domain, &camp_desc ) ) goto parser_failure;
    if ( !parser_get_localized_string( pd, "authors", domain, &camp_authors ) ) goto parser_failure;
    /* entries */
    if ( !parser_get_pdata( pd, "scenarios", &scen_ent ) ) goto parser_failure;
    entries = scen_ent->entries;
    list_reset( entries );
    camp_entries = list_create( LIST_AUTO_DELETE, camp_delete_entry );
    while ( ( sub = list_next( entries ) ) ) {
        if (!camp_first) camp_first = strdup(sub->name);
        centry = calloc( 1, sizeof( Camp_Entry ) );
        centry->id = strdup( sub->name );
        parser_get_string( sub, "scenario", &centry->scen );
        centry->title = camp_resolve_ref_localized(scen_ent, "title", centry->id, domain);
        centry->brief = camp_resolve_ref_localized(scen_ent, "briefing", centry->id, domain);
        if (!centry->brief) goto parser_failure;
        if ( parser_get_entries( sub, "next", &next_entries ) ) {
            centry->nexts = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
            list_reset( next_entries );
            while ( ( subsub = list_next( next_entries ) ) ) {
                result = subsub->name;
                next_scen = list_first( subsub->values );
                snprintf( str, sizeof(str), "%s>%s", result, next_scen );
                list_add( centry->nexts, strdup( str ) );
            }
        }
        /* if scenario is given, look for subtree "debriefings", otherwise
         * for subtree "options". Internally, they're handled the same.
         */
        if ( (centry->scen && parser_get_pdata( sub, "debriefings", &pdent ))
             || parser_get_pdata( sub, "options", &pdent ) ) {
            next_entries = pdent->entries;
            centry->descs = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
            list_reset( next_entries );
            while ( ( subsub = list_next( next_entries ) ) ) {
                char key[1024];
                const char *result = subsub->name;
                const char *next_desc;
                snprintf(key, sizeof key, "%s/%s", pdent->name, subsub->name);
                next_desc = camp_resolve_ref(scen_ent, key, centry->id);
                next_desc = trd(domain, next_desc ? next_desc : list_first(subsub->values));
                snprintf( str, sizeof(str), "%s>%s", result, next_desc );
                list_add( centry->descs, strdup( str ) );
            }
        }
        list_add( camp_entries, centry );
    }
    parser_free( &pd );
    camp_loaded = 1;
    camp_verify_tree();
    free(domain);
    return 1;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
    camp_delete();
    free(domain);
    return 0;
}
/*
====================================================================
Load a campaign description (newly allocated string)
and setup the setup :) except the type which is set when the 
engine performs the load action.
====================================================================
*/
char* camp_load_info( const char *fname )
{
    PData *pd;
    char path[512];
    char *name, *desc;
    char *domain = 0;
    char *info = 0;
    sprintf( path, "%s/campaigns/%s", get_gamedir(), fname );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    if ( !parser_get_value( pd, "name", &name, 0 ) ) goto parser_failure;
    name = (char *)trd(domain, name);
    if ( !parser_get_value( pd, "desc", &desc, 0 ) ) goto parser_failure;
    desc = (char *)trd(domain, desc);
    if ( ( info = calloc( strlen( name ) + strlen( desc ) + 3, sizeof( char ) ) ) == 0 ) {
        fprintf( stderr, tr("Out of memory\n") );
        goto failure;
    }
    sprintf( info, "%s##%s", name, desc );
    strcpy( setup.fname, fname );
    setup.scen_state = 0;
    parser_free( &pd );
    free(domain);
    return info;
parser_failure:
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    if ( info ) free( info );
    parser_free( &pd );
    free(domain);
    return 0;
}

void camp_delete()
{
    if ( camp_fname ) free( camp_fname ); camp_fname = 0;
    if ( camp_name ) free( camp_name ); camp_name = 0;
    if ( camp_desc ) free( camp_desc ); camp_desc = 0;
    if ( camp_authors ) free( camp_authors ); camp_authors = 0;
    if ( camp_entries ) list_delete( camp_entries ); camp_entries = 0;
    camp_loaded = 0;
    camp_cur_scen = 0;
    free(camp_first); camp_first = 0;
    
}

/*
====================================================================
Query next campaign scenario entry by this result for the current
entry. If 'id' is NULL the first entry called 'first' is loaded.
====================================================================
*/
Camp_Entry *camp_get_entry( const char *id )
{
    const char *search_str = id ? id : camp_first;
    Camp_Entry *entry;
    list_reset( camp_entries );
    while ( ( entry = list_next( camp_entries ) ) )
        if ( strcmp( entry->id, search_str ) == 0 ) {
            return entry;
        }
    fprintf( stderr, tr("Campaign entry '%s' not found in campaign '%s'\n"), search_str, camp_name );
    return 0;
}

/*
====================================================================
Return list of result identifiers for current campaign entry.
(List of char *)
====================================================================
*/
List *camp_get_result_list()
{
    List *list = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    if (camp_cur_scen->nexts) {
        const char *str;
        list_reset( camp_cur_scen->nexts );
        while ( ( str = list_next( camp_cur_scen->nexts ) ) ) {
            List *tokens;
            if ( ( tokens = parser_split_string( str, ">" ) ) ) {
                list_add( list, strdup(list_first( tokens )) );
                list_delete( tokens );
            }
        }
    }
    return list;
}

/*
====================================================================
Return textual description of result or 0 if no description.
====================================================================
*/
const char *camp_get_description(const char *result)
{
    const char *ret = 0;
    if (!result) return 0;
    if (camp_cur_scen->descs) {
        const char *str;
        list_reset( camp_cur_scen->descs );
        while ( ( str = list_next( camp_cur_scen->descs ) ) ) {
            List *tokens;
            if ( ( tokens = parser_split_string( str, ">" ) ) ) {
                int found = 0;
                if ( STRCMP( result, list_first( tokens ) ) ) {
                    ret = strchr( str, '>' ) + 1;
                    found = 1;
                }
                list_delete( tokens );
                if ( found ) break;
            }
        }
    }
    return ret;
}

/*
====================================================================
Set the next scenario entry by searching the results in current
scenario entry. If 'id' is NULL entry 'first' is loaded
====================================================================
*/
int camp_set_next( const char *id )
{
    List *tokens;
    char *next_str;
    int found = 0;
    if ( id == 0 )
        camp_cur_scen = camp_get_entry( camp_first );
    else {
        if (!camp_cur_scen->nexts) return 0;
        list_reset( camp_cur_scen->nexts );
        while ( ( next_str = list_next( camp_cur_scen->nexts ) ) ) {
            if ( ( tokens = parser_split_string( next_str, ">" ) ) ) {
                if ( STRCMP( id, list_first( tokens ) ) ) {
                    camp_cur_scen = camp_get_entry( list_get( tokens, 2 ) );
                    found = 1;
                }
                list_delete( tokens );
                if ( found ) break;
            }
        }
    }
    return camp_cur_scen != 0;
}

/*
====================================================================
Set current scenario by camp scen entry id.
====================================================================
*/
void camp_set_cur( const char *id )
{
    camp_cur_scen = camp_get_entry( id );
}

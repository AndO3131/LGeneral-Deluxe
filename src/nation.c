/***************************************************************************
                          nation.c  -  description
                             -------------------
    begin                : Wed Jan 24 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/
/***************************************************************************
                     Modifications by LGD team 2012+.
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sdl.h"
#include "list.h"
#include "misc.h"
#include "file.h"
#include "nation.h"
#include "parser.h"
#include "localize.h"
#include "config.h"
#include "FPGE/pgf.h"

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern Config config;

/*
====================================================================
Nations
====================================================================
*/
Nation *nations = 0;
int nation_count = 0;
SDL_Surface *nation_flags = 0;
int nation_flag_width = 20, nation_flag_height = 13;
int outside_nation_flag_width = 0, outside_nation_flag_height = 0;

/*
====================================================================
Load nations database.
====================================================================
*/
int nations_load( char *fname )
{
    char *path, *extension;
    path = calloc( 256, sizeof( char ) );
    extension = calloc( 10, sizeof( char ) );
    search_file_name( path, extension, fname, config.mod_name, "Scenario", 'n' );
    if ( strcmp( extension, "ndb" ) == 0 )
        return nations_load_ndb( fname, path );
    else if ( strcmp( extension, "lgdndb" ) == 0 )
        return nations_load_lgdndb( fname, path );
    return 0;
}

/*
====================================================================
Read nations from SRCDIR/fname.
====================================================================
*/
int nations_load_ndb( char *fname, char *path )
{
    int i;
    PData *pd, *sub;
    List *entries;
    char *str;
    char *domain = 0;
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    /* icon size */
    if ( !parser_get_int( pd, "icon_width", &outside_nation_flag_width ) ) goto failure;
    if ( !parser_get_int( pd, "icon_height", &outside_nation_flag_height ) ) goto parser_failure;
    /* icons */
    if ( !parser_get_value( pd, "icons", &str, 0 ) ) goto parser_failure;
    search_file_name_exact( path, str, config.mod_name );
    if ( ( nation_flags = load_surf( path,  SDL_SWSURFACE, 
           outside_nation_flag_width, outside_nation_flag_height, nation_flag_width, nation_flag_height ) ) == 0 )
    {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }
    /* nations */
    if ( !parser_get_entries( pd, "nations", &entries ) ) goto parser_failure;
    nation_count = entries->count;
    nations = calloc( nation_count, sizeof( Nation ) );
    list_reset( entries ); i = 0;
    while ( ( sub = list_next( entries ) ) ) {
        nations[i].id = strdup( sub->name );
        if ( !parser_get_localized_string( sub, "name", domain, &nations[i].name ) ) goto parser_failure;
        if ( !parser_get_int( sub, "icon_id", &nations[i].flag_offset ) ) goto parser_failure;
        nations[i].flag_offset *= nation_flag_width;
        i++;
    }
    parser_free( &pd );
    free(domain);
    return 1;
parser_failure:        
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    nations_delete();
    if ( pd ) parser_free( &pd );
    free(domain);
    return 0;
}

/*
====================================================================
Read nations from SRCDIR/fname.
====================================================================
*/
int nations_load_lgdndb( char *fname, char *path )
{
    int i;
    char str[MAX_LINE_SHORT];
    char *domain = 0;

    FILE *inf;
    char line[1024],tokens[20][256];
    int j=0,block=0,last_line_length=-1,cursor=0,token=0;
    int utf16 = 0, lines=0;
    nation_count = 0;

    inf=fopen(path,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open nation database file\n");
        return 0;
    }

    //find number of countries
    while (load_line(inf,line,utf16)>=0)
    {
        //strip comments
        if (line[0]==0x23 || line[0]==0x09) { line[0]=0; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;
        if (block == 2 && strlen(line)>0)
            nation_count++;
    }
    nations = calloc( nation_count, sizeof( Nation ) );

    fclose(inf);

    block=0;last_line_length=-1;cursor=0;token=0;lines=0;
    inf=fopen(path,"rb");
    while (load_line(inf,line,utf16)>=0)
    {
        //count lines so error can be displayed with line number
        lines++;

        //strip comments
        if (line[0]==0x23 || line[0]==0x09) { line[0]=0; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        //Block#1 icon data
        if (block == 1 && strlen(line)>0)
        {
            if ( strcmp( tokens[0], "icons_file" ) == 0 )
            {
                strncpy(str,tokens[1],256);
            }
            if ( strcmp( tokens[0], "icon_width" ) == 0 )
            {
                outside_nation_flag_width = atoi( tokens[1] );
            }
            if ( strcmp( tokens[0], "icon_height" ) == 0 )
            {
                outside_nation_flag_height = atoi( tokens[1] );
            }
            if ( strcmp( str, "" ) != 0 && outside_nation_flag_width != 0 && outside_nation_flag_height != 0 )
            {
                search_file_name_exact( path, str, config.mod_name );
                if ( ( nation_flags = load_surf( path,  SDL_SWSURFACE, 
                       outside_nation_flag_width, outside_nation_flag_height, nation_flag_width, nation_flag_height ) ) == 0 )
                {
                    fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
                    goto failure;
                }
            }
        }
        //Block#2 nations
        if (block == 2 && strlen(line)>0)
        {
            nations[j].id = strdup( tokens[0] );
            nations[j].name = strdup( tokens[1] );
            nations[j].flag_offset = atoi( tokens[2] ) * nation_flag_width;
            j++;
        }
    }    
    fclose(inf);
    return 1;
failure:
    fclose(inf);
    nations_delete();
    free(domain);
    return 0;
}

/*
====================================================================
Delete nations.
====================================================================
*/
void nations_delete( void )
{
    int i;
    if ( nation_flags ) SDL_FreeSurface ( nation_flags ); nation_flags = 0;
    if ( nations == 0 ) return;
    for ( i = 0; i < nation_count; i++ ) {
        if ( nations[i].id ) free( nations[i].id );
        if ( nations[i].name ) free( nations[i].name );
    }
    free( nations ); nations = 0; nation_count = 0;
}

/*
====================================================================
Search for a nation by id string. If this fails 0 is returned.
====================================================================
*/
Nation* nation_find( char *id )
{
    int i;
    if ( id == 0 ) return 0;
    for ( i = 0; i < nation_count; i++ )
        if ( STRCMP( id, nations[i].id ) )
            return &nations[i];
    return 0;
}

/*
====================================================================
Search for a nation by position in list. If this fails 0 is returned.
====================================================================
*/
Nation* nation_find_by_id( int id )
{
    int i;
    for ( i = 0; i < nation_count; i++ )
        if ( i == id )
            return &nations[i];
    return 0;
}

/*
====================================================================
Get nation id (position in list)
====================================================================
*/
int nation_get_index( Nation *nation )
{
    int i;
    for ( i = 0; i < nation_count; i++ )
        if ( nation == &nations[i] )
            return i;
    return -1;
}

/*
====================================================================
Draw flag icon to surface.
  NATION_DRAW_FLAG_NORMAL: simply draw icon.
  NATION_DRAW_FLAG_OBJ:    add a golden frame to mark as military
                           objective
====================================================================
*/
void nation_draw_flag( Nation *nation, SDL_Surface *surf, int x, int y, int obj )
{
    if ( obj == NATION_DRAW_FLAG_OBJ ) {
        DEST( surf, x, y, nation_flag_width, nation_flag_height );
        fill_surf( 0xffff00 );
        DEST( surf, x + 1, y + 1, nation_flag_width - 2, nation_flag_height - 2 );
        SOURCE( nation_flags, nation->flag_offset + 1, 1 );
        blit_surf();
    }
    else {
        DEST( surf, x, y, 20, 13 );
        SOURCE( nation_flags, nation->flag_offset, 0 );
        blit_surf();
    }
}

/*
====================================================================
Get a specific pixel value in a nation's flag.
====================================================================
*/
Uint32 nation_get_flag_pixel( Nation *nation, int x, int y )
{
    return get_pixel( nation_flags, nation->flag_offset + x, y );
}


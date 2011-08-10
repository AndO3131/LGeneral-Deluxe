/***************************************************************************
                          nations.h -  description
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "shp.h" 
#include "nations.h"

/*
====================================================================
Externals
====================================================================
*/
extern char *source_path;
extern char *dest_path;
extern char target_name[128];

/*
====================================================================
Nations.
====================================================================
*/
int nation_count = 24;
char *nations[] = {
    "aus", "Austria",       "0",
    "bel", "Belgia",        "1",
    "bul", "Bulgaria",      "2",
    "lux", "Luxemburg",     "3",
    "den", "Denmark",       "4",
    "fin", "Finnland",      "5",
    "fra", "France",        "6",
    "ger", "Germany",       "7",
    "gre", "Greece",        "8",
    "usa", "USA",           "9",
    "hun", "Hungary",       "10",
    "tur", "Turkey",        "11",
    "it",  "Italy",         "12",
    "net", "Netherlands",   "13",
    "nor", "Norway",        "14",
    "pol", "Poland",        "15",
    "por", "Portugal",      "16",
    "rum", "Rumania",       "17",
    "esp", "Spain",         "18",
    "so",  "Sovjetunion",   "19",
    "swe", "Sweden",        "20",
    "swi", "Switzerland",   "21",
    "eng", "Great Britain", "22",
    "yug", "Yugoslavia",    "23"
};

/*
====================================================================
Create nations database and convert graphics.
Only called when converting full campaign. For custom campaigns only
flags may differ as nation definitions are hardcoded in converter to
PG nations.
====================================================================
*/
int nations_convert( void )
{
    int i, height = 0;
    FILE *file;
    char path[MAXPATHLEN];
    SDL_Rect srect, drect;
    PG_Shp *shp =0;
    SDL_Surface *surf = 0;
    Uint32 ckey = MAPRGB( CKEY_RED, CKEY_GREEN, CKEY_BLUE ); /* transparency key */
    
    /* nation database */
    printf( "Nation database...\n" );
    snprintf( path, MAXPATHLEN, "%s/nations/%s.ndb", dest_path, target_name );
    if ( ( file = fopen( path, "w" ) ) == 0 ) {
        fprintf( stderr, "%s: access denied\n", path );
        return 0;
    }
    fprintf( file, "@\n" );
    fprintf( file, "icons»%s.bmp\n", target_name );
    fprintf( file, "icon_width»20\nicon_height»13\n" );
    /* domain */
    fprintf( file, "domain»pg\n" );
    fprintf( file, "<nations\n" );
    for ( i = 0; i < nation_count; i++ )
        fprintf( file, "<%s\nname»%s\nicon_id»%s\n>\n", nations[i * 3], nations[i * 3 + 1], nations[i * 3 + 2] );
    fprintf( file, ">\n" );
    fclose( file );
    
    /* nation graphics */
    printf( "Nation flag graphics...\n" );
    /* create new surface */
    if ( ( shp = shp_load( "FLAGS.SHP" ) ) == 0 )
        return 0;
    for ( i = 0; i < nation_count; i++ )
        if ( shp->headers[i].valid )
            height += shp->headers[i].actual_height;
    surf = SDL_CreateRGBSurface( SDL_SWSURFACE, shp->headers[0].actual_width, height, shp->surf->format->BitsPerPixel,
                                 shp->surf->format->Rmask, shp->surf->format->Gmask, shp->surf->format->Bmask,
                                 shp->surf->format->Amask );
    if ( surf == 0 ) {
        fprintf( stderr, "error creating surface: %s\n", SDL_GetError() );
        goto failure;
    }
    SDL_FillRect( surf, 0, ckey );
    /* copy flags */
    srect.w = drect.w = shp->headers[0].actual_width;
    srect.h = drect.h = shp->headers[0].actual_height;
    height = 0;
    for ( i = 0; i < nation_count; i++ ) {
        srect.x = shp->headers[i].x1;
        srect.y = shp->headers[i].y1 + shp->offsets[i];
        drect.x = 0;
        drect.y = height;
        SDL_BlitSurface( shp->surf, &srect, surf, &drect );
        height += shp->headers[i].actual_height;
    }
    snprintf( path, MAXPATHLEN, "%s/gfx/flags/%s.bmp", dest_path, target_name );
    if ( SDL_SaveBMP( surf, path ) ) {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }
    SDL_FreeSurface( surf );
    shp_free( &shp );
    return 1;
failure:
    if ( surf ) SDL_FreeSurface( surf );
    if ( shp ) shp_free( &shp );
    return 0;
}

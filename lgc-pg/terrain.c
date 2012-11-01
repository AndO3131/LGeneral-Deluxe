/***************************************************************************
                          terrain.c -  description
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
 
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shp.h"
#include "terrain.h"
#include "misc.h"

/*
====================================================================
Externals
====================================================================
*/
extern char *source_path;
extern char *dest_path;
extern char target_name[128];
extern char *move_types[];
extern int   move_type_count;

/*
====================================================================
Weather types.
====================================================================
*/
#define NUM_WEATHER_TYPES 12
struct {
    char *id, *name; int ground; char *flags, *set_ext;
} weatherTypes[NUM_WEATHER_TYPES] = {
    { "fair",   "Fair(Dry)",     0, "none", "" }, /* f */
    { "clouds", "Overcast(Dry)", 0, "none", "" }, /* o */
    { "r",      "Raining(Dry)",  0, "no_air_attack°bad_sight", "" },
    { "s",      "Snowing(Dry)",  0, "no_air_attack°bad_sight", "" },
    { "m",      "Fair(Mud)",     1, "none", "_rain" },
    { "M",      "Overcast(Mud)", 1, "none", "_rain" },
    { "rain",   "Raining(Mud)",  1, "no_air_attack°bad_sight", "_rain" }, /* R */
    { "x",      "Snowing(Mud)",  1, "no_air_attack°bad_sight", "_rain" },
    { "i",      "Fair(Ice)",     2, "double_fuel_cost", "_snow" },
    { "I",      "Overcast(Ice)", 2, "double_fuel_cost", "_snow" },
    { "X",      "Raining(Ice)",  2, "double_fuel_cost°no_air_attack°bad_sight", "_snow" },
    { "snow",   "Snowing(Ice)",  2, "double_fuel_cost°no_air_attack°bad_sight", "_snow" } /* S */
};
/*
====================================================================
Terrain definitions.
====================================================================
*/
#define NUM_TERRAIN_TYPES 15
struct {
    char id, *name, *set_name;
    int min_entr, max_entr, max_ini; 
    char *spot_costs, *move_costs[3];
    char *flags[3];
} terrainTypes[NUM_TERRAIN_TYPES] = {
    { 'c', "Clear", "clear", 0, 5, 99, 
        "112211221122",
        { "1121A1X1", "2331A1X2", "1221A1X2" },
        { "none","none","none" } },
    { 'r', "Road", "road", 0, 5, 99,
        "112211221122",
        { "1111A1X1", "1121A1X1", "1121A1X1" },
        { "none","none","none" } },
    { '#', "Fields", "fields", 0, 5, 3, 
        "112211221122",
        { "4AA2A1X3", "AAA2A1XA", "AAA2A1X3" },
        { "none","none","none" } },
    { '~', "Rough", "rough", 1, 6, 5,
        "112211221122",
        { "2242A1X3", "34A2A1XA", "23A3A1X3" },
        { "none","none","none" } },
    { 'R', "River", "river", 0, 5, 99,
        "112211221122",
        { "AAAAA1XA", "XXXAA1XX", "2232A1X2" },
        { "river","river","none" } },
    { 'f', "Forest", "forest", 2, 7, 3,
        "222222222222",
        { "2232A1X3", "33A2A1X4", "22A2A1X4" },
        { "inf_close_def","inf_close_def","inf_close_def" } }, 
    { 'F', "Fortification", "fort", 2, 7, 3,
        "112211221122",
        { "1121A1X1", "2241A1X3", "1131A1X2" },
        { "inf_close_def","inf_close_def","inf_close_def" } },
    { 'a', "Airfield", "airfield", 2, 7, 99,
        "112211221122",
        { "1111A1X1", "1121A1X1", "1121A1X1" },
        { "supply_air°supply_ground","supply_air°supply_ground","supply_air°supply_ground"} }, 
    { 't', "Town", "town", 3, 8, 1,
        "222222222222",
        { "1111A1X1", "1121A1X1", "1121A1X1" },
        { "inf_close_def°supply_ground","inf_close_def°supply_ground","inf_close_def°supply_ground"} },
    { 'o', "Ocean", "ocean", 0, 0, 99,
        "112211221122",
        { "XXXXX11X", "XXXXX11X", "XXXXX11X" },
        { "none","none","none" } },
    { 'm', "Mountain", "mountain", 1, 6, 8,
        "222222222222",
        { "AAAAA1XA", "AAAAA1XA", "AAAAA1XA" },
        { "inf_close_def","inf_close_def","inf_close_def" } }, 
    { 's', "Swamp", "swamp", 0, 3, 4,
        "112211221122",
        { "44A2X1XA", "XXXAX1XX", "2231X1X3" },
        { "swamp","swamp","none" } },
    { 'd', "Desert", "desert", 0, 5, 99,
        "112211221122",
        { "1132A1X2", "1132A1X2", "1132A1X2" },
        { "none","none","none" } },
    { 'D',  "Rough Desert", "rough_desert", 1, 6, 99,
        "112211221122",
        { "2242A1X3", "34A2A1X3", "23A3A1X3" },
        { "none","none","none" } },
    { 'h', "Harbor", "harbor", 3, 8, 5, 
        "112211221122",
        { "1111A111", "1121A111", "1121A111" },
        { "inf_close_def°supply_ground°supply_ships",
          "inf_close_def°supply_ground°supply_ships",
          "inf_close_def°supply_ground°supply_ships"} }
};
/*
====================================================================
Terrain tile types.
====================================================================
*/
int terrain_tile_count = 237;
char tile_type[] = {
    'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 
    'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o', 'c',
    'c', 'o', 'o', 'o', 'h', 'h', 'h', 'h', 'r', 'o', 'r',
    'c', 'c', 'c', 'c', 'r', 'c', 'c', 'r', 'r', 'o', 'c',
    'c', 'c', 'c', 'r', 'r', 'r', 'o', 'o', 'R', 'R', 'R',
    'R', 'r', 'r', 'r', 'r', 'r', 'R', 'R', 'R', 'R', 'R',
    'r', 'r', 'r', 'r', 'r', 'R', 'R', 'o', 'r', 'm', 'm',
    'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 
    'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 
    'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 
    'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 
    'm', 'm', 'm', 's', 't', 't', 't', 'a', 'c', 'c', '~',
    '~', 's', 's', 'f', 'f', 'f', 'f', 'c', '~', '~', '~',
    '~', 's', 'f', 'f', 'f', 'c', 'c', 'c', 'F', 'F', 'F',
    'r', 'r', 'r', '#', 'F', 'F', 'F', 'F', 'R', 'F', 'r',
    'R', 'r', '#', 'F', 'F', 'F', 'F', 'F', 'F', 'r', 'r',
    'r', '#', 'F', 'F', 'F', 'm', 'm', 'd', 'm', 'm', 'd',
    'm', 'm', 'm', 'd', 'm', 'm', 'm', 'd', 'd', 'm', 'm',
    'm', 'm', 'D', 'D', 'D', 'D', 'D', 't', 'F', 
    /* ??? -- really mountains ??? */
    'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm', 'm',
    'm', 'm', 'm', 'm', 'm', 'm',
    /* ??? */
    'r', 'r', 'r', 'r', 'R', 'R', 'R', 'c', 'h', 'h', 'd',
    'd', 'd', 'd'
};

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Convert map tiles belonging to terrain type 'id'
from TACMAP.SHP into one bitmap called 'fname'.
====================================================================
*/
static int terrain_convert_tiles( char id, PG_Shp *shp, char *fname )
{
    Uint32 grass_pixel, snow_pixel, mud_pixel;
    int i, j, pos;
    SDL_Rect srect, drect;
    SDL_Surface *surf;
    char path[MAXPATHLEN];
    int count = 0;
    int is_road = !strcmp( fname, "road" );
    
    /* count occurence */
    for ( i = 0; i < terrain_tile_count; i++ )
        if ( tile_type[i] == id )
            count++;

    /* create surface */
    surf = SDL_CreateRGBSurface( SDL_SWSURFACE, 60 * count, 50, shp->surf->format->BitsPerPixel,
                                 shp->surf->format->Rmask, shp->surf->format->Gmask, shp->surf->format->Bmask,
                                 shp->surf->format->Amask );
    if ( surf == 0 ) {
        fprintf( stderr, "error creating surface: %s\n", SDL_GetError() );
        goto failure;
    }

    /* modified colors */
    grass_pixel = SDL_MapRGB( surf->format, 192, 192, 112 );
    snow_pixel = SDL_MapRGB( surf->format, 229, 229, 229 );
    mud_pixel = SDL_MapRGB( surf->format, 206, 176, 101 );

    /* copy pics */
    srect.w = drect.w = 60;
    srect.h = drect.h = 50;
    srect.x = drect.y = 0;
    pos = 0; count = 0;
    for ( i = 0; i < terrain_tile_count; i++ )
        if ( tile_type[i] == id ) {
			/* road tile no 2 is buggy ... */
			if (!is_road || count != 2) {
                srect.y = i * 50;
                drect.x = pos;
                SDL_BlitSurface( shp->surf, &srect, surf, &drect );
			}
			/* ... but can be repaired by mirroring tile no 7 */
			if (is_road && count == 7) {
				srect.y = i * 50;
                drect.x = 2 * 60;
				copy_surf_mirrored(shp->surf, &srect, surf, &drect);
			}
			pos += 60;
            count++;
        }

    /* default terrain */
    snprintf( path, MAXPATHLEN, "%s/gfx/terrain/%s/%s.bmp", 
                                            dest_path, target_name, fname );
    if ( SDL_SaveBMP( surf, path ) != 0 ) {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }

    /* snow terrain */
    for ( j = 0; j < surf->h; j++ )
            for ( i = 0; i < surf->w; i++ )
                if ( grass_pixel == get_pixel( surf, i, j ) )
                    set_pixel( surf, i, j, snow_pixel );
    snprintf( path, MAXPATHLEN, "%s/gfx/terrain/%s/%s_snow.bmp", 
                                            dest_path, target_name, fname );
    if ( SDL_SaveBMP( surf, path ) != 0 ) {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }
    
    /* rain terrain */
    for ( j = 0; j < surf->h; j++ )
            for ( i = 0; i < surf->w; i++ )
                if ( snow_pixel == get_pixel( surf, i, j ) )
                    set_pixel( surf, i, j, mud_pixel );
    snprintf( path, MAXPATHLEN, "%s/gfx/terrain/%s/%s_rain.bmp", 
                                            dest_path, target_name, fname );
    if ( SDL_SaveBMP( surf, path ) != 0 ) {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }
    SDL_FreeSurface( surf );
    return 1;
failure:
    if ( surf ) SDL_FreeSurface( surf );
    return 0;
}


/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Convert terrain database.
====================================================================
*/
int terrain_convert_database( void )
{
    int i, j, k;
    FILE *file = 0;
    char path[MAXPATHLEN];
    
    printf( "Terrain database...\n" );
    
    snprintf( path, MAXPATHLEN, "%s/maps/%s.tdb", dest_path, target_name );
    if ( ( file = fopen( path, "w" ) ) == 0 ) {
        fprintf( stderr, "%s: access denied\n", path );
        return 0;
    }
    
    /* weather types */
    fprintf( file, "@\n" );
    fprintf( file, "<weather\n" );
    for ( i = 0; i < NUM_WEATHER_TYPES; i++ )
        fprintf( file, "<%s\nname»%s\nflags»%s\n>\n", 
                 weatherTypes[i].id, weatherTypes[i].name, weatherTypes[i].flags );
    fprintf( file, ">\n" );
    /* domain */
    fprintf( file, "domain»pg\n" );
    /* additional graphics and sounds */
    fprintf( file, "hex_width»60\nhex_height»50\nhex_x_offset»45\nhex_y_offset»25\n" );
    fprintf( file, "fog»pg/fog.bmp\ndanger»pg/danger.bmp\ngrid»pg/grid.bmp\nframe»pg/select_frame.bmp\n" );
    fprintf( file, "crosshair»pg/crosshair.bmp\nexplosion»pg/explosion.bmp\ndamage_bar»pg/damage_bars.bmp\n" );
    fprintf( file, "explosion_sound»pg/explosion.wav\nselect_sound»pg/select.wav\n" );
    /* terrain types */
    fprintf( file, "<terrain\n" );
    for ( i = 0; i < NUM_TERRAIN_TYPES; i++ ) {
        fprintf( file, "<%c\n", terrainTypes[i].id );
        fprintf( file, "name»%s\n", terrainTypes[i].name );
        fprintf( file, "<image\n" );
        for ( j = 0; j < NUM_WEATHER_TYPES; j++ )
            fprintf( file, "%s»%s/%s%s.bmp\n", weatherTypes[j].id, 
                                    target_name, terrainTypes[i].set_name, 
                                    weatherTypes[j].set_ext );
        fprintf( file, ">\n" );
        fprintf( file, "<spot_cost\n" );
        for ( j = 0; j < NUM_WEATHER_TYPES; j++ )
            fprintf( file, "%s»%c\n", weatherTypes[j].id, terrainTypes[i].spot_costs[j] );
        fprintf( file, ">\n" );
        fprintf( file, "<move_cost\n" );
        for ( k = 0; k < move_type_count; k++ ) 
        {
            fprintf( file, "<%s\n", move_types[k * 3] );
                for ( j = 0; j < NUM_WEATHER_TYPES; j++ )
                    fprintf( file, "%s»%c\n", weatherTypes[j].id,
                             terrainTypes[i].move_costs[weatherTypes[j].ground][k] );
            fprintf( file, ">\n" );
        }
        fprintf( file, ">\n" );
        fprintf( file, "min_entr»%d\nmax_entr»%d\n",
                 terrainTypes[i].min_entr,terrainTypes[i].max_entr);
        fprintf( file, "max_init»%d\n", terrainTypes[i].max_ini );
        fprintf( file, "<flags\n" );
        for ( j = 0; j < NUM_WEATHER_TYPES; j++ )
            fprintf( file, "%s»%s\n", weatherTypes[j].id, 
                     terrainTypes[i].flags[weatherTypes[j].ground] );
        fprintf( file, ">\n" );
        fprintf( file, ">\n" );
    }
    fprintf( file, ">" );
    fclose( file );
    return 1;
}

/*
====================================================================
Convert terrain graphics
====================================================================
*/
int terrain_convert_graphics( void )
{
    int i;
    SDL_Rect srect, drect;
    PG_Shp *shp;
    char path[MAXPATHLEN], path2[MAXPATHLEN];
    SDL_Surface *surf;
    
    /* if target name differs from create pg directory as well; the standard
     * images just copy from converter are the same for all campaigns and not
     * duplicated. */
    snprintf( path, MAXPATHLEN, "%s/gfx/terrain/%s", dest_path, target_name );
    mkdir( path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
    if (strcmp(target_name, "pg")) {
        snprintf( path, MAXPATHLEN, "%s/gfx/terrain/pg", dest_path );
        mkdir( path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
    }
    
    printf( "Terrain graphics...\n" );
    
    /* explosion */
    if ( ( shp = shp_load( "EXPLODE.SHP" ) ) == 0 ) return 0;
    surf = SDL_CreateRGBSurface( SDL_SWSURFACE, 60 * 5, 50, shp->surf->format->BitsPerPixel,
                                 shp->surf->format->Rmask, shp->surf->format->Gmask, shp->surf->format->Bmask,
                                 shp->surf->format->Amask );
    if ( surf == 0 ) {
        fprintf( stderr, "error creating surface: %s\n", SDL_GetError() );
        goto failure;
    }
    srect.w = drect.w = 60;
    srect.h = drect.h = 50;
    srect.x = drect.y = 0;
    for ( i = 0; i < 5; i++ ) {
        srect.y = i * srect.h;
        drect.x = i * srect.w;
        SDL_BlitSurface( shp->surf, &srect, surf, &drect );
    }
    snprintf( path, MAXPATHLEN, "%s/gfx/terrain/pg/explosion.bmp", dest_path );
    if ( SDL_SaveBMP( surf, path ) != 0 ) {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }
    SDL_FreeSurface( surf );
    shp_free(&shp);
    /* fog */
    snprintf( path, MAXPATHLEN, "%s/convdata/fog.bmp", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/gfx/terrain/pg/fog.bmp", dest_path );
    copy( path, path2 );
    /* danger */
    snprintf( path, MAXPATHLEN, "%s/convdata/danger.bmp", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/gfx/terrain/pg/danger.bmp", dest_path );
    copy( path, path2 );
    /* grid */
    snprintf( path, MAXPATHLEN, "%s/convdata/grid.bmp", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/gfx/terrain/pg/grid.bmp", dest_path );
    copy( path, path2 );
    /* select frame */
    snprintf( path, MAXPATHLEN, "%s/convdata/select_frame.bmp", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/gfx/terrain/pg/select_frame.bmp", dest_path );
    copy( path, path2 );
    /* crosshair */
    snprintf( path, MAXPATHLEN, "%s/convdata/crosshair.bmp", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/gfx/terrain/pg/crosshair.bmp", dest_path );
    copy( path, path2 );
    /* damage bars */
    snprintf( path, MAXPATHLEN, "%s/convdata/damage_bars.bmp", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/gfx/terrain/pg/damage_bars.bmp", dest_path );
    copy( path, path2 );
    /* terrain graphics */
    if ( ( shp = shp_load( "TACMAP.SHP" ) ) == 0 ) goto failure;
    if ( !terrain_convert_tiles( 'c', shp, "clear" ) ) goto failure;
    if ( !terrain_convert_tiles( 'r', shp, "road" ) ) goto failure;
    if ( !terrain_convert_tiles( '#', shp, "fields" ) ) goto failure;
    if ( !terrain_convert_tiles( '~', shp, "rough" ) ) goto failure;
    if ( !terrain_convert_tiles( 'R', shp, "river" ) ) goto failure;
    if ( !terrain_convert_tiles( 'f', shp, "forest" ) ) goto failure;
    if ( !terrain_convert_tiles( 'F', shp, "fort" ) ) goto failure;
    if ( !terrain_convert_tiles( 'a', shp, "airfield" ) ) goto failure;
    if ( !terrain_convert_tiles( 't', shp, "town" ) ) goto failure;
    if ( !terrain_convert_tiles( 'o', shp, "ocean" ) ) goto failure;
    if ( !terrain_convert_tiles( 'm', shp, "mountain" ) ) goto failure;
    if ( !terrain_convert_tiles( 's', shp, "swamp" ) ) goto failure;
    if ( !terrain_convert_tiles( 'd', shp, "desert" ) ) goto failure;
    if ( !terrain_convert_tiles( 'D', shp, "rough_desert" ) ) goto failure;
    if ( !terrain_convert_tiles( 'h', shp, "harbor" ) ) goto failure;
    shp_free(&shp);
    return 1;
failure:
    if ( surf ) SDL_FreeSurface( surf );
    if ( shp ) shp_free( &shp );
    return 0;
}

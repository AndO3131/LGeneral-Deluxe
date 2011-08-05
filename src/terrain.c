/***************************************************************************
                          terrain.c  -  description
                             -------------------
    begin                : Wed Mar 17 2002
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
#include "unit_lib.h"
#include "terrain.h"
#include "localize.h"

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern Mov_Type *mov_types;
extern int mov_type_count;
extern int log_x, log_y;
extern Font *log_font;
extern Config config;

/*
====================================================================
Flag conversion table
====================================================================
*/
StrToFlag fct_terrain[] = {
    { "no_air_attack", NO_AIR_ATTACK },         
    { "double_fuel_cost", DOUBLE_FUEL_COST },   
    { "supply_air", SUPPLY_AIR },               
    { "supply_ground", SUPPLY_GROUND },         
    { "supply_ships", SUPPLY_SHIPS },           
    { "river", RIVER },
    { "no_spotting", NO_SPOTTING },             
    { "inf_close_def", INF_CLOSE_DEF },         
    { "cut_strength", CUT_STRENGTH },
    { "bad_sight", BAD_SIGHT },
    { "swamp", SWAMP },
    { "X", 0}    
};

/*
====================================================================
Geometry of a hex tile
====================================================================
*/
int hex_w, hex_h;
int hex_x_offset, hex_y_offset;

/*
====================================================================
Terrain types & co
====================================================================
*/
Terrain_Type *terrain_types = 0;
int terrain_type_count = 0;
Weather_Type *weather_types = 0;
int weather_type_count = 0;
Terrain_Icons *terrain_icons = 0;

/*
====================================================================
Load terrain types, weather information and hex tile icons.
====================================================================
*/
int terrain_load( char *fname )
{
    int i, j, k;
    PData *pd, *sub, *subsub, *subsubsub;
    List *entries, *flags;
    char path[512];
    char *flag, *str;
    char *domain = 0;
    int count;
    /* log info */
    int  log_dot_limit = 40; /* maximum of dots */
    char log_str[128];
    sprintf( path, "%s/maps/%s", get_gamedir(), fname );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    /* get weather */
    if ( !parser_get_entries( pd, "weather", &entries ) ) goto parser_failure;
    weather_type_count = entries->count;
    weather_types = calloc( weather_type_count, sizeof( Weather_Type ) );
    list_reset( entries ); i = 0;
    while ( ( sub = list_next( entries ) ) ) {
        weather_types[i].id = strdup( sub->name );
        if ( !parser_get_localized_string( sub, "name", domain, &weather_types[i].name ) ) goto parser_failure;
        if ( !parser_get_values( sub, "flags", &flags ) ) goto parser_failure;
        list_reset( flags );
        while ( ( flag = list_next( flags ) ) )
            weather_types[i].flags |= check_flag( flag, fct_terrain );
        i++;
    }
    /* hex tile geometry */
    if ( !parser_get_int( pd, "hex_width", &hex_w ) ) goto parser_failure;
    if ( !parser_get_int( pd, "hex_height", &hex_h ) ) goto parser_failure;
    if ( !parser_get_int( pd, "hex_x_offset", &hex_x_offset ) ) goto parser_failure;
    if ( !parser_get_int( pd, "hex_y_offset", &hex_y_offset ) ) goto parser_failure;
    /* terrain icons */
    terrain_icons = calloc( 1, sizeof( Terrain_Icons ) );
    if ( !parser_get_value( pd, "fog", &str, 0 ) ) goto parser_failure;
    sprintf( path, "terrain/%s", str );
    if ( ( terrain_icons->fog = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
    if ( !parser_get_value( pd, "danger", &str, 0 ) ) goto parser_failure;
    sprintf( path, "terrain/%s", str );
    if ( ( terrain_icons->danger = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
    if ( !parser_get_value( pd, "grid", &str, 0 ) ) goto parser_failure;
    sprintf( path, "terrain/%s", str );
    if ( ( terrain_icons->grid = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
    if ( !parser_get_value( pd, "frame", &str, 0 ) ) goto parser_failure;
    sprintf( path, "terrain/%s", str );
    if ( ( terrain_icons->select = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure;
    if ( !parser_get_value( pd, "crosshair", &str, 0 ) ) goto parser_failure;
    sprintf( path, "terrain/%s", str );
    if ( ( terrain_icons->cross = anim_create( load_surf( path, SDL_SWSURFACE ), 1000/config.anim_speed, hex_w, hex_h, sdl.screen, 0, 0 ) ) == 0 )
        goto failure;
    anim_hide( terrain_icons->cross, 1 );
    if ( !parser_get_value( pd, "explosion", &str, 0 ) ) goto parser_failure;
    sprintf( path, "terrain/%s", str );
    if ( ( terrain_icons->expl1 = anim_create( load_surf( path, SDL_SWSURFACE ), 50/config.anim_speed, hex_w, hex_h, sdl.screen, 0, 0 ) ) == 0 )
        goto failure;
    anim_hide( terrain_icons->expl1, 1 );
    if ( ( terrain_icons->expl2 = anim_create( load_surf( path, SDL_SWSURFACE ), 50/config.anim_speed, hex_w, hex_h, sdl.screen, 0, 0 ) ) == 0 )
        goto failure;
    anim_hide( terrain_icons->expl2, 1 );
    /* terrain sounds */
#ifdef WITH_SOUND    
    if ( parser_get_value( pd, "explosion_sound", &str, 0 ) )
        terrain_icons->wav_expl = wav_load( str, 2 );
    if ( parser_get_value( pd, "select_sound", &str, 0 ) )
        terrain_icons->wav_select = wav_load( str, 1 );
#endif
    /* terrain types */
    if ( !parser_get_entries( pd, "terrain", &entries ) ) goto parser_failure;
    terrain_type_count = entries->count;
    terrain_types = calloc( terrain_type_count, sizeof( Terrain_Type ) );
    list_reset( entries ); i = 0;
    while ( ( sub = list_next( entries ) ) ) {
        /* id */
        terrain_types[i].id = sub->name[0];
        /* name */
        if ( !parser_get_localized_string( sub, "name", domain, &terrain_types[i].name ) ) goto parser_failure;
        /* each weather type got its own image -- if it's named 'default' we
           point towards the image of weather_type 0 */
        terrain_types[i].images = calloc( weather_type_count, sizeof( SDL_Surface* ) );
        for ( j = 0; j < weather_type_count; j++ ) {
            sprintf( path, "image/%s", weather_types[j].id );
            if ( !parser_get_value( sub, path, &str, 0 ) ) goto parser_failure;
            if ( STRCMP( "default", str ) && j > 0 ) {
                /* just a pointer */
                terrain_types[i].images[j] = terrain_types[i].images[0];
            }
            else {
                sprintf( path, "terrain/%s", str );
                if ( ( terrain_types[i].images[j] = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto parser_failure;
                SDL_SetColorKey( terrain_types[i].images[j], SDL_SRCCOLORKEY, 
                                 get_pixel( terrain_types[i].images[j], 0, 0 ) );
            }
        }
        /* fog image */
        terrain_types[i].images_fogged = calloc( weather_type_count, sizeof( SDL_Surface* ) );
        for ( j = 0; j < weather_type_count; j++ ) {
            if ( terrain_types[i].images[j] == terrain_types[i].images[0] && j > 0 ) {
                /* just a pointer */
                terrain_types[i].images_fogged[j] = terrain_types[i].images_fogged[0];
            }
            else {
                terrain_types[i].images_fogged[j] = create_surf( terrain_types[i].images[j]->w, terrain_types[i].images[j]->h, SDL_SWSURFACE );
                FULL_DEST( terrain_types[i].images_fogged[j]  );
                FULL_SOURCE( terrain_types[i].images[j] );
                blit_surf();
                count =  terrain_types[i].images[j]->w /  hex_w;
                for ( k = 0; k < count; k++ ) {
                    DEST( terrain_types[i].images_fogged[j], k * hex_w, 0,  hex_w, hex_h );
                    SOURCE(  terrain_icons->fog, 0, 0 );
                    alpha_blit_surf( FOG_ALPHA );
                }
                SDL_SetColorKey( terrain_types[i].images_fogged[j], SDL_SRCCOLORKEY, get_pixel( terrain_types[i].images[j], 0, 0 ) );
            }
        }
        /* spot cost */
        terrain_types[i].spt = calloc( weather_type_count, sizeof( int ) );
        if ( !parser_get_pdata( sub, "spot_cost",  &subsub ) ) goto parser_failure;
        for ( j = 0; j < weather_type_count; j++ )
            if ( !parser_get_int( subsub, weather_types[j].id, &terrain_types[i].spt[j] ) ) goto parser_failure;
        /* mov cost */
        terrain_types[i].mov = calloc( mov_type_count * weather_type_count, sizeof( int ) );
        if ( !parser_get_pdata( sub, "move_cost",  &subsub ) ) goto parser_failure;
        for ( k = 0; k < mov_type_count; k++ ) {
            if ( !parser_get_pdata( subsub, mov_types[k].id,  &subsubsub ) ) goto parser_failure;
            for ( j = 0; j < weather_type_count; j++ ) {
                if ( !parser_get_value( subsubsub, weather_types[j].id, &str, 0 ) ) goto parser_failure;
                if ( str[0] == 'X' )
                    terrain_types[i].mov[j + k * weather_type_count] = 0; /* impassable */
                else
                    if ( str[0] == 'A' )
                        terrain_types[i].mov[j + k * weather_type_count] = -1; /* costs all */
                    else
                        terrain_types[i].mov[j + k * weather_type_count] = atoi( str ); /* normal cost */
            }
        }
        /* entrenchment */
        if ( !parser_get_int( sub, "min_entr",  &terrain_types[i].min_entr ) ) goto parser_failure;
        if ( !parser_get_int( sub, "max_entr",  &terrain_types[i].max_entr ) ) goto parser_failure;
        /* initiative modification */
        if ( !parser_get_int( sub, "max_init",  &terrain_types[i].max_ini ) ) goto parser_failure;
        /* flags */
        terrain_types[i].flags = calloc( weather_type_count, sizeof( int ) );
        if ( !parser_get_pdata( sub, "flags",  &subsub ) ) goto parser_failure;
        for ( j = 0; j < weather_type_count; j++ ) {
            if ( !parser_get_values( subsub, weather_types[j].id, &flags ) ) goto parser_failure;
            list_reset( flags );
            while ( ( flag = list_next( flags ) ) )
                terrain_types[i].flags[j] |= check_flag( flag, fct_terrain );
        }
        /* next terrain */
        i++;
        /* LOG */
        strcpy( log_str, "  [                                        ]" );
        for ( k = 0; k < i * log_dot_limit / entries->count; k++ )
            log_str[3 + k] = '*';
        write_text( log_font, sdl.screen, log_x, log_y, log_str, 255 );
        SDL_UpdateRect( sdl.screen, log_font->last_x, log_font->last_y, log_font->last_width, log_font->last_height );
    }
    parser_free( &pd );
    /* LOG */
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    free(domain);
    return 1;
parser_failure:        
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    terrain_delete();
    if ( pd ) parser_free( &pd );
    free(domain);
    printf(tr("If data seems to be missing, please re-run the converter lgc-pg.\n"));
    return 0;
}

/*
====================================================================
Delete terrain types & co
====================================================================
*/
void terrain_delete( void )
{
    int i, j;
    /* terrain types */
    if ( terrain_types ) {
        for ( i = 0; i < terrain_type_count; i++ ) {
            if ( terrain_types[i].images ) {
                for ( j = weather_type_count - 1; j >= 0; j-- ) 
                    if ( terrain_types[i].images[j] ) {
                        if ( terrain_types[i].images[j] == terrain_types[i].images[0] )
                            if ( j > 0 )
                                continue; /* only a pointer */
                        SDL_FreeSurface( terrain_types[i].images[j] );
                    }
                free( terrain_types[i].images );
            }
            if ( terrain_types[i].images_fogged ) {
                for ( j = weather_type_count - 1; j >= 0; j-- )
                    if ( terrain_types[i].images_fogged[j] ) {
                        if ( terrain_types[i].images_fogged[j] == terrain_types[i].images_fogged[0] )
                            if ( j > 0 )
                                continue; /* only a pointer */
                        SDL_FreeSurface( terrain_types[i].images_fogged[j] );
                    }
                free( terrain_types[i].images_fogged );
            }
            if ( terrain_types[i].flags ) free( terrain_types[i].flags );
            if ( terrain_types[i].mov ) free( terrain_types[i].mov );
            if ( terrain_types[i].spt ) free( terrain_types[i].spt );
            if ( terrain_types[i].name ) free( terrain_types[i].name );
        }
        free( terrain_types ); terrain_types = 0;
        terrain_type_count = 0;
    }
    /* weather */
    if ( weather_types ) {
        for ( i = 0; i < weather_type_count; i++ ) {
            if ( weather_types[i].id ) free( weather_types[i].id );
            if ( weather_types[i].name ) free( weather_types[i].name );
        }
        free( weather_types ); weather_types = 0;
        weather_type_count = 0;
    }
    /* terrain icons */
    if ( terrain_icons ) {
        if ( terrain_icons->fog ) SDL_FreeSurface( terrain_icons->fog );
        if ( terrain_icons->grid ) SDL_FreeSurface( terrain_icons->grid );
        if ( terrain_icons->danger ) SDL_FreeSurface( terrain_icons->danger );
        if ( terrain_icons->select ) SDL_FreeSurface( terrain_icons->select );
        anim_delete( &terrain_icons->cross );
        anim_delete( &terrain_icons->expl1 );
        anim_delete( &terrain_icons->expl2 );
#ifdef WITH_SOUND
        if ( terrain_icons->wav_expl ) wav_free( terrain_icons->wav_expl );
        if ( terrain_icons->wav_select ) wav_free( terrain_icons->wav_select );
#endif
        free( terrain_icons ); terrain_icons = 0;
    }
}

/*
====================================================================
Get the movement cost for a terrain type by passing movement
type id and weather id in addition.
Return -1 if all movement points are consumed
Return 0  if movement is impossible
Return cost else.
====================================================================
*/
int terrain_get_mov( Terrain_Type *type, int mov_type, int weather )
{
    return type->mov[mov_type * weather_type_count + weather];
}

/***************************************************************************
                          strat_map.c  -  description
                             -------------------
    begin                : Fri Apr 5 2002
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
 
#include <dirent.h>
#include "lgeneral.h"
#include "windows.h"
#include "unit.h"
#include "purchase_dlg.h"
#include "gui.h"
#include "nation.h"
#include "player.h"
#include "date.h"
#include "map.h"
#include "scenario.h"
#include "engine.h"
#include "strat_map.h"

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern Nation *nations;
extern int nation_count;
extern int nation_flag_width, nation_flag_height;
extern int map_w, map_h;
extern int hex_w, hex_h;
extern int hex_x_offset, hex_y_offset, terrain_columns, terrain_rows;
extern int terrain_type_count;
extern Terrain_Type *terrain_types;
extern Mask_Tile **mask;
extern Map_Tile **map;
extern int air_mode;
extern Player *cur_player;
extern int cur_weather;
extern Weather_Type *weather_types;
extern Terrain_Images *terrain_images;

/*
====================================================================
Strategic map data.
====================================================================
*/
static int screen_x, screen_y; /* position on screen */
static int width, height; /* size of map picture */
static int sm_x_offset, sm_y_offset; /* offset for first strat map tile 0,0 in strat_map */
static int strat_tile_width;
static int strat_tile_height; /* size of a shrinked map tile */
static int strat_tile_x_offset;
static int strat_tile_y_offset; /* offsets that will be add to one's position for the next tile */
static SDL_Surface *strat_map = 0; /* picture assembled by create_strat_map() */
static SDL_Surface *fog_layer = 0; /* used to buffer fog before addind to strat map */
static SDL_Surface *unit_layer = 0; /* layer with unit flags indicating there position */
static SDL_Surface *flag_layer = 0; /* layer with all flags; white frame means normal; gold frame means objective */
static int tile_count; /* equals map::def::tile_count */
static SDL_Surface **strat_tile_pic = 0; /* shrinked normal map tiles; created in strat_map_create() */
static SDL_Surface *strat_flags = 0; /* resized flag picture containing the nation flags */
static int strat_flag_width, strat_flag_height; /* size of resized strat flag */
static SDL_Surface *blink_dot = 0; /* white and black dot that blinks indicating which
                                      unit wasn't moved yet */
static List *dots = 0; /* list of dot positions that need blinking */
static int blink_on = 0; /* switch used for blinking */

/*
====================================================================
LOCALS
====================================================================
*/

/*
====================================================================
Get size of strategic map depending on scale where
scale == 1: normal size
scale == 2: size / 2
scale == 3: size / 3
...
====================================================================
*/
static int scaled_strat_map_width( int scale )
{
    return ( ( map_w - 1 ) * hex_x_offset / scale );
}
static int scaled_strat_map_height( int scale )
{
    return ( ( map_h - 1 ) * hex_h / scale );
}

/*
====================================================================
Updates the picture offset for the strategic map in all map tiles.
====================================================================
*/
static void update_strat_image_offset()
{
    int x, y;
    for ( x = 0; x < map_w; x++ )
        for ( y = 0;  y < map_h; y++ )
            map_tile( x, y )->strat_image_offset = strat_tile_width
                                                 * map_tile( x, y )->image_offset_x / hex_w +
                                                 strat_tile_width * terrain_columns
                                                 * map_tile( x, y )->image_offset_y / hex_h;
}

/*
====================================================================
PUBLIC
====================================================================
*/

/*
====================================================================
Is called after scenario was loaded in init_engine() and creates
the strategic map tile pictures, flags and the strat_map itself +
unit_layer
====================================================================
*/
void strat_map_create()
{
    Uint32 ckey;
    int i, j, j2, x, y;
    int strat_tile_count;
    Uint32 pixel;
    int hori_scale, vert_scale;
    int scale;
    /* scale normal geometry so it fits the screen */
    /* try horizontal */
    hori_scale = 1;
    while( scaled_strat_map_width( hori_scale ) > sdl.screen->w ) hori_scale++;
    vert_scale = 1;
    while( scaled_strat_map_height( vert_scale ) > sdl.screen->h )
        vert_scale++;
    /* use greater scale */
    if ( hori_scale > vert_scale )
        scale = hori_scale;
    else
        scale = vert_scale;
    /* get strat map tile size */
    strat_tile_width = hex_w / scale;
    strat_tile_height = hex_h / scale;
    strat_tile_x_offset = hex_x_offset / scale;
    strat_tile_y_offset = hex_y_offset / scale;
    /* create strat tile array */
    tile_count = terrain_type_count / 4;
    strat_tile_pic = calloc( tile_count, sizeof( SDL_Surface* ) );
    /* create strat tiles */
    for ( i = 0; i < tile_count; i++ ) {
        /* how many tiles are rowed? */
        strat_tile_count = terrain_columns * terrain_rows;
        /* create strat_pic */
        strat_tile_pic[i] = create_surf( strat_tile_count * strat_tile_width,
                                         strat_tile_height,
                                         SDL_SWSURFACE );
        /* clear to color key */
        ckey = get_pixel( terrain_images->images[0], 0, 0 );
        FULL_DEST( strat_tile_pic[i] );
        fill_surf( ckey );
        SDL_SetColorKey( strat_tile_pic[i], SDL_SRCCOLORKEY, ckey );
        /* copy pixels from pic to strat_pic if strat_fog is none transparent */
        for ( j = 0; j < terrain_columns; j++ )
            for ( j2 = 0; j2 < terrain_rows; j2++ )
                for ( x = 0; x < strat_tile_width; x++ )
                    for ( y = 0; y < strat_tile_height; y++ ) {
                        /* we have the pixel in strat_pic by x + j * strat_fog_pic->w,y */
                        /* no we need the aquivalent pixel in tiles[i]->pic to copy it */
                        pixel = get_pixel( terrain_images->images[i],
                                           j * hex_w +
                                           scale * x,
                                           j2 * hex_h + 
                                           scale * y);
                        set_pixel( strat_tile_pic[i], j * strat_tile_width + strat_tile_width * terrain_columns * j2 + x,
                                   y, pixel );
                    }
    }
    /* update strat picture offset in all map tiles */
    update_strat_image_offset();
    /* resized nation flags */
    strat_flag_width = strat_tile_width - 2;
    strat_flag_height = strat_tile_height - 2;
    strat_flags = create_surf( strat_flag_width, strat_flag_height * nation_count, SDL_SWSURFACE );
    /* scale down */
    for ( i = 0; i < nation_count; i++ )
        for ( x = 0; x < strat_flag_width; x++ )
            for ( y = 0; y < strat_flag_height; y++ ) {
                pixel = nation_get_flag_pixel( &nations[i], 
                                               nation_flag_width * x / strat_flag_width,
                                               nation_flag_height * y / strat_flag_height );
                set_pixel( strat_flags, x, i * strat_flag_height + y, pixel );
            }
    /* measurements */
    width = ( map_w - 1 ) * strat_tile_x_offset;
    sm_x_offset = -strat_tile_width + strat_tile_x_offset;
    height = ( map_h - 1 ) * strat_tile_height;
    sm_y_offset = -strat_tile_height / 2;
    screen_x = ( sdl.screen->w - width ) / 2;
    screen_y = ( sdl.screen->h - height ) / 2;
    /* build white rectangle */
    blink_dot = create_surf( 2, 2, SDL_SWSURFACE );
    set_pixel( blink_dot, 0, 0, SDL_MapRGB( blink_dot->format, 255, 255, 255 ) );
    set_pixel( blink_dot, 1, 0, SDL_MapRGB( blink_dot->format, 16, 16, 16 ) );
    set_pixel( blink_dot, 1, 1, SDL_MapRGB( blink_dot->format, 255, 255, 255 ) );
    set_pixel( blink_dot, 0, 1, SDL_MapRGB( blink_dot->format, 16, 16, 16 ) );
    /* position list */
    dots = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
}

/*
====================================================================
Clean up stuff that was allocated by strat_map_create()
====================================================================
*/
void strat_map_delete()
{
    int i;
    if ( strat_tile_pic ) {
        for ( i = 0; i < tile_count; i++ )
            free_surf( &strat_tile_pic[i] );
        free( strat_tile_pic );
        strat_tile_pic = 0;
    }
    free_surf( &strat_flags );
    free_surf( &strat_map );
    free_surf( &fog_layer );
    free_surf( &unit_layer );
    free_surf( &flag_layer );
    free_surf( &blink_dot );
    if ( dots ) {
        list_delete( dots );
        dots = 0;
    }
}

/*
====================================================================
Update the bitmap containing the strategic map.
====================================================================
*/
void strat_map_update_terrain_layer()
{
    int i, j;
    /* reallocate */
    free_surf( &strat_map );
    free_surf( &fog_layer );
    strat_map = create_surf( width, height, SDL_SWSURFACE );
    SDL_FillRect( strat_map, 0, 0x0 );
    fog_layer = create_surf( width, height, SDL_SWSURFACE );
    SDL_FillRect( fog_layer, 0, 0x0 );
    /* first gather all fogged tiles without alpha */
    for ( j = 0; j < map_h; j++ )
        for ( i = 0; i < map_w; i++ )
            if ( mask[i][j].fog ) {
                DEST( fog_layer,
                      sm_x_offset + i * strat_tile_x_offset,
                      sm_y_offset + j * strat_tile_height + ( i & 1 ) * strat_tile_y_offset,
                      strat_tile_width, strat_tile_height );
                SOURCE( strat_tile_pic
                        [ground_conditions_get_index( weather_types[cur_weather].ground_conditions )],
                        map_tile( i, j )->strat_image_offset, 0 );
                blit_surf();
            }
    /* now add this fog map with transparency to strat_map */
    FULL_DEST( strat_map ); FULL_SOURCE( fog_layer );
    alpha_blit_surf( 255 - FOG_ALPHA );
    /* now add unfogged map tiles */
    for ( j = 0; j < map_h; j++ )
        for ( i = 0; i < map_w; i++ )
            if ( !mask[i][j].fog ) {
                DEST( strat_map,
                      sm_x_offset + i * strat_tile_x_offset,
                      sm_y_offset + j * strat_tile_height + ( i & 1 ) * strat_tile_y_offset,
                      strat_tile_width, strat_tile_height );
                SOURCE( strat_tile_pic[ground_conditions_get_index( weather_types[cur_weather].ground_conditions )],
                        map_tile( i, j )->strat_image_offset, 0 );
                blit_surf();
            }
}
typedef struct {
    int x, y;
    Nation *nation;
} DotPos;
void strat_map_update_unit_layer()
{
    int i, j;
    Unit *unit;
    DotPos *dotpos = 0;
    list_clear( dots );
    free_surf( &unit_layer );
    free_surf( &flag_layer );
    unit_layer = create_surf( width, height, SDL_SWSURFACE );
    SDL_FillRect( unit_layer, 0, 0x0 );
    flag_layer = create_surf( width, height, SDL_SWSURFACE );
    SDL_FillRect( flag_layer, 0, 0x0 );
    /* draw tiles; fogged tiles are copied transparent with the inverse of FOG_ALPHA */
    for ( j = 0; j < map_h; j++ )
        for ( i = 0; i < map_w; i++ ) {
            if ( map_tile( i, j )->nation != 0 ) {
                /* draw frame and flag only if objective */
                if ( map_tile( i, j )->obj ) {
                    DEST( flag_layer,
                          sm_x_offset + i * strat_tile_x_offset,
                          sm_y_offset + j * strat_tile_height + ( i & 1 ) * strat_tile_y_offset,
                          strat_tile_width, strat_tile_height );
                    fill_surf( 0xffff00 );
                    /* add flag */
                    DEST( flag_layer,
                          sm_x_offset + i * strat_tile_x_offset + 1,
                          sm_y_offset + j * strat_tile_height + ( i & 1 ) * strat_tile_y_offset + 1,
                          strat_flag_width, strat_flag_height );
                    SOURCE( strat_flags, 0, nation_get_index( map_tile( i, j )->nation ) * strat_flag_height );
                    blit_surf();
                }
            }
            unit = 0;
            if ( air_mode && map[i][j].a_unit )
                unit = map_tile( i, j )->a_unit;
            if ( !air_mode && map[i][j].g_unit )
                unit = map_tile( i, j )->g_unit;
            if ( map_mask_tile( i, j )->fog ) unit = 0;
            if ( unit ) {
                /* nation flag has a small frame of one pixel */
                DEST( unit_layer,
                      sm_x_offset + i * strat_tile_x_offset + 1,
                      sm_y_offset + j * strat_tile_height + ( i & 1 ) * strat_tile_y_offset + 1,
                      strat_flag_width, strat_flag_height );
                SOURCE( strat_flags, 0, nation_get_index( unit->nation ) * strat_flag_height );
                blit_surf();
                if ( cur_player )
                if ( cur_player == unit->player )
                if ( unit->cur_mov > 0 ) {
                    /* unit needs a blinking spot */
                    dotpos = calloc( 1, sizeof( DotPos ) );
                    dotpos->x = sm_x_offset + i * strat_tile_x_offset + 1/* +
                                ( strat_flag_width - blink_dot->w ) / 2*/;
                    dotpos->y = sm_y_offset + j * strat_tile_height + ( i & 1 ) * strat_tile_y_offset + 1/* +
                                ( strat_flag_height - blink_dot->h ) / 2*/;
                    dotpos->nation = unit->nation;
                    list_add( dots, dotpos );
                }
            }
        }
}
void strat_map_draw()
{
    SDL_FillRect( sdl.screen, 0, 0x0 );
    DEST( sdl.screen, screen_x, screen_y, width, height );
    SOURCE( strat_map, 0, 0 );
    blit_surf();
    DEST( sdl.screen, screen_x, screen_y, width, height );
    SOURCE( flag_layer, 0, 0 );
    blit_surf();
    DEST( sdl.screen, screen_x, screen_y, width, height );
    SOURCE( unit_layer, 0, 0 );
    blit_surf();
}

/*
====================================================================
Handle motion input. If mouse pointer is on a map tile x,y and true
is returned.
====================================================================
*/
int strat_map_get_pos( int cx, int cy, int *map_x, int *map_y )
{
    int x = 0, y = 0;
    int tile_x_offset, tile_y_offset;
    int tile_x, tile_y;
    Uint32 ckey;
    /* check if outside of map */
    if ( cx < screen_x || cy < screen_y )
        return 0;
    if ( cx >= screen_x + width || cy >= screen_y + height )
        return 0;
    /* clip mouse pos to mask */
    cx -= screen_x;
    cy -= screen_y;
    /* get the map offset in screen from mouse position */
    x = ( cx - sm_x_offset ) / strat_tile_x_offset;
    if ( x & 1 )
        y = ( cy - ( strat_tile_height >> 1 ) - sm_y_offset ) / strat_tile_height;
    else
        y = ( cy - sm_y_offset ) / strat_tile_height;
    /* get the starting position of this strat map tile at x,y in surface strat_map */
    tile_x_offset = sm_x_offset;
    tile_y_offset = sm_y_offset;
    tile_x_offset += x * strat_tile_x_offset;
    tile_y_offset += y * strat_tile_height;
    if ( x & 1 )
        tile_y_offset += strat_tile_height >> 1;
    /* now test where exactly the mouse pointer is in this tile using
       strat_tile_pic[0] */
    tile_x = cx - tile_x_offset;
    tile_y = cy - tile_y_offset;
    ckey = get_pixel( strat_tile_pic[0], 0, 0 );
    if ( get_pixel( strat_tile_pic[0], tile_x, tile_y ) == ckey ) {
        if ( tile_y < strat_tile_y_offset && !( x & 1 ) ) y--;
        if ( tile_y >= strat_tile_y_offset && (x & 1 ) ) y++;
        x--;
    }
    /* assign */
    *map_x = x;
    *map_y = y;
    return 1;
}

/*
====================================================================
Blink the dots that show unmoved units.
====================================================================
*/
void strat_map_blink()
{
    DotPos *pos;
    blink_on = !blink_on;
    list_reset( dots );
    while ( ( pos = list_next( dots ) ) ) {
        if ( blink_on ) {
            DEST( sdl.screen, screen_x + pos->x, screen_y + pos->y, strat_flag_width, strat_flag_height );
            SOURCE( strat_flags, 0, nation_get_index( pos->nation ) * strat_flag_height );
            blit_surf();
        }
        else {
            DEST( sdl.screen, screen_x + pos->x, screen_y + pos->y, strat_flag_width, strat_flag_height );
            SOURCE( strat_map, pos->x, pos->y );
            blit_surf();
            SOURCE( unit_layer, pos->x, pos->y );
            alpha_blit_surf( 96 );
        }
/*        if ( blink_on ) {
            DEST( sdl.screen, screen_x + pos->x, screen_y + pos->y, blink_dot->w, blink_dot->h );
            SOURCE( blink_dot, 0, 0 );
            blit_surf();
        }
        else {
            DEST( sdl.screen, screen_x + pos->x, screen_y + pos->y, blink_dot->w, blink_dot->h );
            SOURCE( unit_layer, pos->x, pos->y );
            blit_surf();
        }*/
    }
    add_refresh_region( screen_x, screen_y, width, height );
}

/*
 * pg.c
 *
 *  Created on: 2011-09-04
 *      Author: wino
 */
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

#include <stdio.h>
#include "fpge.h"
#include "pg.h"
#include "pgf.h"
#include "terrain.h"
#include "map.h"
#include "localize.h"
#include "sdl.h"
#include "misc.h"
#include "file.h"
#include "lgeneral.h"

extern Sdl sdl;
extern Font *log_font;
extern Config config;
extern int map_w, map_h;
extern Map_Tile **map;
extern Mask_Tile **mask;
extern int log_x, log_y;       /* position where to draw log info */
extern Terrain_Type *terrain_types;
extern int terrain_type_count;
extern int hex_w, hex_h, terrain_columns;

char stm_fname[16];

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
    'r', 'r', 'r', '@', 'F', 'F', 'F', 'F', 'R', 'F', 'r',
    'R', 'r', '@', 'F', 'F', 'F', 'F', 'F', 'F', 'r', 'r',
    'r', '@', 'F', 'F', 'F', 'm', 'm', 'd', 'm', 'm', 'd',
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
Read map tile name with that id to buf
====================================================================
*/
static void tile_get_name( FILE *file, int id, char *buf )
{
    fseek( file, 2 + id * 20, SEEK_SET );
    if ( feof( file ) ) 
        sprintf( buf, "none" );
    else
{
        fread( buf, 20, 1, file );
//fprintf(stderr, "%s\n", buf);
}
}

int parse_set_file(FILE *inf, int offset)
{
    FILE *name_file;
    char path[MAX_PATH];
    int x,y,c=0,i;
    /* log info */
    char log_str[128], name_buf[24];
 
    //get the map size
    fseek(inf, offset+MAP_X_ADDR, SEEK_SET);
    fread(&map_w, 2, 1, inf);
    fseek(inf, offset+MAP_Y_ADDR, SEEK_SET);
    fread(&map_h, 2, 1, inf);
    ++map_w;
    ++map_h; //PG sets this to one less than size i.e. last index
    if ( !terrain_load( "terrain" ) )
    {
        map_delete();
        return 0;
    }
    search_file_name_exact( path, "Scenario/mapnames.str", config.mod_name );
    if ( (name_file = fopen( path, "r" )) == NULL )
    {
        return 0;
    }
    /* allocate map memory */
    map = calloc( map_w, sizeof( Map_Tile* ) );
    for ( i = 0; i < map_w; i++ )
        map[i] = calloc( map_h, sizeof( Map_Tile ) );
    mask = calloc( map_w, sizeof( Mask_Tile* ) );
    for ( i = 0; i < map_w; i++ )
        mask[i] = calloc( map_h, sizeof( Mask_Tile ) );
    //get STM
    fseek(inf, offset+MAP_STM, SEEK_SET);
    fread(stm_fname, 14, 1, inf);
    stm_fname[14]=0;
    //stm_number=atoi(stm);
    //printf("nn=%d\n",nn);
    //get the tiles numbers
    sprintf( log_str, tr("Loading Flags") );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    fseek(inf, offset+MAP_LAYERS_START + 5 * map_w * map_h, SEEK_SET);
    for (y = 0; y < map_h; ++y)
        for (x = 0; x < map_w; ++x)
        {
            /* default is no flag */
            map[x][y].nation = 0;
            map[x][y].player = 0;
            map[x][y].deploy_center = 0;
            /* default is no mil target */
            map[x][y].obj = 0;
            c+=fread(&(map[x][y].terrain_id), 2, 1, inf);
            for ( i = 0; i < terrain_type_count; i++ ) {
                if ( terrain_types[i].id == tile_type[map[x][y].terrain_id] )
                {
                    map[x][y].terrain = &terrain_types[i];
                }
            }
            /* tile not found, used first one */
            if ( map[x][y].terrain == 0 )
                map[x][y].terrain = &terrain_types[0];
            /* check image id -- set offset */
            map[x][y].image_offset_x = ( map[x][y].terrain_id ) % terrain_columns * hex_w;
            map[x][y].image_offset_y = ( map[x][y].terrain_id ) / terrain_columns * hex_h;
            /* clear units on this tile */
            map[x][y].g_unit = 0;
            map[x][y].a_unit = 0;
        }
    if (c!=map_w*map_h)
        fprintf(stderr, "WARNING: SET file too short !\n");
    //get the country(flag) info
    fseek(inf, offset+MAP_LAYERS_START + 3 * map_w * map_h, SEEK_SET);
    for (y = 0; y < map_h; ++y)
    {
        for (x = 0; x < map_w; ++x){
            c = 0;
            fread(&c, 1, 1, inf);
            map[x][y].obj = 0;
            map[x][y].nation = nation_find_by_id( c - 1 );
            if (map[x][y].nation != 0)
            {
//                fprintf(stderr, "%s ", map[x][y].nation->name);
                map[x][y].player = player_get_by_nation( map[x][y].nation );
//                fprintf(stderr, "%s ", map[x][y].player->name);
                if ( map[x][y].nation )
                    map[x][y].deploy_center = 1;
            }
        }
//        fprintf(stderr, "\n");
    }
    //get the names
    fseek(inf, offset+MAP_LAYERS_START + 0 * map_w * map_h, SEEK_SET);

    for (y = 0; y < map_h; ++y)
        for (x = 0; x < map_w; ++x)
        {
            i = 0;
            fread( &i, 2, 1, inf );
            i = SDL_SwapLE16( i );
            tile_get_name( name_file, i, name_buf );
            map[x][y].name = strdup( name_buf );
        }
/*
    //get the road connectivity
    fseek(inf,offset+MAP_LAYERS_START + 2 * map_w * map_h, SEEK_SET);
    for (y = 0; y < map_h; ++y)
        for (x = 0; x < map_w; ++x){
            fread(&(map[x][y].rc), 1, 1, inf);
            if (map[x][y].rc&(~0xBB)) printf("WARNING: Wrong road at %d,%d !\n",x,y);

        }
*/
    return 1;
}

// read SET and STM files
int load_map_pg(char *fullName) {
    FILE *inf;
    int error=0;

    inf = fopen(fullName, "rb");
    if (!inf)
        return 0;

    error=parse_set_file(inf,0);
    fclose(inf);

    if (error)
        return error;

    return 1;
}

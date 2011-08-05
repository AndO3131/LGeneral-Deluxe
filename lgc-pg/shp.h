/***************************************************************************
                          shp.h -  description
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
 
#ifndef __SHP_H
#define __SHP_H
 
#include <SDL.h> 
 
/*
====================================================================
SHP file format:
magic (4 byte)
icon_count (4 byte)
offset_list:
  icon_offset (4 byte)
  icon_palette_offset (4 byte)
Icon format:
header (24 byte)
raw data
====================================================================
*/

/*
====================================================================
Icon header.
height:        height of icon - 1
width:         width of icon - 1
cx, cy:        center_x, center_y ???
x1,x2,y1,y2:   position and size of actual icon
actual_width:  
actual_height: size of rect (x1,y1,x2,y2)
valid:         do x1,x2,y1,y2 make sense?
====================================================================
*/
typedef struct {
    int height; 
    int width; 
    int cx, cy;  
    int x1, y1, x2, y2;
    int actual_width;
    int actual_height;
    int valid; 
} Icon_Header;

/*
====================================================================
SHP icon file is converted to this structure.
surf:    surface with icons listed vertically
headers: information on each icon listed
offsets: y offset where icon starts
count:   number of icons listed
====================================================================
*/
typedef struct {
    SDL_Surface *surf;
    Icon_Header *headers;
    int *offsets;
    int count;
} PG_Shp;


/*
====================================================================
Get SDL pixel from R,G,B values
====================================================================
*/
#define MAPRGB( red, green, blue ) SDL_MapRGB( SDL_GetVideoSurface()->format, red, green, blue )
/*
====================================================================
Transparency color key.
====================================================================
*/
#define CKEY_RED   0x0 
#define CKEY_GREEN 0x0
#define CKEY_BLUE  0x0

/*
====================================================================
Draw/get a pixel at x,y in surf.
====================================================================
*/
void set_pixel( SDL_Surface *surf, int x, int y, int pixel );
Uint32 get_pixel( SDL_Surface *surf, int x, int y );

/*
====================================================================
Load a SHP file to a PG_Shp struct.
====================================================================
*/
PG_Shp *shp_load( const char *fname );
/*
====================================================================
Free a PG_Shp struct.
====================================================================
*/
void shp_free( PG_Shp **shp );

/*
====================================================================
Read all SHP files in source directory and save to directory 
'.view' in dest directory.
====================================================================
*/
int shp_all_to_bmp( void );

#endif

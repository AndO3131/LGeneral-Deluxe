/***************************************************************************
                          shp.c -  description
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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <SDL_endian.h>
#include "misc.h"
#include "shp.h"

/*
====================================================================
Externals
====================================================================
*/
extern char *source_path;
extern char *dest_path;
extern int use_def_pal;

/*
====================================================================
Default palette. Used if icon is not associated with a palette.
====================================================================
*/
typedef struct { int r; int g; int b; } RGB_Entry;
RGB_Entry def_pal[256] =  {
{   0,   0,   0},{   0,   0, 171},{   0, 171,   0},  // 0-2
{   0, 171, 171},{ 171,   0,   0},{ 171,   0, 171},  // 3-5
{ 171,  87,   0},{ 171, 171, 171},{  87,  87,  87},  // 6-8
{  87,  87, 255},{  87, 255,  87},{  87, 255, 255},  // 9-11
{ 255,  87,  87},{ 255,  87, 255},{ 255, 255,  87},  // 12-14
{ 255, 255, 255},{  79,   0,   0},{ 107,   0,   0},  // 15-17
{ 135,   0,   0},{ 167,   7,   0},{ 195,  11,   0},  // 18-20
{ 223,  19,   0},{ 255,   0,   0},{ 255,  83,  19},  // 21-23
{ 255, 131,  39},{ 255, 175,  59},{ 255, 211,  79},  // 24-26
{ 255, 243,  99},{ 243, 255, 123},{ 255, 255, 255},  // 27-29
{ 223, 235, 223},{ 195, 211, 195},{ 167, 191, 167},  // 30-32
{ 139, 171, 139},{ 119, 151, 119},{  95, 131,  95},  // 33-35
{  75, 111,  75},{  59,  91,  59},{  43,  71,  43},  // 36-38
{ 255, 255, 255},{ 227, 231, 243},{ 215, 219, 235},  // 39-41
{ 203, 211, 231},{ 191, 199, 227},{ 183, 191, 223},  // 42-44
{ 171, 183, 219},{ 163, 175, 211},{ 151, 163, 207},  // 45-47
{ 139, 155, 203},{ 131, 147, 199},{ 123, 139, 191},  // 48-50
{ 115, 131, 187},{ 107, 123, 183},{  99, 119, 179},  // 51-53
{  91, 111, 175},{ 255, 255, 255},{ 231, 239, 239},  // 54-56
{ 211, 219, 219},{ 191, 203, 203},{ 171, 187, 187},  // 57-59
{ 151, 171, 171},{ 135, 155, 155},{ 119, 139, 139},  // 60-62
{  99, 123, 123},{  87, 107, 107},{  71,  91,  91},  // 63-65
{  55,  75,  75},{  43,  59,  59},{  31,  43,  43},  // 66-68
{  15,  27,  27},{   7,  11,  11},{ 255, 243, 143},  // 69-71
{ 247, 231, 135},{ 239, 219, 127},{ 231, 211, 123},  // 72-74
{ 227, 199, 119},{ 219, 191, 111},{ 211, 179, 107},  // 75-77
{ 207, 171, 103},{ 187, 195,  39},{ 251, 243, 167},  // 78-80
{ 239, 235, 163},{ 227, 227, 159},{ 215, 215, 151},  // 81-83
{ 199, 203, 147},{ 187, 191, 143},{ 175, 179, 135},  // 84-86
{ 159, 167, 131},{ 147, 155, 123},{ 135, 143, 115},  // 87-89
{ 223, 219, 123},{ 207, 207, 119},{ 195, 195, 119},  // 90-92
{ 179, 179, 115},{ 163, 167, 111},{ 151, 155, 107},  // 93-95
{ 139, 143, 103},{ 127, 131,  95},{ 115, 119,  91},  // 96-98
{ 195, 195, 119},{  51,  39,  27},{  55,  43,  31},  // 99-101
{  63,  47,  35},{  71,  55,  39},{  75,  59,  43},  // 102-104
{  83,  67,  43},{  91,  71,  51},{  95,  79,  55},  // 105-107
{ 103,  83,  59},{ 111,  91,  63},{ 191, 131,  47},  // 108-110
{ 254, 254, 254},{ 255, 255, 151},{ 255,  87, 255},  // 111-113 111=blink white
                                                     // 112= blink gold
{ 111,  91,  63},{ 115,  95,  67},{ 123, 103,  71},  // 114-116
{ 131, 111,  75},{ 139, 119,  83},{ 255, 255,  87},  // 117-119
{   0, 171,   0},{  87,  87, 255},{   0,   0, 171},  // 120-122
{ 135, 143, 115},{ 159, 167, 131},{  95, 223, 255},  // 123-125
{ 203, 211, 231},{ 231, 211, 171},{  59,   0,   0},  // 126-128
{  83,   0,   0},{ 103,   0,   0},{ 127,   7,   0},  // 129-131
{ 147,  11,   0},{ 167,  15,   0},{ 191,   0,   0},  // 132-134
{ 191,  63,  15},{ 191,  99,  31},{ 191, 131,  47},  // 135-137
{ 191, 159,  59},{ 191, 183,  75},{ 183, 191,  95},  // 138-140
{ 191, 191, 191},{ 167, 179, 167},{ 147, 159, 147},  // 141-143
{ 127, 143, 127},{ 107, 131, 107},{  91, 115,  91},  // 144-146
{  71,  99,  71},{  59,  83,  59},{  47,  71,  47},  // 147-149
{  35,  55,  35},{ 191, 191, 191},{ 171, 175, 183},  // 150-152
{ 163, 167, 179},{ 155, 159, 175},{ 143, 151, 171},  // 153-155
{ 139, 143, 167},{ 131, 139, 167},{ 123, 131, 159},  // 156-158
{ 115, 123, 155},{ 107, 119, 155},{  99, 111, 151},  // 159-161
{  95, 107, 143},{  87,  99, 143},{  83,  95, 139},  // 162-164
{  75,  91, 135},{  71,  83, 131},{ 191, 191, 191},  // 165-167
{ 175, 179, 179},{ 159, 167, 167},{ 143, 155, 155},  // 168-170
{ 131, 143, 143},{ 115, 131, 131},{ 103, 119, 119},  // 171-173
{  91, 107, 107},{  75,  95,  95},{  67,  83,  83},  // 174-176
{  55,  71,  71},{  43,  59,  59},{  35,  47,  47},  // 177-179
{  23,  35,  35},{  11,  23,  23},{   7,  11,  11},  // 180-182
{ 191, 183, 107},{ 187, 175, 103},{ 179, 167,  95},  // 183-185
{ 175, 159,  95},{ 171, 151,  91},{ 167, 143,  83},  // 186-188
{ 159, 135,  83},{ 155, 131,  79},{ 143, 147,  31},  // 189-191
{ 191, 183, 127},{ 179, 179, 123},{ 171, 171, 119},  // 192-194
{ 163, 163, 115},{ 151, 155, 111},{ 143, 143, 107},  // 195-197
{ 131, 135, 103},{ 119, 127,  99},{ 111, 119,  95},  // 198-200
{ 103, 107,  87},{ 167, 167,  95},{ 155, 155,  91},  // 201-203
{ 147, 147,  91},{ 135, 135,  87},{ 123, 127,  83},  // 204-206
{ 115, 119,  83},{ 107, 107,  79},{  95,  99,  71},  // 207-209
{  87,  91,  71},{ 147, 147,  91},{ 147, 127,  87},  // 210-212
{ 155, 135,  91},{ 163, 143,  95},{ 171, 151, 103},  // 213-215
{ 179, 155, 107},{ 187, 167, 115},{ 195, 175, 119},  // 216-218
{ 203, 183, 127},{ 211, 191, 131},{ 219, 199, 139},  // 219-221
{ 143,  91,  31},{ 255,  87, 255},{ 191, 191, 115},  // 222-224 224=blink dark
{ 255,  87, 255},{ 223, 207, 143},{ 231, 215, 147},  // 225-227
{ 239, 223, 155},{ 247, 231, 163},{ 255, 239, 171},  // 228-230
{ 191, 191,  67},{   0, 131,   0},{  67,  67, 191},  // 231-233
{   0,   0, 131},{ 103, 107,  87},{ 119, 127,  99},  // 234-236
{  71, 167, 191},{ 155, 159, 175},{ 175, 159, 131},  // 237-239
{ 135, 143, 115},{ 159, 167, 131},{ 255,  35,  35},  // 240-242
{  43, 199, 183},{  43, 171, 199},{  43, 127, 199},  // 243-245
{  43,  83, 199},{  47,  43, 199},{  91,  43, 199},  // 246-248
{ 135,  43, 199},{ 179,  43, 199},{ 199,  43, 175},  // 249-251
{ 199,  43, 131},{ 199,  43,  87},{ 199,  43,  43},  // 252-254
{ 255, 119, 123}};                                   // 255 

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Read icon header from file pos.
====================================================================
*/
static void shp_read_icon_header( FILE *file, Icon_Header *header )
{
    memset( header, 0, sizeof( Icon_Header ) );
    fread( &header->height, 2, 1, file );
    header->height = SDL_SwapLE16(header->height);
    header->height++; /* if y1 == y2 it is at least one line anyway */
    fread( &header->width, 2, 1, file );
    header->width = SDL_SwapLE16(header->width);
    header->width++;
    fread( &header->cx, 2, 1, file );
    header->cx = SDL_SwapLE16(header->cx);
    fread( &header->cy, 2, 1, file );
    header->cy = SDL_SwapLE16(header->cy);
    fread( &header->x1, 4, 1, file );
    header->x1 = SDL_SwapLE32(header->x1);
    fread( &header->y1, 4, 1, file );
    header->y1 = SDL_SwapLE32(header->y1);
    fread( &header->x2, 4, 1, file );
    header->x2 = SDL_SwapLE32(header->x2);
    fread( &header->y2, 4, 1, file );
    header->y2 = SDL_SwapLE32(header->y2);
    header->valid = 1;
    if ( header->x1 >= header->width || header->x2 >= header->width )
        header->valid = 0;
    if ( header->y1 >= header->height || header->y2 >= header->height )
        header->valid = 0;
    if ( header->x1 < 0 ) {
        header->x1 = 0;
        header->actual_width = header->x2;
    }
    else
        header->actual_width = header->x2 - header->x1 + 1;
    if ( header->y1 < 0 ) {
        header->y1 = 0;
        header->actual_height = abs( header->y1 ) + header->y2 + 1;
    }
    else 
        header->actual_height = header->y2 - header->y1 + 1;
}

/*
====================================================================
Read palette from file pos.
====================================================================
*/
static void shp_read_palette( FILE *file, RGB_Entry *pal )
{
    int i;
    int count = 0;
    int id;
    int part;
    memset( pal, 0, sizeof( RGB_Entry ) * 256 );
    fread( &count, 4, 1, file );
    count = SDL_SwapLE32(count);
    for ( i = 0; i < count; i++ ) {
        id = 0; fread( &id, 1, 1, file );
        if ( id >= 256 ) id = 255;
        part = 0; fread( &part, 1, 1, file ); pal[id].r = part * 4;
        part = 0; fread( &part, 1, 1, file ); pal[id].g = part * 4;
        part = 0; fread( &part, 1, 1, file ); pal[id].b = part * 4;
    }
}

/*
====================================================================
Read raw SHP icon data from file and draw it to 'surf' at 'y'
interpreting the indices of the icon as palette entry indices.
====================================================================
*/
static void shp_read_icon( FILE *file, SDL_Surface *surf, int y, RGB_Entry *pal, Icon_Header *header )
{
    int bytes, flag;
    int x = 0, i, y1 = header->y1;
    Uint8 buf;
    Uint32 ckey = MAPRGB( CKEY_RED, CKEY_GREEN, CKEY_BLUE ); /* transparent color key */
    /* read */
    while ( y1 <= header->y2 ) {
        buf = 0; fread( &buf, 1, 1, file );
        flag = buf % 2; bytes = buf / 2;
        if ( bytes == 0 && flag == 1 ) {
            /* transparency */
            buf = 0; fread( &buf, 1, 1, file );
            for ( i = 0; i < buf; i++ )
                set_pixel( surf, x++ + header->x1, y + y1, ckey );
        }
        else
            if ( bytes == 0 ) {
                /* end of line */
                y1++;
                x = 0;
            }
            else
                if ( flag == 0 ) {
                    /* byte row */
                    buf = 0; fread( &buf, 1, 1, file );
                    for ( i = 0; i < bytes; i++ )
                        set_pixel( surf, x++ + header->x1, y + y1, MAPRGB( pal[buf].r, pal[buf].g, pal[buf].b ) );
                }
                else {
                    /* bytes != 0 && flag == 1: read next bytes uncompressed */
                    for ( i = 0; i < bytes; i++ ) {
                        buf = 0; fread( &buf, 1, 1, file );
                        set_pixel( surf, x++ + header->x1, y + y1, MAPRGB( pal[buf].r, pal[buf].g, pal[buf].b ) );
                    }
                }
    }
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Draw a pixel at x,y in surf.
====================================================================
*/
void set_pixel( SDL_Surface *surf, int x, int y, int pixel )
{
    int pos = 0;

    pos = y * surf->pitch + x * surf->format->BytesPerPixel;
    memcpy( surf->pixels + pos, &pixel, surf->format->BytesPerPixel );
}
Uint32 get_pixel( SDL_Surface *surf, int x, int y )
{
    int pos = 0;
    Uint32 pixel = 0;
    pos = y * surf->pitch + x * surf->format->BytesPerPixel;
    memcpy( &pixel, surf->pixels + pos, surf->format->BytesPerPixel );
    return pixel;
}

/*
====================================================================
Load a SHP file to a PG_Shp struct.
Since SHP files are only found in source path (=original PG data),
for simplicity this function only gets the file name and the 
source_path is always prepended for opening the file.
====================================================================
*/
PG_Shp *shp_load( const char *fname )
{
    int i;
    FILE *file = 0;
    char path[MAXPATHLEN];
    int dummy;
    int width = 0, height = 0;
    int old_pos, pos, pal_pos;
    PG_Shp *shp = 0; 
    SDL_PixelFormat *pf = SDL_GetVideoSurface()->format;
    Uint32 ckey = MAPRGB( CKEY_RED, CKEY_GREEN, CKEY_BLUE ); /* transparent color key */
    Icon_Header header;
    RGB_Entry pal[256];
    RGB_Entry *actual_pal = 0;
    int icon_maxw = 60, icon_maxh = 50;
    
    snprintf( path, MAXPATHLEN, "%s/%s", source_path, fname );
    if ( ( file = fopen_ic( path, "r" ) ) == NULL ) {
        printf("Could not open file %s\n",path);
        return NULL;
    }   
    
    /* magic */
    fread( &dummy, 4, 1, file );
    /* shp struct */
    shp = calloc( 1, sizeof( PG_Shp ) );
    /* icon count */
    fread( &shp->count, 4, 1, file );
    shp->count = SDL_SwapLE32(shp->count);
    if ( shp->count == 0 ) {
        fprintf( stderr, "%s: no icons found\n", path );
        goto failure;
    }
    /* create surface (measure size first) */
    for ( i = 0; i < shp->count; i++ ) {
        /* read file position of actual data and palette */
        fread( &pos, 4, 1, file );
        pos = SDL_SwapLE32(pos);
        fread( &dummy, 4, 1, file );
        old_pos = ftell( file );
        /* read header */
        fseek( file, pos, SEEK_SET );
        shp_read_icon_header( file, &header );
        /* XXX if icon is too large, ignore it and replace with an empty
         * icon of maximum size; use hardcoded limit which is basically okay
         * as we convert PG data and can assume the icons have size of map
         * tile at maximum. */
        if ( header.width > icon_maxw || header.height > icon_maxh ) {
            fprintf( stderr, "Icon %d in %s is too large (%dx%d), replacing "
                                        "with empty icon\n", i, fname, 
                                        header.width,header.height  );
            header.width = icon_maxw;
            header.height = icon_maxh;
            header.valid = 0;
        }
        if ( header.width > width )
            width = header.width;
        height += header.height;
        fseek( file, old_pos, SEEK_SET );
    }
    shp->surf = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height, pf->BitsPerPixel, pf->Rmask, pf->Gmask, pf->Bmask, pf->Amask );
    if ( shp->surf == 0 ) {
        fprintf( stderr, "error creating surface: %s\n", SDL_GetError() );
        goto failure;
    }
    SDL_FillRect( shp->surf, 0, ckey );
    /* read icons */
    shp->offsets = calloc( shp->count, sizeof( int ) );
    shp->headers = calloc( shp->count, sizeof( Icon_Header ) );
    fseek( file, 8, SEEK_SET );
    for ( i = 0; i < shp->count; i++ ) {
        /* read position of data and palette */
        pos = pal_pos = 0;
        fread( &pos, 4, 1, file );
        pos = SDL_SwapLE32(pos);
        fread( &pal_pos, 4, 1, file );
        pal_pos = SDL_SwapLE32(pal_pos);
        old_pos = ftell( file );
        /* read palette */
        if ( !use_def_pal && pal_pos > 0 ) {
            fseek( file, pal_pos, SEEK_SET );
            shp_read_palette( file, pal );
            actual_pal = pal;
        }
        else
            actual_pal = def_pal;
        /* read header */
        fseek( file, pos, SEEK_SET );
        shp_read_icon_header( file, &header );
        /* see comment in measure loop above; have empty icon if too large */
        if ( header.width > icon_maxw || header.height > icon_maxh ) {
            /* error message already given in measure loop */
            header.width = icon_maxw;
            header.height = icon_maxh;
            header.valid = 0;
        }
        if ( header.valid )
            shp_read_icon( file, shp->surf, shp->offsets[i], actual_pal, &header );
        if ( i < shp->count - 1 )
            shp->offsets[i + 1] = shp->offsets[i] + header.height;
        memcpy( &shp->headers[i], &header, sizeof( Icon_Header ) );
        fseek( file, old_pos, SEEK_SET );
    }
    fclose( file );
    return shp;
failure:
    if ( file ) fclose( file );
    if ( shp ) shp_free( &shp );
    return 0;
}

/*
====================================================================
Free a PG_Shp struct.
====================================================================
*/
void shp_free( PG_Shp **shp )
{
    if ( *shp == 0 ) return;
    if ( (*shp)->headers ) free( (*shp)->headers );
    if ( (*shp)->offsets ) free( (*shp)->offsets );
    if ( (*shp)->surf ) SDL_FreeSurface( (*shp)->surf );
    free( *shp ); *shp = 0;
}

/*
====================================================================
Read all SHP files in source directory and save to directory 
'.view' in dest directory.
====================================================================
*/
int shp_all_to_bmp( void )
{
    char path[MAXPATHLEN];
    int length;
    DIR *dir = 0;
    PG_Shp *shp;
    struct dirent *dirent = 0;
    /* open directory */
    if ( ( dir = opendir( source_path ) ) == 0 ) {
        fprintf( stderr, "%s: can't open directory\n", source_path );
        return 0;
    }
    while ( ( dirent = readdir( dir ) ) != 0 ) {
        if ( dirent->d_name[0] == '.' ) continue;
        if ( !strncmp( "tacally.shp", strlower( dirent->d_name ), 11 ) ) continue;
        if ( !strncmp( "tacgerm.shp", strlower( dirent->d_name ), 11 ) ) continue;
        if ( !strncmp( "a_", strlower( dirent->d_name ), 2 ) ) continue;
        length = strlen( dirent->d_name );
        if ( (dirent->d_name[length - 1] != 'P' || dirent->d_name[length - 2] != 'H' || dirent->d_name[length - 3] != 'S') &&
             (dirent->d_name[length - 1] != 'p' || dirent->d_name[length - 2] != 'h' || dirent->d_name[length - 3] != 's') )
            continue;
        printf( "%s...\n", dirent->d_name );
        if ( ( shp = shp_load( dirent->d_name ) ) == 0 ) continue;
        snprintf( path, MAXPATHLEN, "%s/.view/%s.bmp", dest_path, dirent->d_name );
        SDL_SaveBMP( shp->surf, path );
        shp_free( &shp );
    }
    closedir( dir );
    return 1;
}

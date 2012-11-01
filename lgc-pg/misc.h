/***************************************************************************
                          tools.h  -  description
                             -------------------
    begin                : Fri Jan 19 2001
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

#ifndef __MISC_H
#define __MISC_H

#include "config.h"
#include <SDL.h> 

/* check if number is odd or even */
#define ODD( x ) ( x & 1 )
#define EVEN( x ) ( !( x & 1 ) )

/* free with a check */
#define FREE( ptr ) { if ( ptr ) free( ptr ); ptr = 0; }

/* check if a serious of flags is set in source */
#define CHECK_FLAGS( source, flags ) ( source & (flags) )

/* return random value between ( and including ) upper,lower limit */
#define RANDOM( lower, upper ) ( ( rand() % ( ( upper ) - ( lower ) + 1 ) ) + ( lower ) )

/* check if within this rect */
#define FOCUS( cx, cy, rx, ry, rw, rh ) ( cx >= rx && cy >= ry && cx < rx + rw && cy < ry + rh )

/* compare strings */
#define STRCMP( str1, str2 ) ( strlen( str1 ) == strlen( str2 ) && !strncmp( str1, str2, strlen( str1 ) ) )

/* return minimum */
#define MINIMUM( a, b ) ((a<b)?a:b)

#define MAXPATHLEN 512

/* delay struct */
typedef struct {
    int limit;
    int cur;
} Delay;

/* set delay to ms milliseconds */
inline void set_delay( Delay *delay, int ms );

/* reset delay ( cur = 0 )*/
inline void reset_delay( Delay *delay );

/* check if time's out ( add ms milliseconds )and reset */
inline int timed_out( Delay *delay, int ms );

/* return distance betwteen to map positions */
int get_dist( int x1, int y1, int x2, int y2 );

/* init random seed by using ftime */
void set_random_seed();

/* get coordintaes from string */
void get_coord( char *str, int *x, int *y );

// text structure //
typedef struct {
    char **lines;
    int count;
} Text;
// convert a str into text ( for listbox ) //
Text* create_text( char *str, int char_width );
// delete text //
void delete_text( Text *text );

/*
====================================================================
Delete an array of strings and set it and counter 0.
====================================================================
*/
void delete_string_list( char ***list, int *count );

/*
====================================================================
To simplify conversion from string to flag tables of these
entries are used.
====================================================================
*/
typedef struct { char *string; int flag; } StrToFlag;
/*
====================================================================
This function checks if 'name' occurs in fct and return the flag
or 0 if not found.
====================================================================
*/
int check_flag( char *name, StrToFlag *fct );

/*
====================================================================
Get neighbored tile coords clockwise with id between 0 and 5.
====================================================================
*/
int get_close_hex_pos( int x, int y, int id, int *dest_x, int *dest_y );

/*
====================================================================
Check if these positions are close to each other.
====================================================================
*/
int is_close( int x1, int y1, int x2, int y2 );

/*
====================================================================
Copy source to dest and at maximum limit chars. Terminate with 0.
====================================================================
*/
void strcpy_lt( char *dest, char *src, int limit );

/*
====================================================================
Return (pointer to static duplicate) string as lowercase.
====================================================================
*/
char *strlower( const char *str );

/*
====================================================================
Copy file
====================================================================
*/
void copy_ic( char *sname, char *dname );
void copy( char *sname, char *dname );
int copy_pg_bmp( char *src, char *dest );

/*
====================================================================
Return the directory the game data is installed under.
====================================================================
*/
const char *get_gamedir(void);

/** Try to open file with lowercase name, then with uppercase name.
 * If both fails return NULL. Path itself is considered to be in
 * correct case, only the name after last '/' is modified. */
FILE *fopen_ic( const char *path, const char *mode );

/*
====================================================================
Copy to dest from source horizontally mirrored.
====================================================================
*/
void copy_surf_mirrored( SDL_Surface *source, SDL_Rect *srect, SDL_Surface *dest, SDL_Rect *drect );

#endif

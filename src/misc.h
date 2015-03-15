/***************************************************************************
                          misc.h  -  description
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

struct PData;
struct _Font;

#define MAXPATHLEN 512

/* check if number is odd or even */
#define ODD( x ) ( x & 1 )
#define EVEN( x ) ( !( x & 1 ) )

/* free with a check */
#define FREE( ptr ) { if ( ptr ) free( ptr ); ptr = 0; }

/* check if a serious of flags is set in source */
#define CHECK_FLAGS( source, flags ) ( source & (flags) )

/* return random value between ( and including ) upper,lower limit */
#define RANDOM( lower, upper ) ( ( rand() % ( ( upper ) - ( lower ) + 1 ) ) + ( lower ) )
#define DICE(maxeye) (1+(int)(((double)maxeye)*rand()/(RAND_MAX+1.0)))

/* check if within this rect */
#define FOCUS( cx, cy, rx, ry, rw, rh ) ( cx >= rx && cy >= ry && cx < rx + rw && cy < ry + rh )

/* compare strings */
#define STRCMP( str1, str2 ) ( strlen( str1 ) == strlen( str2 ) && !strncmp( str1, str2, strlen( str1 ) ) )

/* return minimum */
#define MINIMUM( a, b ) ((a<b)?a:b)

/* return maximum */
#define MAXIMUM( a, b ) ((a>b)?a:b)

/* compile time assert */
#ifndef NDEBUG
#  define COMPILE_TIME_ASSERT( x ) do { switch (0) {case 0: case x:;} } while(0)
#else
#  define COMPILE_TIME_ASSERT( x )
#endif
/* check for symbol existence at compile-time */
#ifndef NDEBUG
#  define COMPILE_TIME_ASSERT_SYMBOL( s ) COMPILE_TIME_ASSERT( sizeof s )
#else
#  define COMPILE_TIME_ASSERT_SYMBOL( s )
#endif

/* ascii-codes of game-related symbols */
enum GameSymbols {
    CharDunno1 = 1,	/* no idea (kulkanie?) */
    CharDunno2 = 2,	/* no idea (kulkanie?) */
    CharStrength = 3,
    CharFuel = 4,
    CharAmmo = 5,
    CharEntr = 6,
    CharBack = 17,	/* no idea actually (kulkanie?) */
    CharDistance = 26,
    CharNoExp = 128,
    CharExpGrowth = CharNoExp,
    CharExp = 133,
    CharCheckBoxEmpty = 136,
    CharCheckBoxEmptyFocused = 137,
    CharCheckBoxChecked = 138,
    CharCheckBoxCheckedFocused = 139,
};

#define GS_STRENGTH "\003"
#define GS_FUEL "\004"
#define GS_AMMO "\005"
#define GS_ENTR "\006"
#define GS_BACK "\021"
#define GS_DISTANCE "\032"
#define GS_NOEXP "\200"
#define GS_EXP "\205"
#define GS_CHECK_BOX_EMPTY "\210"
#define GS_CHECK_BOX_EMPTY_FOCUSED "\211"
#define GS_CHECK_BOX_CHECKED "\212"
#define GS_CHECK_BOX_CHECKED_FOCUSED "\213"

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
void get_coord( const char *str, int *x, int *y );

/** text structure */
typedef struct {
    char **lines;
    int count;
} Text;
/** convert a str into text ( for listbox ) */
Text* create_text( struct _Font *fnt, const char *str, int width );
/** delete text */
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
int check_flag( const char *name, StrToFlag *fct );

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
void strcpy_lt( char *dest, const char *src, int limit );

/*
====================================================================
Returns the basename of the given file.
====================================================================
*/
const char *get_basename(const char *filename);

/*
====================================================================
Return the domain out of the given parse-tree. If not contained,
construct the domain from the file name.
Will be allocated on the heap.
====================================================================
*/
char *determine_domain(struct PData *tree, const char *filename);

/*
====================================================================
Return the directory the game data is installed under.
====================================================================
*/
const char *get_gamedir(void);

/** Macro wrapper to read data from file with (very) simple error handling.
 * Only to be used as standalone command, not in conditions. */
#define _fread(ptr,size,nmemb,stream) \
	do { int _freadretval = fread(ptr,size,nmemb,stream); if (_freadretval != nmemb && !feof(stream)) fprintf(stderr, "%s: %d: _fread error\n",__FILE__,__LINE__);} while (0)

#endif

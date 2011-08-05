/***************************************************************************
                          misc.c  -  description
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

#include "misc.h"
#include "parser.h"
#include "localize.h"
#include "paths.h"
#include "sdl.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern int map_w, map_h; /* FIX ME! */

/* compares to strings and returns true if their first strlen(str1) chars are equal */
inline int equal_str( const char *str1, const char *str2 )
{
    if ( strlen( str1 ) != strlen( str2 ) ) return 0;
    return ( !strncmp( str1, str2, strlen( str1 ) ) );
}

/* set delay to ms milliseconds */
inline void set_delay( Delay *delay, int ms )
{
    delay->limit = ms;
    delay->cur = 0;
}

/* reset delay ( cur = 0 )*/
inline void reset_delay( Delay *delay )
{
    delay->cur = 0;
}

/* check if times out and reset */
inline int timed_out( Delay *delay, int ms )
{
    delay->cur += ms;
    if ( delay->cur >= delay->limit ) {

        delay->cur = 0;
        return 1;

    }
    return 0;
}

/* Convert grid coordinates into isometric (diagonal) coordinates. */
inline static void convert_coords_to_diag( int *x, int *y )
{
  *y += (*x + 1) / 2;
}

/* return distance between to map positions */
int get_dist( int x0, int y0, int x1, int y1 )
{
    int dx, dy;
    convert_coords_to_diag( &x0, &y0 );
    convert_coords_to_diag( &x1, &y1 );
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    if ( (y1 <= y0 && x1 >= x0) || (y1 > y0 && x1 < x0) )
        return dx + dy;
    else if (dx > dy)
        return dx;
    else
        return dy;
}

/* init random seed by using ftime */
void set_random_seed()
{
    srand( (unsigned int)time( 0 ) );
}

/* get coordinates from string */
void get_coord( const char *str, int *x, int *y )
{
    int i;
    char *cur_arg;

    *x = *y = 0;

    /* get position of comma */
    for ( i = 0; i < strlen( str ); i++ )
        if ( str[i] == ',' ) break;
    if ( i == strlen( str ) ) {
        fprintf( stderr, tr("get_coord: no comma found in pair of coordinates '%s'\n"), str );
        return; /* no comma found */
    }

    /* y */
    cur_arg = (char *)str + i + 1;
    if ( cur_arg[0] == 0 )
        fprintf( stderr, tr("get_coord: warning: y-coordinate is empty (maybe you left a space between x and comma?)\n") );
    *y = atoi( cur_arg );
    /* x */
    cur_arg = strdup( str ); cur_arg[i] = 0;
    *x = atoi( cur_arg );
    FREE( cur_arg );
}

/* replace new_lines with spaces in text */
void repl_new_lines( char *text )
{
    int i;
    for ( i = 0; i < strlen( text ); i++ )
        if ( (unsigned char)text[i] < 32 )
            text[i] = 32;
}

/** return 1 if ch is a line breaking character */
inline static int text_is_linebreak(char ch)
{
    return ch == '#';
}

/** return 1 if text is breakable just before end */
inline static int text_is_breakable(const char *begin, const char *end)
{
    return begin != end
            && (end[-1] == '\t' || end[-1] == '\n' || end[-1] == ' '
                || end[-1] == '-');
}

/** transient data for building up a struct Text object */
struct TextData {
    Text *text;
    int reserved;
};

/**
 * Adds the given range as a line to the struct Text object,
 * performing text transformations as necessary
 */
static void text_add_line(struct TextData *td, const char *begin, const char *end)
{
    int idx = td->text->count;
    char *out;
    if (idx == td->reserved) {
        if (td->reserved == 0) td->reserved = 1;
        td->reserved *= 2;
        td->text->lines = realloc(td->text->lines, td->reserved * sizeof td->text->lines[0]);
    }

    out = td->text->lines[idx] = malloc(end - begin + 1);
    for (; begin != end; ++begin, ++out) {
        switch (*begin) {
            case '\n':
            case '\t':
            case '\r':
            case '#': *out = ' '; break;
            default: *out = *begin; break;
        }
    }
    *out = '\0';

    td->text->count++;
}

/**
 * convert a str into text ( for listbox )
 * width is the available width of a line in pixels
 */
Text* create_text( struct _Font *fnt, const char *orig_str, int width )
{
    struct TextData td;
    const char *line_start = orig_str;
    const char *head = orig_str;
    const char *committed = orig_str;
    int cumulated_width = 0;	/* width of text in this line */
    int break_line = 0;
    if (width < 0) width = 0;

    memset(&td, 0, sizeof(td));
    td.text = calloc ( 1, sizeof( Text ) );

    while (*committed) {
        int ch_width = char_width(fnt, *head);

        if (committed != head && text_is_linebreak(head[-1]))
            break_line = 1;
        else if (cumulated_width > width) {
            /* if the word is too long to fit into one line,
             * force a line break at the current position.
             */
            if (committed == line_start)
                /* take away last char (unless only one) and break there */
                committed = head - (head - 1 != line_start);
            head = committed;
            break_line = 1;
        }
        else if (text_is_breakable(committed, head))
            committed = head;

        if (!break_line) {
            cumulated_width += ch_width;
            head++;
        }

        if (!*head) break_line = 1;

        if (break_line) {
            text_add_line(&td, line_start, head);
            line_start = committed = head;
            cumulated_width = 0;
            break_line = 0;
        }
    }

    if (!td.text->lines) text_add_line(&td, "", "" + 1);

    return td.text;
}

/** delete text */
void delete_text( Text *text )
{
    int i;

    if ( text == 0 ) return;

    if ( text->lines ) {
        for ( i = 0; i < text->count; i++ )
            if ( text->lines[i] )
                free( text->lines[i] );
        free( text->lines );
    }
    free( text );
}

/*
====================================================================
Delete an array of strings and set it and counter 0.
====================================================================
*/
void delete_string_list( char ***list, int *count )
{
    int i;
    if ( *list == 0 ) return;
    for ( i = 0; i < *count; i++ )
        if ( (*list)[i] ) free( (*list)[i] );
    free( *list );
    *list = 0; *count = 0;
}

/*
====================================================================
This function checks if 'name' occurs in fct and return the flag
or 0 if not found.
====================================================================
*/
int check_flag( const char *name, StrToFlag *fct )
{
    /* get flags */
    int i = 0;
    while ( fct[i].string[0] != 'X' ) {
        if ( STRCMP( fct[i].string, name ) )
            return fct[i].flag;
        i++;
    }
    if ( !STRCMP( "none", name ) ) 
        fprintf( stderr, tr("%s: unknown flag\n"), name );
    return 0;
}

/*
====================================================================
Get neighbored tile coords clockwise with id between 0 and 5.
====================================================================
*/
int get_close_hex_pos( int x, int y, int id, int *dest_x, int *dest_y )
{
    if ( id == 0 ) {
        *dest_x = x;
        *dest_y = y - 1;
    }
    else
    if ( id == 1 ) {
        if ( x & 1 ) {
            *dest_x = x + 1;
            *dest_y = y;
        }
        else {
            *dest_x = x + 1;
            *dest_y = y - 1;
        }
    }
    else
    if ( id == 2 ) {
        if ( x & 1 ) {
            *dest_x = x + 1;
            *dest_y = y + 1;
        }
        else {
            *dest_x = x + 1;
            *dest_y = y;
        }
    }
    else
    if ( id == 3 ) {
        *dest_x = x;
        *dest_y = y + 1;
    }
    else
    if ( id == 4 ) {
        if ( x & 1 ) {
            *dest_x = x - 1;
            *dest_y = y + 1;
        }
        else {
            *dest_x = x - 1;
            *dest_y = y;
        }
    }
    else
    if ( id == 5 ) {
        if ( x & 1 ) {
            *dest_x = x - 1;
            *dest_y = y;
        }
        else {
            *dest_x = x - 1;
            *dest_y = y - 1;
        }
    }
	if ( *dest_x <= 0 || *dest_y <= 0 || *dest_x >= map_w-1 || 
							*dest_y >= map_h-1 )
		return 0;
    return 1;
}

/*
====================================================================
Check if these positions are adjacent to each other.
====================================================================
*/
int is_close( int x1, int y1, int x2, int y2 )
{
    int i, next_x, next_y;
    if ( x1 == x2 && y1 == y2 ) return 1;
    for ( i = 0; i < 6; i++ )
        if ( get_close_hex_pos( x1, y1, i, &next_x, &next_y ) )
            if ( next_x == x2 && next_y == y2 )
                return 1;
    return 0;
}

/*
====================================================================
Copy source to dest and at maximum limit chars. Terminate with 0.
====================================================================
*/
void strcpy_lt( char *dest, const char *src, int limit )
{
    int len = strlen( src );
    if ( len > limit ) {
        strncpy( dest, src, limit );
        dest[limit] = 0;
    }
    else
        strcpy( dest, src );
}

/*
====================================================================
Returns the basename of the given file.
====================================================================
*/
const char *get_basename(const char *filename)
{
    const char *pos = strrchr(filename, '/');
    if (pos) return pos + 1;
    return filename;
}

/*
====================================================================
Return the domain out of the given parse-tree. If not contained,
construct the domain from the file name.
Will be allocated on the heap.
====================================================================
*/
char *determine_domain(struct PData *tree, const char *filename)
{
    char *domain;
    char *pos;
    if ( parser_get_string(tree, "domain", &domain) ) return domain;

    domain = strdup(get_basename(filename));
    pos = strchr(domain, '.');
    if (pos) *pos = 0;
    return domain;
}

/*
====================================================================
Return path of installation directory as static string (not thread
safe).
====================================================================
*/
const char *get_gamedir(void)
{
#ifndef INSTALLDIR
    return ".";
#else
    static char gamedir[MAXPATHLEN];
    snprintf( gamedir, MAXPATHLEN, "%s", INSTALLDIR );
    return gamedir;
#endif
}

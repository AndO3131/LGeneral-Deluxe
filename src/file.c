
/***************************************************************************
                          file.c  -  description
                             -------------------
    begin                : Thu Jan 18 2001
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "misc.h"
#include "list.h"
#include "file.h"
#include "localize.h"
#include "sdl.h"

//#define FILE_DEBUG
const int extension_image_length = 4;
const int extension_sound_length = 3;
const char *extension_image[] = { "bmp", "png", "jpg", "jpeg" };
const char *extension_sound[] = { "wav", "ogg", "mp3" };

/*
====================================================================
Read all lines from file.
====================================================================
*/
List *file_read_lines( FILE *file )
{
    List *list;
    char buffer[4096];

    if ( !file ) return 0;

    list = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    
    /* read lines */
    while( !feof( file ) ) {
        if ( !fgets( buffer, sizeof buffer - 1, file ) ) break;
        if ( buffer[0] == 10 ) continue; /* empty line */
        buffer[strlen( buffer ) - 1] = 0; /* cancel newline */
        list_add( list, strdup( buffer ) );
    }
    return list;
}

/*
====================================================================
Parse the predefined order of files from the given .order file.
Returns a list of files, or 0 if error.
====================================================================
*/
static List *file_get_order_list( const char *orderfile )
{
    List *l = 0;
    FILE *f = fopen( orderfile, "r" );
    if (!f) {
        fprintf( stderr, tr("%s cannot be opened\n"), orderfile );
        return 0;
    }
    
    l = file_read_lines( f );
    fclose( f );

    return l;
}

/*
====================================================================
Extracts the list of files that match the order list, and returns
the matches in the correct order. Returns an empty list otherwise.
====================================================================
*/
static List *file_extract_order_list( List *files, List *orderlist )
{
    List *extracted = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    char *ordered, *name;

    if ( !orderlist ) return extracted;
    
    list_reset( orderlist );
    while ( ( ordered = list_next( orderlist ) ) ) {
        list_reset( files );
        while ( ( name = list_next( files ) ) ) {
        
            if ( strcmp( name, ordered ) == 0 ) {
                list_transfer( files, extracted, name );
                break;
            }
        
        }
    }
    return extracted;
}

/*
====================================================================
Comparator for sorting. Compares alphabetically with
an entry prefixed by an asterisk considered to be smaller.
====================================================================
*/
static int file_compare( const void *ptr1, const void *ptr2 )
{
  const char * const s1 = *(const char **)ptr1;
  const char * const s2 = *(const char **)ptr2;
  if ( *s1 == '*' && *s2 != '*' ) return -1;
  else if ( *s1 != '*' && *s2 == '*' ) return +1;
  else return strcmp( s1, s2 );
}

/*
====================================================================
Return a list (autodeleted strings)
with all accessible files and directories in path
with the extension ext (if != 0). Don't show hidden files or
Makefile stuff.
The directoriers are marked with an asteriks.
====================================================================
*/
List* dir_get_entries( const char *path, const char *root, const char *ext )
{
    Text *text = 0;
    int i;
    DIR *dir;
    DIR *test_dir;
    struct dirent *dirent = 0;
    List *list;
    List *order = 0;
    List *extracted;
    struct stat fstat;
    char file_name[4096];
    FILE *file;
    /* open this directory */
    if ( ( dir = opendir( path ) ) == 0 ) {
        fprintf( stderr, tr("get_file_list: can't open parent directory '%s'\n"), path );
        return 0;
    }
    text = calloc( 1, sizeof( Text ) );
    /* use dynamic list to gather all valid entries */
    list = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    /* read each entry and check if its a valid entry, then add it to the dynamic list */
    while ( ( dirent = readdir( dir ) ) != 0 ) {
        /* compose filename */
        snprintf( file_name, sizeof file_name, "%s/%s", path, dirent->d_name );
        /* regard special hidden file .order */
        if ( STRCMP( dirent->d_name, ".order" ) ) {
            order = file_get_order_list( file_name );
            continue;
        }
        /* hiden stuff is not displayed */
        if ( dirent->d_name[0] == '.' && dirent->d_name[1] != '.' ) continue;
        if ( STRCMP( dirent->d_name, "Makefile" ) ) continue;
        if ( STRCMP( dirent->d_name, "Makefile.in" ) ) continue;
        if ( STRCMP( dirent->d_name, "Makefile.am" ) ) continue;
        /* check if it's the root directory */
        if ( root )
            if ( dirent->d_name[0] == '.' ) {
                if ( STRCMP( path, root ) )
                    continue;
            }
        /* get stats */
        if ( stat( file_name, &fstat ) == -1 ) continue;
        /* check directory */
        if ( S_ISDIR( fstat.st_mode ) ) {
            if ( ( test_dir = opendir( file_name ) ) == 0  ) continue;
            closedir( test_dir );
            sprintf( file_name, "*%s", dirent->d_name );
            list_add( list, strdup( file_name ) );
        }
        else
        /* check regular file */
        if ( S_ISREG( fstat.st_mode ) ) {
            /* test it */
            if ( ( file = fopen( file_name, "r" ) ) == 0 ) continue;
            fclose( file );
            /* check if this file has the proper extension */
            if ( ext )
                if ( !STRCMP( dirent->d_name + ( strlen( dirent->d_name ) - strlen( ext ) ), ext ) )
                    continue;
            list_add( list, strdup( dirent->d_name ) );
        }
    }
    /* close dir */
    closedir( dir );
    /* extract ordered entries
     * Directories are never ordered as they have an asterisk prefixed
     * (this is intentional!)
     */
    extracted = file_extract_order_list( list, order );
    /* convert to static list */
    text->count = list->count;
    text->lines = malloc( list->count * sizeof( char* ));
    list_reset( list );
    for ( i = 0; i < text->count; i++ )
        text->lines[i] = strdup( list_next( list ) );
    list_delete( list );
    /* sort this list: directories at top and everything in alphabetical order */
    qsort( text->lines, text->count, sizeof text->lines[0], file_compare );
    /* return as dynamic list */
    list = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    /* transfer directories */
    for ( i = 0; i < text->count && text->lines[i][0] == '*'; i++ )
        list_add( list, strdup( text->lines[i] ) );
    /* insert list of extracted ordered files between directories and files */
    {
        char *str;
        list_reset( extracted );
        while ( ( str = list_next( extracted ) ) )
            list_transfer( extracted, list, str );
    }
    /* transfer files */
    for ( ; i < text->count; i++ )
        list_add( list, strdup( text->lines[i] ) );
    delete_text( text );
    if ( order ) list_delete( order );
    list_delete( extracted );
    return list;
}

/*
====================================================================
Check if a file exist.
====================================================================
*/
int file_exists( const char *path )
{
    char full_path[512];
    get_full_bmp_path( full_path, path );
    FILE *f;
    if ( f = fopen( full_path, "r" ) )
    {
        fclose(f);
        return 1;
    }
    return 0;
}

/*
====================================================================
Find full file name.
Extensions are added according to type given.
'i' - images (bmp, png, jpg)
's' - sounds (wav, ogg, mp3)
====================================================================
*/
char *search_file_name( const char *path, char type )
{
    int i = 0, found = 0;
    char pathFinal[256];
    switch (type)
    {
        case 'i':
        {
            while ( ( !found ) && ( i < extension_image_length ) )
            {
                sprintf( pathFinal, "%s.%s", path, extension_image[i] );
                if ( file_exists( pathFinal ) )
                {
                    found = 1;
                }
                i++;
            }
            break;
        }
        case 's':
        {
            while ( (!found) && i < extension_sound_length )
            {
                sprintf( pathFinal, "%s.%s", path, extension_sound[i] );
                if ( file_exists( pathFinal ) )
                {
                    found = 1;
                }
                i++;
            }
            break;
        }
    }
    char *path2 = &pathFinal[0];
    return path2;
}

#ifdef TESTFILE
int map_w, map_h;	/* shut up linker in misc.c */
void char_width() {}
void parser_get_string() {}

int main( int argc, char **argv )
{
    List *l;
    char *name;
    const char *path, *ext, *root;
    
    if ( argc != 2 && argc != 3 && argc != 4 ) {
        fprintf( stderr, "Syntax: testfile path [ext [root]]\n" ); return 1;
    }
    
    path = argv[1];
    ext = argc >= 3 ? argv[2] : 0;
    root = argc >= 4 ? argv[3] : path;
    
    l = dir_get_entries( path, root, ext );
    if ( !l ) {
        fprintf( stderr, "Error reading directory %s\n", path ); return 1;
    }
    
    list_reset(l);
    while ( ( name = list_next(l) ) )
        printf( "%s\n", name );
    return 0;
}
#endif

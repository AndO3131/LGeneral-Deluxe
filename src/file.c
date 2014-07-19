
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
#include "config.h"
#include "FPGE/pgf.h"

extern Config config;

//#define FILE_DEBUG
const int extension_image_length = 4;
const int extension_sound_length = 3;
const int extension_scenario_length = 2;
const int extension_campaign_length = 2;
const int extension_nations_length = 2;
const int extension_unit_library_length = 2;
const char *extension_image[] = { "bmp", "png", "jpg", "jpeg" };
const char *extension_sound[] = { "wav", "ogg", "mp3" };
const char *extension_scenario[] = { "lgscn", "pgscn" };
const char *extension_campaign[] = { "lgcam", "pgcam" };
const char *extension_nations[] = { "lgdndb", "ndb" };
const char *extension_unit_library[] = { "lgdudb", "udb" };

/*
====================================================================
Read all lines from file.
====================================================================
*/
List *file_read_lines( FILE *file )
{
    List *list;
    char buffer[MAX_BUFFER];

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
Remove name entry.
====================================================================
*/
static void delete_name_entry( void *ptr )
{
    Name_Entry_Type *entry = (Name_Entry_Type*)ptr;
    if ( entry->file_name ) free( entry->file_name );
    if ( entry->internal_name ) free( entry->internal_name );
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
static List *file_extract_order_list( List *files, List *orderlist, int file_type )
{
    List *extracted;
    if ( file_type == LIST_ALL )
        extracted = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    else
        extracted = list_create( LIST_AUTO_DELETE, delete_name_entry );
    char *ordered, *name;
    Name_Entry_Type *name_entry;

    if ( !orderlist ) return extracted;

    list_reset( orderlist );
    while ( ( ordered = list_next( orderlist ) ) ) {
        list_reset( files );
        if ( file_type == LIST_ALL )
        {
            while ( ( name = list_next( files ) ) )
            {
                if ( strcmp( name, ordered ) == 0 ) {
                    list_transfer( files, extracted, name );
                    break;
                }
            }
        }
        else
        {
            while ( ( name_entry = list_next( files ) ) )
            {
                if ( strcmp( name_entry->file_name, ordered ) == 0 ) {
                    list_transfer( files, extracted, name_entry );
                    break;
                }
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
List* dir_get_entries( const char *path, const char *root, int file_type, int emptyFile, int dir_only )
{
    Text *text = 0;
    int i, ext_limit, flag = 0;
    DIR *dir;
    DIR *test_dir;
    struct dirent *dirent = 0;
    List *list;
    List *order = 0;
    List *extracted;
    List *list_final;
    struct stat fstat;
    Name_Entry_Type *name_entry;
    char file_name[MAX_BUFFER], *ext[10];
    FILE *file;
    /* open this directory */
    if ( ( dir = opendir( path ) ) == 0 ) {
        fprintf( stderr, tr("get_file_list: can't open parent directory '%s'\n"), path );
        return 0;
    }
    /* determine extensions according to file_type */
    if ( file_type == LIST_ALL )
    {
        ext_limit = 0;
        /* use dynamic list to gather all valid entries */
        list = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    }
    else if ( file_type == LIST_SCENARIOS )
    {
        ext_limit = extension_scenario_length;
        for ( i = 0;i < ext_limit; i++ )
        {
            ext[i] = calloc( MAX_EXTENSION, sizeof( char ) );
            snprintf( ext[i], MAX_EXTENSION, ".%s", extension_scenario[i] );
        }
        /* use dynamic list to gather all valid entries */
        list = list_create( LIST_AUTO_DELETE, delete_name_entry );
    }
    else if ( file_type == LIST_CAMPAIGNS )
    {
        ext_limit = extension_campaign_length;
        for ( i = 0;i < ext_limit; i++ )
        {
            ext[i] = calloc( MAX_EXTENSION, sizeof( char ) );
            snprintf( ext[i], MAX_EXTENSION, ".%s", extension_campaign[i] );
        }
        /* use dynamic list to gather all valid entries */
        list = list_create( LIST_AUTO_DELETE, delete_name_entry );
    }
    text = calloc( 1, sizeof( Text ) );
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
            if ( !dir_only )
                sprintf( file_name, "*%s", dirent->d_name );
            else
                sprintf( file_name, "%s", dirent->d_name );
            if ( !( dir_only && ( STRCMP( dirent->d_name, "Default" ) != 0 ) ) )
            {
                if ( file_type == LIST_ALL )
                    list_add( list, strdup( file_name ) );
                else
                {
                    name_entry = calloc( 1, sizeof( Name_Entry_Type ) );
                    name_entry->file_name = strdup( file_name );
                    name_entry->internal_name = strdup( file_name );
                    list_add( list, name_entry );
                }
            }
        }
        else
        /* check regular file */
        if ( S_ISREG( fstat.st_mode ) && !dir_only ) {
            /* test it */
            if ( ( file = fopen( file_name, "r" ) ) == 0 ) continue;
            fclose( file );
            /* check if this file has the proper extension */
            if ( ext_limit != 0 )
            {
                /* if at least one extension fits, add this file to list */
                flag = 0;
                char internal_name[MAX_BUFFER];
                for ( i = 0; i < ext_limit; i++ )
                    if ( !STRCMP( strrchr( dirent->d_name, '.' ), ext[i] ) )
                        flag++;
                if ( flag == ext_limit )
                    continue;
                snprintf( file_name, strcspn( dirent->d_name, "." ) + 1, "%s", dirent->d_name );
                if ( strcmp( strrchr( dirent->d_name, '.' ), ".pgscn" ) == 0 )
                {
                    char temp[MAX_BUFFER];
                    search_file_name( temp, 0, file_name, config.mod_name, "Scenario", 'o' );
                    snprintf( internal_name, MAX_BUFFER, "%s", load_pgf_pgscn_info( file_name, temp, 1 ) );
                    /* add one scenario entry */
                    name_entry = calloc( 1, sizeof( Name_Entry_Type ) );
                    name_entry->file_name = strdup( file_name );
                    name_entry->internal_name = strdup( internal_name );
                    list_add( list, name_entry );
                }
                else if ( strcmp( strrchr( dirent->d_name, '.' ), ".pgcam" ) == 0 )
                {
                    char temp[MAX_BUFFER];
                    search_file_name( temp, 0, file_name, config.mod_name, "Scenario", 'c' );
                    List *campaign_entries;
                    campaign_entries = list_create( LIST_AUTO_DELETE, delete_name_entry );
                    parse_pgcam_info( campaign_entries, file_name, temp, 0 );
                    /* add several campaign entries */
                    list_reset( campaign_entries );
                    while ( ( name_entry = list_next( campaign_entries ) ) )
                    {
                        list_transfer( campaign_entries, list, name_entry );
                    }
                    list_delete( campaign_entries );
                }
                else
                {
                    snprintf( internal_name, MAX_BUFFER, "%s", file_name );
                    if ( file_type == LIST_ALL )
                        list_add( list, strdup( file_name ) );
                    else
                    {
                        name_entry = calloc( 1, sizeof( Name_Entry_Type ) );
                        name_entry->file_name = strdup( file_name );
                        name_entry->internal_name = strdup( internal_name );
                        list_add( list, name_entry );
                    }
                }
            }
            else
            {
                snprintf( file_name, MAX_PATH, "%s", dirent->d_name );
                if ( file_type == LIST_ALL )
                    list_add( list, strdup( file_name ) );
                else
                {
                    name_entry = calloc( 1, sizeof( Name_Entry_Type ) );
                    name_entry->file_name = strdup( file_name );
                    name_entry->internal_name = strdup( file_name );
                    list_add( list, name_entry );
                }
            }
        }
    }
    /* close dir */
    closedir( dir );
    /* extract ordered entries
     * Directories are never ordered as they have an asterisk prefixed
     * (this is intentional!)
     */
    extracted = file_extract_order_list( list, order, file_type );
    /* convert to static list */
    text->count = list->count;
    text->lines = malloc( list->count * sizeof( char* ));
    list_reset( list );
    for ( i = 0; i < text->count; i++ )
    {
        if ( file_type == LIST_ALL )
            text->lines[i] = strdup( list_next( list ) );
        else
        {
            name_entry = list_next( list );
            text->lines[i] = strdup( name_entry->file_name );
        }
    }
    /* sort this list: directories at top and everything in alphabetical order */
    qsort( text->lines, text->count, sizeof text->lines[0], file_compare );
    /* return as dynamic list */
    if ( file_type == LIST_ALL )
        list_final = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    else
        list_final = list_create( LIST_AUTO_DELETE, delete_name_entry );
    /* transfer directories */
    for ( i = 0; i < text->count && text->lines[i][0] == '*'; i++ )
    {
        if ( file_type == LIST_ALL )
            list_add( list_final, strdup( text->lines[i] ) );
        else
        {
            list_reset( list );
            while ( ( name_entry = list_next( list ) ) )
                if ( strcmp( name_entry->file_name, text->lines[i] ) == 0 )
                    break;
            list_transfer( list, list_final, name_entry );
        }
    }
    /* insert empty file */
    if ( emptyFile )
    {
        if ( file_type == LIST_ALL )
            list_add( list_final, strdup( tr("<empty file>") ) );
        else
        {
            name_entry = calloc( 1, sizeof( Name_Entry_Type ) );
            name_entry->file_name = strdup( tr("<empty file>") );
            name_entry->internal_name = strdup( tr("<empty file>") );
            list_add( list_final, name_entry );
        }
    }
    /* insert list of extracted ordered files between directories and files */
    if ( file_type == LIST_ALL )
    {
        char *str;
        list_reset( extracted );
        while ( ( str = list_next( extracted ) ) )
        {
            list_transfer( extracted, list_final, str );
        }
    }
    else
    {
        Name_Entry_Type *extracted_name_entry;
        list_reset( extracted );
        while ( ( extracted_name_entry = list_next( extracted ) ) )
        {
            list_transfer( extracted, list_final, extracted_name_entry );
        }
    }
    /* transfer files */
    for ( ; i < text->count; i++ )
    {
        if ( file_type == LIST_ALL )
            list_add( list_final, strdup( text->lines[i] ) );
        else
        {
            list_reset( list );
            while ( ( name_entry = list_next( list ) ) )
                if ( strcmp( name_entry->file_name, text->lines[i] ) == 0 )
                    break;
            list_transfer( list, list_final, name_entry );
        }
    }
    delete_text( text );
    if ( order ) list_delete( order );
    list_delete( extracted );
    list_delete( list );
    return list_final;
}

/*
====================================================================
Check if a file exist.
====================================================================
*/
int file_exists( char *path )
{
    char full_path[MAX_PATH];
    get_full_bmp_path( full_path, path );
    FILE *f = fopen_ic( full_path, "r" );
    if ( f )
    {
        fclose(f);
        strcpy( path, full_path );
        return 1;
    }
    return 0;
}

/*
====================================================================
Check if filename is in list.
Return Value: Position of filename else -1.
====================================================================
*/
int dir_check( List *list, char *item )
{
    int pos = -1;
    List_Entry *cur = list->head.next;
    while ( cur != &list->tail ) {
        pos++;
        if ( strcmp(cur->item, item) == 0 ) return pos;
        cur = cur->next;
    }
    return -1;
}

/*
====================================================================
Create directory if folderName doesn't exist.
====================================================================
*/
int dir_create( const char *folderName, const char *subdir )
{
    struct stat st = {0};
    char dir[MAX_PATH];
    snprintf( dir, MAX_PATH, "%s/%s/Save/%s/%s", get_gamedir(), config.mod_name, subdir, folderName );
    if (stat( dir, &st ) == -1)
    {
#ifndef WIN32
        mkdir( dir, 0777 );
#else
        mkdir( dir );
#endif
        return 1;
    }
    return 0;
}

/*
====================================================================
Find full file name.
Extensions are added according to type given and may be returned
for further use.
'i' - images (bmp, png, jpg)
's' - sounds (wav, ogg, mp3)
'o' - scenarios (lgscn, pgscn)
'c' - campaigns (lgcam, pgcam)
'n' - nations (lgdndb, ndb)
'u' - unit library (lgdudb, udb)
====================================================================
*/
int search_file_name( char *pathFinal, char *extension, const char *name, const char *modFolder, char *subFolder, const char type )
{
    int i = 0;
    char pathTemp[MAX_PATH];
    if ( !STRCMP( modFolder, "" ) )
    {
        switch (type)
        {
            case 'i':
            {
                while ( i < extension_image_length )
                {
                    snprintf( pathTemp, MAX_PATH, "%s/%s/%s.%s", modFolder, subFolder, name, extension_image[i] );
                    if ( file_exists( pathTemp ) )
                    {
                        if ( pathFinal != 0 )
                            snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                        if ( extension != 0 )
                            snprintf( extension, MAX_EXTENSION, "%s", extension_image[i] );
                        return 1;
                    }
                    i++;
                }
                break;
            }
            case 's':
            {
                while ( i < extension_sound_length )
                {
                    snprintf( pathTemp, MAX_PATH, "%s/%s/%s.%s", modFolder, subFolder, name, extension_sound[i] );
                    if ( file_exists( pathTemp ) )
                    {
                        if ( pathFinal != 0 )
                            snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                        if ( extension != 0 )
                            snprintf( extension, MAX_EXTENSION, "%s", extension_sound[i] );
                        return 1;
                    }
                    i++;
                }
                break;
            }
            case 'o':
            {
                while ( i < extension_scenario_length )
                {
                    snprintf( pathTemp, MAX_PATH, "%s/%s/%s.%s", modFolder, subFolder, name, extension_scenario[i] );
                    if ( file_exists( pathTemp ) )
                    {
                        if ( pathFinal != 0 )
                            snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                        if ( extension != 0 )
                            snprintf( extension, MAX_EXTENSION, "%s", extension_scenario[i] );
                        return 1;
                    }
                    i++;
                }
                break;
            }
            case 'c':
            {
                while ( i < extension_campaign_length )
                {
                    snprintf( pathTemp, MAX_PATH, "%s/%s/%s.%s", modFolder, subFolder, name, extension_campaign[i] );
                    if ( file_exists( pathTemp ) )
                    {
                        if ( pathFinal != 0 )
                            snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                        if ( extension != 0 )
                            snprintf( extension, MAX_EXTENSION, "%s", extension_campaign[i] );
                        return 1;
                    }
                    i++;
                }
                break;
            }
            case 'n':
            {
                while ( i < extension_nations_length )
                {
                    snprintf( pathTemp, MAX_PATH, "%s/%s/%s.%s", modFolder, subFolder, name, extension_nations[i] );
                    if ( file_exists( pathTemp ) )
                    {
                        if ( pathFinal != 0 )
                            snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                        if ( extension != 0 )
                            snprintf( extension, MAX_EXTENSION, "%s", extension_nations[i] );
                        return 1;
                    }
                    i++;
                }
                break;
            }
            case 'u':
            {
                while ( i < extension_unit_library_length )
                {
                    snprintf( pathTemp, MAX_PATH, "%s/%s/%s.%s", modFolder, subFolder, name, extension_unit_library[i] );
                    if ( file_exists( pathTemp ) )
                    {
                        if ( pathFinal != 0 )
                            snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                        if ( extension != 0 )
                            snprintf( extension, MAX_EXTENSION, "%s", extension_unit_library[i] );
                        return 1;
                    }
                    i++;
                }
                break;
            }
        }
    }
    i = 0;
    switch (type)
    {
        case 'i':
        {
            while ( i < extension_image_length )
            {
                snprintf( pathTemp, MAX_PATH, "Default/%s/%s.%s", subFolder, name, extension_image[i] );
                if ( file_exists( pathTemp ) )
                {
                    if ( pathFinal != 0 )
                        snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                    if ( extension != 0 )
                        snprintf( extension, MAX_EXTENSION, "%s", extension_image[i] );
                    return 1;
                }
                i++;
            }
            break;
        }
        case 's':
        {
            while ( i < extension_sound_length )
            {
                snprintf( pathTemp, MAX_PATH, "Default/%s/%s.%s", subFolder, name, extension_sound[i] );
                if ( file_exists( pathTemp ) )
                {
                    if ( pathFinal != 0 )
                        snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                    if ( extension != 0 )
                        snprintf( extension, MAX_EXTENSION, "%s", extension_sound[i] );
                    return 1;
                }
                i++;
            }
            break;
        }
        case 'o':
        {
            while ( i < extension_scenario_length )
            {
                snprintf( pathTemp, MAX_PATH, "Default/%s/%s.%s", subFolder, name, extension_scenario[i] );
                if ( file_exists( pathTemp ) )
                {
                    if ( pathFinal != 0 )
                        snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                    if ( extension != 0 )
                        snprintf( extension, MAX_EXTENSION, "%s", extension_scenario[i] );
                    return 1;
                }
                i++;
            }
            break;
        }
        case 'c':
        {
            while ( i < extension_campaign_length )
            {
                snprintf( pathTemp, MAX_PATH, "Default/%s/%s.%s", subFolder, name, extension_campaign[i] );
                if ( file_exists( pathTemp ) )
                {
                    if ( pathFinal != 0 )
                        snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                    if ( extension != 0 )
                        snprintf( extension, MAX_EXTENSION, "%s", extension_campaign[i] );
                    return 1;
                }
                i++;
            }
            break;
        }
        case 'n':
        {
            while ( i < extension_nations_length )
            {
                snprintf( pathTemp, MAX_PATH, "Default/%s/%s.%s", subFolder, name, extension_nations[i] );
                if ( file_exists( pathTemp ) )
                {
                    if ( pathFinal != 0 )
                        snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                    if ( extension != 0 )
                        snprintf( extension, MAX_EXTENSION, "%s", extension_nations[i] );
                    return 1;
                }
                i++;
            }
            break;
        }
        case 'u':
        {
            while ( i < extension_unit_library_length )
            {
                snprintf( pathTemp, MAX_PATH, "Default/%s/%s.%s", subFolder, name, extension_unit_library[i] );
                if ( file_exists( pathTemp ) )
                {
                    if ( pathFinal != 0 )
                        snprintf( pathFinal, MAX_PATH, "%s", pathTemp );
                    if ( extension != 0 )
                        snprintf( extension, MAX_EXTENSION, "%s", extension_unit_library[i] );
                    return 1;
                }
                i++;
            }
            break;
        }
    }
    return 0;
}

/*
====================================================================
Find file name in directories. Extension already given.
====================================================================
*/
int search_file_name_exact( char *pathFinal, char *path, char *modFolder )
{
    if ( !STRCMP( modFolder, "" ) )
    {
        snprintf( pathFinal, MAX_PATH, "%s/%s", modFolder, path );
        if ( file_exists( pathFinal ) )
            return 1;
    }
    snprintf( pathFinal, MAX_PATH, "Default/%s", path );
    if ( file_exists( pathFinal ) )
        return 1;
    return 0;
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

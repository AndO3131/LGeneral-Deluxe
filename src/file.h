/***************************************************************************
                          file.h  -  description
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


#ifndef __FILE_H
#define __FILE_H

#include <stdio.h>

enum {
    LIST_ALL = 0,
    LIST_SCENARIOS,
    LIST_CAMPAIGNS
};

/*
====================================================================
File name and folder name type.
====================================================================
*/
typedef struct {
    char *file_name;
    char *internal_name;
} Name_Entry_Type;

/*
====================================================================
Read all lines from file.
====================================================================
*/
List* file_read_lines( FILE *file );

/*
====================================================================
Return a list (autodeleted strings)
with all accessible files and directories in path
with the extension ext (if != 0). Don't show hidden files or
Makefile stuff.
The directoriers are marked with an asteriks.
====================================================================
*/
List* dir_get_entries( const char *path, const char *root, int file_type, int emptyFile, int dir_only );

/*
====================================================================
Check if filename is in list.
Return Value: Position of filename else -1.
====================================================================
*/
int dir_check( List *list, char *item );

/*
====================================================================
Create directory if folderName doesn't exist.
====================================================================
*/
int dir_create( const char *folderName, const char *subdir );

/*
====================================================================
Find full file name.
Extensions are added according to type given.
'i' - images (bmp, png, jpg)
's' - sounds (wav, ogg, mp3)
====================================================================
*/
int search_file_name( char *pathFinal, char *extension, const char *name, const char *modFolder, const char type );

/*
====================================================================
Find file name in directories. Extension already given.
====================================================================
*/
int search_file_name_exact( char *pathFinal, char *path, char *modFolder );

#endif

/***************************************************************************
                          parser.h  -  description
                             -------------------
    begin                : Sat Mar 9 2002
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


#ifndef __PARSER_H
#define __PARSER_H

#include "list.h"
#include <stdio.h>

struct CommonTreeData;

/*
====================================================================
This module provides functions to parse ASCII data from strings
and files.
Synopsis:
  groupname <begin group> entry1 .. entryX <end group>
  variable  <set> value
A group entry may either be a variable or a group (interlacing).
A variable value may either be a single token or a list of tokens
enclosed by <begin list> <end list>.
Text enclosed by ".." is counted as a single token.
====================================================================
*/

/*
====================================================================
Symbols. 
Note: These symbols are ignored when found in a token "<expression>" 
as they belong to this token then.
PARSER_GROUP_BEGIN:   <begin group>
PARSER_GROUP_END:     <end group>
PARSER_SET:           <set>
PARSER_LIST_BEGIN:    <begin list>
PARSER_LIST_END:      <end list>
PARSER_COMMENT_BEGIN: <begin comment>
PARSER_COMMENT_END:   <end comment>
PARSER_STRING:        <string>
PARSER_SYMBOLS:       List of all symbols + whitespace used to 
                      split strings and tokens.
PARSER_SKIP_SYMBOLS:  text bewteen these two symbols is handled as 
                      comment and therefore completely ignored
====================================================================
*/
typedef enum ParserToken {
  PARSER_INVALID =       -2,
  PARSER_EOI =           -1,
  PARSER_GROUP_BEGIN =   '{',
  PARSER_GROUP_END =     '}',
  PARSER_SET =           '=',
  PARSER_LIST_BEGIN =    '(',
  PARSER_LIST_END =      ')',
  PARSER_COMMENT_BEGIN = '[',
  PARSER_COMMENT_END   = ']',
  PARSER_STRING        = 256,
} ParserToken;
#define PARSER_SYMBOLS       " =(){}[]"
#define PARSER_SKIP_SYMBOLS  "[]"

/*
====================================================================
An input string is converted into a PData tree struct.
The name identifies this entry and it's the token that is searched
for when reading this entry.
Either 'values' or 'entries' is set.
If 'entries' is not NULL the PData is a group and 'entries' 
contains pointers to other groups or lists.
If 'values' is not NULL the PData is a list and 'values' contains
a list of value strings associated with 'name'.
====================================================================
*/
typedef struct PData {
    char *name;
    List *values;
    List *entries;
    struct CommonTreeData *ctd;
    int lineno:24;	/* line number this entry was parsed from */
} PData;

/*
====================================================================
This function splits a string into tokens using the characters
found in symbols as breakpoints. If the first symbol is ' ' all
whitespaces are used as breakpoints though NOT added as a token 
(thus removed from string).
====================================================================
*/
List* parser_split_string( const char *string, const char *symbols );
/*
====================================================================
This is the light version of parser_split_string which checks for
just one character and does not add this glue characters to the 
list. It's about 2% faster. Wow.
====================================================================
*/
List *parser_explode_string( const char *string, char c );

/*
====================================================================
This function reads in a whole file and converts it into a
PData tree struct. If an error occurs NULL is returned and 
parser_error is set. 'tree_name' is the name of the PData tree.
====================================================================
*/
PData* parser_read_file( const char *tree_name, const char *fname );

/*
====================================================================
This function frees a PData tree struct.
====================================================================
*/
void parser_free( PData **pdata );

/*
====================================================================
This function creates a new PData struct, inserts it into parent,
and returns it.
Does not take ownership of 'name'.
Does take ownership of 'list'.
====================================================================
*/
PData *parser_insert_new_pdata( PData *parent, const char *name, List *values );

/*
====================================================================
Returns the file name this node was parsed from.
====================================================================
*/
const char *parser_get_filename( PData *pd );

/*
====================================================================
Returns the line number this node was parsed from.
====================================================================
*/
int parser_get_linenumber( PData *pd );

/*
====================================================================
Functions to access a PData tree. 
'name' is the pass within tree 'pd' where subtrees are separated 
by '/' (e.g.: name = 'config/graphics/animations')
parser_get_pdata   : get pdata entry associated with 'name'
parser_get_entries : get list of subtrees (PData structs) in 'name'
parser_get_values  : get value list of 'name'
parser_get_value   : get a single value from value list of 'name'
parser_get_int     : get first value of 'name' converted to integer
parser_get_double  : get first value of 'name' converted to double
parser_get_string  : get first value of 'name' _duplicated_
If an error occurs result is set NULL, False is returned and
parse_error is set.
====================================================================
*/
int parser_get_pdata  ( PData *pd, const char *name, PData  **result );
int parser_get_entries( PData *pd, const char *name, List   **result );
int parser_get_values ( PData *pd, const char *name, List   **result );
int parser_get_value  ( PData *pd, const char *name, char   **result, int index );
int parser_get_int    ( PData *pd, const char *name, int     *result );
int parser_get_double ( PData *pd, const char *name, double  *result );
int parser_get_string ( PData *pd, const char *name, char   **result );

/*
====================================================================
Get first value of 'name', translated to the current language in the
context of the given domain, and return it _duplicated_.
====================================================================
*/
int parser_get_localized_string ( PData *pd, const char *name, const char *domain, char **result );

/*
====================================================================
Returns if a parser error occurred
====================================================================
*/
int parser_is_error(void);

/*
====================================================================
If an error occurred you can query the message with this function.
====================================================================
*/
char* parser_get_error( void );

#endif

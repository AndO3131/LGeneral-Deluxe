/*
 * pgf.h
 *
 *  Created on: 2010-06-07
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

#ifndef PGF_H_
#define PGF_H_

#include <stdio.h>
#include "list.h"

int load_line(FILE *inf, char *line, int isUTF16);
int read_utf16_line_convert_to_utf8(FILE *inf, char *line);
int load_pgf_pgscn(char *fname, char *fullName, int scenNumber);
char *load_pgf_pgscn_info( const char *fname, char *path, int name_only );
int parse_pgcam( const char *fname, const char *full_name, const char *info_entry_name );
char *parse_pgcam_info( List *camp_entries, const char *fname, char *path, char *info_entry_name );
char *parse_pgbrf( const char *path );

#endif /* PGF_H_ */

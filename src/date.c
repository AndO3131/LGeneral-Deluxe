/***************************************************************************
                          date.c  -  description
                             -------------------
    begin                : Sat Jan 20 2001
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "date.h"
#include "misc.h"
#include "localize.h"

/* length of month */
int month_length[] = {
    31,
    28,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31
};

/* full month names */
const char full_month_names[][10] = {
    TR_NOOP("January"),
    TR_NOOP("February"),
    TR_NOOP("March"),
    TR_NOOP("April"),
    TR_NOOP("May_long"),
    TR_NOOP("June"),
    TR_NOOP("July"),
    TR_NOOP("August"),
    TR_NOOP("September"),
    TR_NOOP("October"),
    TR_NOOP("November"),
    TR_NOOP("December")
};

/* short month names */
const char short_month_names[][4] = {
    TR_NOOP("Jan"),
    TR_NOOP("Feb"),
    TR_NOOP("Mar"),
    TR_NOOP("Apr"),
    TR_NOOP("May"),
    TR_NOOP("Jun"),
    TR_NOOP("Jul"),
    TR_NOOP("Aug"),
    TR_NOOP("Sep"),
    TR_NOOP("Oct"),
    TR_NOOP("Nov"),
    TR_NOOP("Dec")
};

/* converts a string to date */
void str_to_date( Date *date, char *str )
{
    int i;
    char aux_str[12];
    memset( aux_str, 0, sizeof( char ) * 12 );
    // day
    for ( i = 0; i < strlen( str ); i++ )
        if ( str[i] == '.' ) {
            strncpy( aux_str, str, i);
            date->day = atoi( aux_str );
            break;
        }
    str = str + i + 1;
    // month
    for ( i = 0; i < strlen( str ); i++ )
        if ( str[i] == '.' ) {
            strncpy( aux_str, str, i);
            date->month = atoi( aux_str ) - 1;
            break;
        }
    str = str + i + 1;
    // year
    date->year = atoi( str );
}

/* converts a date to a string */
void date_to_str( char *str, Date date, int type )
{
    switch ( type ) {
        case DIGIT_DATE:
            sprintf(str, "%i.%i.%i", date.day, date.month, date.year );
            break;
        case FULL_NAME_DATE:
            sprintf(str, "%i. %s %i", date.day, tr(full_month_names[date.month]), date.year );
            break;
        case SHORT_NAME_DATE:
            sprintf(str, "%i. %s %i", date.day, tr(short_month_names[date.month]), date.year );
            break;
    }
}

/* add number of days to date and adjust it */
void date_add_days( Date *date, int days )
{
    /* poor-man's day adder. Add only the lowest amount of days to ensure
     * that no more than one month wraparound occurs per pass.
     */
    for (; days > 0; days -= 28) {
        date->day += MINIMUM(days, 28);
        if ( date->day > month_length[date->month] ) {
            date->day = date->day - month_length[date->month];
            date->month++;
            if ( date->month == 13 ) {
                date->month = 1;
                date->year++;
            }
        }
    }
}


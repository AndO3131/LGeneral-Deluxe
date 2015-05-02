/***************************************************************************
                          campaign.h  -  description
                             -------------------
    begin                : Fri Apr 5 2002
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

#ifndef __CAMPAIGN_H
#define __CAMPAIGN_H

/*
====================================================================
Campaign Scenario entry.
====================================================================
*/
typedef struct {
    char *id;			/* entry id */
    char *scen;			/* scenario file name (if not set this is
				   a selection) */
    char *title;		/* title string, will be displayed at the top */
    char *brief;		/* briefing for this scenario */
    List *nexts;		/* list of strings: result>next_id
				   (if not set this is a final message) */
    List *descs;		/* textual descriptions for result: result>description */
    char *core_transfer;	/* set "allowed" if you want to replace core
				   units from this scenario, with core units
				   that survived the former one */
} Camp_Entry;

/*
====================================================================
Load campaign.
====================================================================
*/
int camp_load( const char *fname );
/*
====================================================================
Load a campaign description (newly allocated string)
and setup the setup :) except the type which is set when the 
engine performs the load action.
====================================================================
*/
char* camp_load_info( const char *fname );

void camp_delete();

/*
====================================================================
Query next campaign scenario entry by this result for the current
entry.
====================================================================
*/
Camp_Entry *camp_get_entry( const char *id );

/*
====================================================================
Return list of result identifiers for current campaign entry.
(List of char *)
====================================================================
*/
List *camp_get_result_list();

/*
====================================================================
Return textual description of result or 0 if no description.
====================================================================
*/
const char *camp_get_description(const char *result);

/*
====================================================================
Set the next scenario entry by searching the results in current
scenario entry. If 'id' is NULL entry 'first' is loaded
====================================================================
*/
int camp_set_next( const char *id );

/*
====================================================================
Set current scenario by camp scen entry id.
====================================================================
*/
void camp_set_cur( const char *id );

#endif

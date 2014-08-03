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

#define MAX_CAMPAIGN_TOKEN_LENGTH 2048

/* ascii-codes of game-related symbols */
enum CampaignState {
    NO_CAMPAIGN = 0,
    FIRST_SCENARIO = 1,
    CONTINUE_CAMPAIGN,
    RESTART_CAMPAIGN
};

/*
====================================================================
Campaign Scenario entry.
====================================================================
*/
typedef struct {
    char *id;       /* entry id */
    char *scen;     /* scenario file name (if not set this is
                       a selection) */
    char *title;    /* title string, will be displayed at the top */
    char *brief;    /* briefing for this scenario */
    List *nexts;    /* list of strings: result>next_id>prestige
                       (if not set this is a final message) */
    List *descs;    /* textual descriptions for result: result>description */
    List *prestige; /* prestige costs for result: result>prestige */
} Camp_Entry;

/*
====================================================================
Delete campaign entry.
====================================================================
*/
void camp_delete_entry( void *ptr );

/* check whether all next entries have a matching scenario entry */
void camp_verify_tree();

/*
====================================================================
Load campaign.
====================================================================
*/
int camp_load( const char *fname, const char *camp_entry );

/*
====================================================================
Load a campaign description from lgcam format.
====================================================================
*/
char* camp_load_info( char *fname, char *camp_entry );

/*
====================================================================
Load a campaign description from lgdcam format.
====================================================================
*/
char* camp_load_lgdcam_info( List *campaign_entries, const char *fname, const char *path, char *info_entry_name );

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

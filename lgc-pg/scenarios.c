/***************************************************************************
                          scenarios.c -  description
                             -------------------
    begin                : Tue Mar 12 2002
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL_endian.h>
#include "units.h"
#include "misc.h"
#include "scenarios.h"
#include "parser.h"

/*
====================================================================
Externals
====================================================================
*/
extern char *source_path;
extern char *dest_path;
extern char target_name[128];
extern int map_or_scen_files_missing;
extern int  nation_count;
extern char *nations[];
/* unofficial options */
extern const char *axis_name, *allies_name;
extern const char *axis_nations, *allies_nations;

int unit_entry_used[UDB_LIMIT];

/*
====================================================================
The scenario result is determined by a list of conditions.
If any of the conditions turn out to be true the scenario ends,
the message is displayed and result is returned (for campaign).
If there is no result after the LAST turn result and message
of the else struct are used.
A condition has two test fields: one is linked with an 
logical AND the other is linkes with an logical OR. Both
fields must return True if the condition is supposed to be
fullfilled. An empty field does always return True.
Struct:
    result {
        check = every_turn | last_turn
        cond {
               and { test .. testX }
               or  { test .. testY }
               result = ...
               message = ... }
        cond { ... }
        else { result = ... message = ... }
    }
Tests:
  control_hex { player x = ... y = ... } 
      :: control a special victory hex
  control_any_hex { player count = ... } 
      :: control this number of victory
         hexes
  control_all_hexes { player }    
      :: conquer everything important
         on the map
  turns_left { count = ... }      
      :: at least so many turns are left
====================================================================
*/
/*
====================================================================
Scenario file names
====================================================================
*/
char *fnames[] = {
    "Poland",
    "Warsaw",
    "Norway",
    "LowCountries",
    "France",
    "Sealion40",
    "NorthAfrica",
    "MiddleEast",
    "ElAlamein",
    "Caucasus",
    "Sealion43",
    "Torch",
    "Husky",
    "Anzio",
    "D-Day",
    "Anvil",
    "Ardennes",
    "Cobra",
    "MarketGarden",
    "BerlinWest",
    "Balkans",
    "Crete",
    "Barbarossa",
    "Kiev",
    "Moscow41",
    "Sevastapol",
    "Moscow42",
    "Stalingrad",
    "Kharkov",
    "Kursk",
    "Moscow43",
    "Byelorussia",
    "Budapest",
    "BerlinEast",
    "Berlin",
    "Washington",
    "EarlyMoscow",
    "SealionPlus"
};

/*
====================================================================
AI modules names
====================================================================
*/
char *ai_modules[] = {
    /* axis, allied -- scenarios as listed above */
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default",
    "default", "default"
};
/*
====================================================================
Per-turn prestige
====================================================================
*/
int prestige_per_turn[] = {
    /* axis, allied -- scenarios as listed above */
    0, 20,
    0, 20,
    0, 40,
    0, 48,
    0, 45,
    0, 84,
    0, 45,
    0, 70,
    0, 63,
    0, 85,
    0, 103,
    0, 0,
    71, 0,
    47, 0,
    70, 0,
    48, 0,
    0, 75,
    48, 0,
    61, 0,
    70, 0,
    0, 101,
    0, 45,
    0, 60,
    0, 80,
    0, 115,
    0, 63,
    0, 105,
    0, 95,
    0, 55,
    0, 115,
    0, 122,
    47, 0, 
    0, 0,
    70, 0,
    82, 0,
    0, 115,
    0, 135,
    0, 85
};

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Random generator
====================================================================
*/
int seed;
void random_seed( int _seed )
{
    seed = _seed;
}
int random_get( int low, int high )
{
    int p1 = 1103515245;
    int p2 = 12345;
    seed = ( seed * p1 + p2 ) % 2147483647;
    return ( ( abs( seed ) / 3 ) % ( high - low + 1 ) ) + low;
}

/*
====================================================================
Read all flags from MAP%i.SET and add them to dest_file.
We need the scenario file as well as the victory hex positions
are saved there.
====================================================================
*/
int scen_add_flags( FILE *dest_file, FILE *scen_file, int id )
{
    FILE *map_file;
    char path[MAXPATHLEN];
    int width, height, ibuf;
    int x, y, i, obj;
    int vic_hexes[40]; /* maximum if 20 hexes in PG - terminated with -1 */
    int obj_count = 0;
    /* read victory hexes from scen_file */
    memset( vic_hexes, 0, sizeof(int) * 40 );
    fseek( scen_file, 37, SEEK_SET );
    for ( i = 0; i < 20; i++ ) {
        fread( &vic_hexes[i * 2], 2, 1, scen_file );
        vic_hexes[i * 2] = SDL_SwapLE16(vic_hexes[i * 2]);
        fread( &vic_hexes[i * 2 + 1], 2, 1, scen_file );
        vic_hexes[i * 2 + 1] = SDL_SwapLE16(vic_hexes[i * 2 + 1]);
        if ( vic_hexes[i * 2] >= 1000 || vic_hexes[i * 2] < 0 )
            break;
        obj_count++;
    }
    /* open set file */
    snprintf( path, MAXPATHLEN, "%s/map%02i.set", source_path, id );
    if ( ( map_file = fopen_ic( path, "r" ) ) == 0 ) {
        fprintf( stderr, "%s: file not found\n", path );
        return 0;
    }
    /* read/write map size */
    width = height = 0;
    fseek( map_file, 101, SEEK_SET );
    fread( &width, 2, 1, map_file ); 
    width = SDL_SwapLE16(width);
    fseek( map_file, 103, SEEK_SET );
    fread( &height, 2, 1, map_file ); 
    height = SDL_SwapLE16(height);
    width++; height++;
    /* owner info */
    fseek( map_file, 123 + 3 * width * height, SEEK_SET );
    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            ibuf = 0; fread( &ibuf, 1, 1, map_file );
            if ( ibuf > 0 ) {
                obj = 0;
                for ( i = 0; i < obj_count; i++ )
                    if ( vic_hexes[i * 2] == x && vic_hexes[i * 2 + 1] == y ) {
                        obj = 1; break;
                    }
                fprintf( dest_file, "<flag\nx»%i\ny»%i\nnation»%s\nobj»%i\n>\n", x, y, nations[(ibuf - 1) * 3], obj );
            }
        }
    }
    return 1;
}

/*
====================================================================
Panzer General offers two values: weather condition and weather
region which are used to determine the weather throughout the
scenario. As the historical battle did only occur once the weather
may not change from game to game so we compute the weather of a 
scenario depending on three values:
inital condition, weather region, month
Initial Condition:
    1  clear
    0  rain/snow
Regions:
    0  Desert
    1  Mediterranean
    2  Northern Europe
    3  Eastern Europe
This routine does only use Fair(Dry), Overcast(Dry), Rain(Mud) and
Snow(Ice), so there is no delay between ground and air conditions.
====================================================================
*/
void scen_create_random_weather( FILE *dest_file, FILE *scen_file, int month, int turns )
{
    float month_mod[13] = { 0, 1.7, 1.6, 1.0, 2.0, 1.2, 0.7, 0.5, 0.6, 1.4, 1.7, 2.2, 1.7 };
    int med_weather[4] = { 0, 16, 24, 36 };
    int bad_weather[4] = { 0, 8, 12, 18 };
    int i, result;
    int init_cond = 0, region = 0;
    int weather[turns];
    memset( weather, 0, sizeof( int ) * turns );
    /* get condition and region */
    fseek( scen_file, 16, SEEK_SET );
    fread( &init_cond, 1, 1, scen_file );
    fread( &region, 1, 1, scen_file );
    
    /* compute the weather */
    random_seed( month * turns + ( region + 1 ) * ( init_cond + 1 ) );
    for ( i = 0; i < turns; i++ ) {
        result = random_get( 1, 100 );
        if ( result <= (int)( month_mod[month] * bad_weather[region] ) ) 
            weather[i] = 2;
        else
            if ( result <= (int)( month_mod[month] * med_weather[region] ) )
                weather[i] = 1;
    }
    
    /* initial condition */
    weather[0] = (init_cond==1)?0:2;
    /* from december to february turn 2 (rain) into 3 (snow) */
    if ( month < 3 || month == 12 ) {
        for ( i = 0; i < turns; i++ )
            if ( weather[i] == 2 )
                weather[i]++;
    }
   
    /* write weather */
    fprintf( dest_file, "weather»" );
    i = 0;
    while ( i < turns ) {
        fprintf( dest_file, "%s", weather[i]==0?"fair":weather[i]==1?"clouds":weather[i]==2?"rain":"snow" );
        if ( i < turns - 1 )
            fprintf( dest_file, "°" );
        i++;
    }
    fprintf( dest_file, "\n" );
}

/*
====================================================================
Use fixed weather obtained from a run of PG for scen_id!=-1. If -1
use old algorithm for random weather.
====================================================================
*/
void scen_create_pg_weather( FILE *dest_file, int scen_id, FILE *scen_file, int turns )
{
    /* HACK: use weather type id's directly with shortcut f=fair,o=clouds,R=rain,S=snow */
    char *weathers[] = {
        "fffffroRff",
        "ffffffffffffrooorRff",
        "fffforRRRmfffforRROffffff",
        "fffffffrROooffffffffffooroffff",
        "ffffffffffffoorfffffffffff",
        "ffffffooorfffff",
        "",
        "",
        "",
        "ffffffffffffffrooooofffsoSISSi",
        "fffffffffffffoo",
        "",
        "ffffffffooorRffffffff",
        "fffforofffffff",
        "",
        "ooofffffffforRoffffffff",
        "SISSSSSSSIISISSiffsSSSSISSSISSII",
        "ffffffffffffffrooffffffff",
        "fffffroRooffffff",
        "ffroorfffffff",
        "ffffffffffforooffffffffoo",
        "fffffffffroRR",
        "ffffffffoorofffffffffff",
        "fffffffffoorofffffffffffrooo",
        "fffffsooSSSiffffsSffff",
        "ffffffffffforofff",
        "fffooooosSfffsoSIffffoo",
        "ffffffffffffforoooffffffffffffo",
        "ffsoSSiffroooroRffffsS",
        "fffffffffffffoorofff",
        "ffffooosfffffosSSSIif",
        "ffffffffffffffrooffffff",
        "fffffoorRRRmffffosSf",
        "ffffoosofffoo",
        "fffroRofffrRM",
        "fffffffffffffrffffffff",
        "ffffffffffsooofffffoosos",
        "ffffffroforofff"
    };
    int i;
    char w[32];
    if (strlen(weathers[scen_id])>0&&strlen(weathers[scen_id])!=turns)
        fprintf(stderr,"ERROR: scen %d: mismatch in length of weather (%d) and turn number (%d)\n",
                scen_id,strlen(weathers[scen_id]),turns);
    /* write weather */
    fprintf( dest_file, "weather»" );
    i = 0;
    while ( i < turns ) {
        if (weathers[0]==0)
            strcpy(w,"fair");
        else
        {
            w[0] = weathers[scen_id][i]; w[1] = 0;
                 if (w[0]=='f') strcpy(w,"fair");
            else if (w[0]=='o') strcpy(w,"clouds");
            else if (w[0]=='R') strcpy(w,"rain");
            else if (w[0]=='S') strcpy(w,"snow");
        }
        fprintf( dest_file, "%s", w );
        if ( i < turns - 1 )
            fprintf( dest_file, "°" );
        i++;
    }
    fprintf( dest_file, "\n" );
}

/*
====================================================================
Read unit data from scen_file, convert and write it to dest_file.
====================================================================
*/
void scen_create_unit( int scen_id, FILE *dest_file, FILE *scen_file )
{
    int id = 0, nation = 0, x = 0, y = 0, str = 0, entr = 0, exp = 0, trsp_id = 0, org_trsp_id = 0;
    /* read unit -- 14 bytes */
    /* icon id */
    fread( &id, 2, 1, scen_file ); /* icon id */
    id = SDL_SwapLE16(id);
    fread( &org_trsp_id, 2, 1, scen_file ); /* transporter of organic unit */
    org_trsp_id = SDL_SwapLE16(org_trsp_id);
    fread( &nation, 1, 1, scen_file ); nation--; /* nation + 1 */
    fread( &trsp_id, 2, 1, scen_file ); /* sea/air transport */
    trsp_id = SDL_SwapLE16(trsp_id);
    fread( &x, 2, 1, scen_file ); /* x */
    x = SDL_SwapLE16(x);
    fread( &y, 2, 1, scen_file ); /* y */
    y = SDL_SwapLE16(y);
    fread( &str, 1, 1, scen_file ); /* strength */
    fread( &entr, 1, 1, scen_file ); /* entrenchment */
    fread( &exp, 1, 1, scen_file ); /* experience */
    /* FIX: give transporters to artillery in Kiev */
    if (scen_id==23)
    {
        if (x==7&&y==14) trsp_id = 86;
        if (x==8&&y==23) trsp_id = 86;
    }
    /* mark id */
    unit_entry_used[id - 1] = 1;
    if ( trsp_id ) 
        unit_entry_used[trsp_id - 1] = 1;
    else
        if ( org_trsp_id ) 
            unit_entry_used[org_trsp_id - 1] = 1;
    /* write unit */
    fprintf( dest_file, "<unit\n" );
    fprintf( dest_file, "id»%i\nnation»%s\n", id - 1, nations[nation * 3] );
    fprintf( dest_file, "x»%i\ny»%i\n", x, y );
    fprintf( dest_file, "str»%i\nentr»%i\nexp»%i\n", str, entr, exp );
    if ( trsp_id == 0 && org_trsp_id == 0 )
        fprintf( dest_file, "trsp»none\n" );
    else {
        if ( trsp_id ) 
            fprintf( dest_file, "trsp»%i\n", trsp_id - 1 );
        else
            fprintf( dest_file, "trsp»%i\n", org_trsp_id - 1 );
    }
    fprintf( dest_file, ">\n" );
}

/*
====================================================================
Add the victory conditions
====================================================================
*/
int major_limits[] = {
    /* if an entry is not -1 it's a default axis offensive 
       and this is the turn number that must remain for a major
       victory when all flags where captured */
    -1, /* UNUSED */
    3, /* POLAND */
    7, /* WARSAW */
    5, /* NORWAY */
    6, /* LOWCOUNRTIES */
    13, /* FRANCE */
    3, /* SEALION 40 */
    4, /* NORTH AFRICA */
    5, /* MIDDLE EAST */
    3, /* EL ALAMEIN */
    12, /* CAUCASUS */
    3, /* SEALION 43 */
    -1, /* TORCH */
    -1, /* HUSKY */
    -1, /* ANZIO */
    -1, /* D-DAY */
    -1, /* ANVIL */
    -1, /* ARDENNES */
    -1, /* COBRA */
    -1, /* MARKETGARDEN */
    -1, /* BERLIN WEST */
    3, /* BALKANS */
    2, /* CRETE */
    10, /* BARBAROSSA */
    8, /* KIEV */
    4, /* MOSCOW 41 */
    3, /* SEVASTAPOL */
    6, /* MOSCOW 42 */
    13, /*STALINGRAD */
    4, /* KHARKOV */
    -1, /*KURSK */
    5, /* MOSCOW 43*/
    -1, /* BYELORUSSIA */
    5, /* BUDAPEST */
    -1, /* BERLIN EAST */
    -1, /* BERLIN */
    7, /* WASHINGTON */
    5, /* EARLY MOSCOW */
    3, /* SEALION PLUS */
};
#define COND_BEGIN fprintf( file, "<cond\n" )
#define COND_END   fprintf( file, ">\n" )
#define COND_RESULT( str ) fprintf( file, "result»%s\n", str )
#define COND_MESSAGE( str ) fprintf( file, "message»%s\n", str )
void scen_add_vic_conds( FILE *file, int id )
{
    /* for panzer general the check is usually run every turn.
     * exceptions:
     *   ardennes: major/minor victory depends on whether bruessel
     *     can be taken
     *   d-day: axis must hold its initial three objectives until
     *     the end
     *  anvil: axis must hold its initial five objectives until
     *    the end 
     */
    if ( id == 15 || id == 16 || id == 17 )
        fprintf( file, "<result\ncheck»last_turn\n" );
    else
        fprintf( file, "<result\ncheck»every_turn\n" );
    /* add conditions */
    if ( major_limits[id] != -1 ) {
        COND_BEGIN;
        fprintf( file, "<and\n<control_all_hexes\nplayer»axis\n>\n<turns_left\ncount»%i\n>\n>\n", major_limits[id] );
        COND_RESULT( "major" ); 
        COND_MESSAGE( "Axis Major Victory" );
        COND_END;
        COND_BEGIN;
        fprintf( file, "<and\n<control_all_hexes\nplayer»axis\n>\n>\n" );
        COND_RESULT( "minor" ); 
        COND_MESSAGE( "Axis Minor Victory" );
        COND_END;
        fprintf( file, "<else\n" );
        COND_RESULT( "defeat" ); 
        COND_MESSAGE( "Axis Defeat" );
        COND_END;
    }
    else
    if ( id == 17 ) {
        /* ardennes is a special axis offensive */
        COND_BEGIN;
        fprintf( file, "<and\n"\
                       "<control_hex\nplayer»axis\nx»16\ny»16\n>\n"\
                       "<control_hex\nplayer»axis\nx»26\ny»4\n>\n"\
                       "<control_hex\nplayer»axis\nx»27\ny»21\n>\n"\
                       "<control_hex\nplayer»axis\nx»39\ny»21\n>\n"\
                       "<control_hex\nplayer»axis\nx»48\ny»8\n>\n"\
                       "<control_hex\nplayer»axis\nx»54\ny»14\n>\n"\
                       "<control_hex\nplayer»axis\nx»59\ny»18\n>\n"\
                       ">\n" );
        COND_RESULT( "minor" );
        COND_MESSAGE( "Axis Minor Victory" ); 
        COND_END;
        /* major victory */
        COND_BEGIN;
        fprintf( file, "<or\n<control_all_hexes\nplayer»axis\n>\n>\n" );
        COND_RESULT( "major" );
        COND_MESSAGE( "Axis Major Victory" );
        COND_END;
        /* defeat otherwise */
        fprintf( file, "<else\n" );
        COND_RESULT( "defeat" ); 
        COND_MESSAGE( "Axis Defeat" );
        COND_END;
    }
    else {
        /* allied offensives */
        COND_BEGIN;
        switch ( id ) {
            case 12: /* TORCH */
                fprintf( file, "<or\n<control_hex\nplayer»allies\nx»27\ny»5\n>\n<control_hex_num\nplayer»allies\ncount»6\n>\n>\n" );
                break;
            case 13: /* HUSKY */
                fprintf( file, "<or\n<control_hex_num\nplayer»allies\ncount»14\n>\n>\n" );
                break;
            case 14: /* ANZIO */
                fprintf( file, "<or\n<control_hex\nplayer»allies\nx»13\ny»17\n>\n<control_hex_num\nplayer»allies\ncount»5\n>\n>\n" );
                break;
            case 15: /* D-DAY */
                fprintf( file, "<or\n<control_hex_num\nplayer»allies\ncount»4\n>\n>\n" );
                break;
            case 16: /* ANVIL */
                fprintf( file, "<or\n<control_hex_num\nplayer»allies\ncount»5\n>\n>\n" );
                break;
            case 18: /* COBRA */
                fprintf( file, "<or\n<control_hex_num\nplayer»allies\ncount»5\n>\n>\n" );
                break;
            case 19: /* MARKET-GARDEN */
                fprintf( file, "<and\n<control_hex\nplayer»allies\nx»37\ny»10\n>\n>\n" );
                break;
            case 20: /* BERLIN WEST */
                fprintf( file, "<or\n<control_hex\nplayer»allies\nx»36\ny»14\n>\n<control_hex_num\nplayer»allies\ncount»5\n>\n>\n" );
                break;
            case 30: /* KURSK */
                fprintf( file, "<or\n<control_all_hexes\nplayer»allies\n>\n>\n" );
                break;
            case 32: /* BYELORUSSIA */
                fprintf( file, "<or\n<control_hex\nplayer»allies\nx»3\ny»12\n>\n>\n" );
                break;
            case 34: /* BERLIN EAST */
                fprintf( file, "<or\n<control_hex\nplayer»allies\nx»36\ny»14\n>\n<control_hex_num\nplayer»allies\ncount»8\n>\n>\n" );
                break;
            case 35: /* BERLIN */
                fprintf( file, "<or\n<control_hex\nplayer»allies\nx»36\ny»14\n>\n>\n" );
                break;
        }
        COND_RESULT( "defeat" );
        COND_MESSAGE( "Axis Defeat" );
        COND_END;
        /* axis major victory condition */
        COND_BEGIN;
        if ( id == 15 )
        {
            /* D-DAY */
            fprintf( file, "<and\n"\
                           "<control_hex\nplayer»axis\nx»11\ny»7\n>\n"\
                           "<control_hex\nplayer»axis\nx»22\ny»28\n>\n"\
                           "<control_hex\nplayer»axis\nx»43\ny»27\n>\n"\
                           ">\n" );
        }
        else if ( id == 16 )
        {
            /* ANVIL */
            fprintf( file, "<and\n"\
                           "<control_hex\nplayer»axis\nx»7\ny»3\n>\n"\
                           "<control_hex\nplayer»axis\nx»15\ny»4\n>\n"\
                           "<control_hex\nplayer»axis\nx»5\ny»22\n>\n"\
                           "<control_hex\nplayer»axis\nx»12\ny»27\n>\n"\
                           "<control_hex\nplayer»axis\nx»31\ny»21\n>\n"\
                           ">\n" );
        }
        else
        {
            /* capture all */
            fprintf( file, "<or\n<control_all_hexes\nplayer»axis\n>\n>\n" );
        }
        COND_RESULT( "major" );
        COND_MESSAGE( "Axis Major Victory" );
        COND_END;
        fprintf( file, "<else\n" );
        COND_RESULT( "minor" ); 
        COND_MESSAGE( "Axis Minor Victory" );
        COND_END;
    }
    /* end result struct */
    fprintf( file, ">\n" );
}

/** Read @num bytes into @buf from @file at position @offset. Return
 * number of bytes read or -1 on error.
 * Note: File position indicator is not reset. */
int freadat( FILE *file, int offset, unsigned char *buf, int num)
{
	if (fseek( file, offset, SEEK_SET ) < 0)
		return -1;
	return fread( buf, 1, num, file );
}

/** Read prestige info from open file handle @file and store it to @pi_axis 
 * and @pi_allies respectively. 
 * Return -1 on error (prestige info may be incomplete), 0 on success. */
struct prestige_info {
	int start;
	int bucket;
	int interval;
};
int read_prestige_info( FILE *file, struct prestige_info *pi_axis,
					struct prestige_info *pi_allies )
{
	/* Offsets in scenario buffer as taken from FPGE:
	 * axis start prestige:   scnbuf[0x75]*4+0x77
	 * allies start prestige: scnbuf[0x75]*4+0x77 + 2
	 * axis prestige bucket:  27
	 * allies prestige bucket: 29
	 * axis prestige interval: 31
	 * allies prestige interval: 32
	 */
	unsigned char c, buf[6];
	int idx;
	
	memset(pi_axis,0,sizeof(struct prestige_info));
	memset(pi_allies,0,sizeof(struct prestige_info));

	if (freadat(file, 0x75, &c, 1) < 1)
		return -1;
	idx = ((int)c) * 4 + 0x77;
	
	/* start prestige */
	if (freadat(file, idx, buf, 4) < 4)
		return -1;
	pi_axis->start = buf[0] + 256 * buf[1];
	pi_allies->start = buf[2] + 256 * buf[3];
	
	/* bucket + interval */
	if (freadat(file, 27, buf, 6) < 6)
		return -1;
	pi_axis->bucket = buf[0] + 256 * buf[1];
	pi_allies->bucket = buf[2] + 256 * buf[3];
	pi_axis->interval = buf[4];
	pi_allies->interval = buf[5];
	
	return 0;
}

/** Convert PG prestige info into fixed prestige per turn values.
 * @sid: PG scenario id (starting at 0, or -1 if custom)
 * @pid: player id (0=axis, 1=allies)
 * @pi: PG prestige info (start prestige, bucket and interval)
 * @strat: player strategy
 * @nturns: number of scenario turns
 * @ppt: pointer to prestige per turn output array
 */
void convert_prestige_info(int sid, int pid, const struct prestige_info *pi,
					int strat, int nturns, int *ppt)
{
	int i, val;
	
	/* PG seems to behave as follows: 
	 * Attacker gets start prestige and that's it. No per turn prestige
	 * is added. Defender gets constant prestige per turn and in first
	 * turn also a bonus of two times the turn prestige added to start
	 * prestige.
	 * Loosing/capturing victory objectives does not change the amount
	 * of per turn prestige for defender/attacker (on capturing fixed
	 * amount of prestige is given to player once, no loss for other
	 * player).
	 * 
	 * FIXME: Per turn value seems to be related to number of turns and
	 * bucket value, while interval does not seem to be used at all. But
	 * since I could not figure out the formula use hardcoded per turn
	 * values (as observed in PG). Try bucket/nturns as formula for 
	 * custom scenarios. */
	
	if (sid == -1)
		val = pi->bucket / nturns;
	else 
		val = prestige_per_turn[sid*2 + pid];
	
	/* Clear output array */
	memset(ppt, 0, sizeof(int) * nturns);
	
	/* Set start prestige */
	ppt[0] = pi->start;
	/* Add bonus if strategy is defensive */
	if (strat < 0)
		ppt[0] += 2 * val;
	
	/* Set remaining per turn prestige */
	if (strat <= 0) /* if both are even in strength both get prestige */
		for (i = 1; i < nturns; i++)
			ppt[i] = val;
}

/** Write prestige per turn list to @file. */
int write_prestige_info( FILE *file, int turns, int *ppt )
{
	int i;
	
	fprintf( file, "prestige»" );
	for (i = 0; i < turns; i++)
		fprintf( file, "%d%c", ppt[i],
			(i < turns - 1) ? '°' : '\n');
	return 0;
}

/** Read axis/allies max unit count from file handle @file. 
 * Return -1 on error (values incomplete then) or 0 on success. */
int read_unit_limit( FILE *file, int *axis_ulimit, int *allies_ulimit )
{
	unsigned char buf[3];
	
	*axis_ulimit = *allies_ulimit = 0;
	
	if (freadat(file, 18, buf, 3) < 3)
		return -1;
	*axis_ulimit = buf[0] + buf[1];
	*allies_ulimit = buf[2];
	return 0;
}

/** Get a decent file name from scenario title: Change to lower case,
 * remove blanks and special characters. Return as pointer to static
 * string. */
static char *scentitle_to_filename( const char *title )
{
    static char fname[32];
    int i, j;
    
    snprintf(fname, 32, "%s", title);
    for (i = 0; i < strlen(fname); i++) {
        /* adjust case (first character of word is upper case) */
        if (i == 0 || fname[i - 1] == ' ')
            fname[i] = toupper(fname[i]);
        else
            fname[i] = tolower(fname[i]);
        /* only allow alphanumerical chars */
        if ((fname[i] >= 'A' && fname[i] <= 'Z') ||
            (fname[i] >= 'a' && fname[i] <= 'z') ||
            (fname[i] >= '0' && fname[i] <= '9') ||
            fname[i] == '-' || fname[i] == '_')
            ; /* is okay, do nothing */
        else 
            fname[i] = ' ';
    }
    
    /* remove all blanks */
    for (i = 0; fname[i] != 0; i++) {
        if (fname[i] != ' ')
            continue;
        for (j = i; fname[j] != 0; j++)
            fname[j] = fname[j + 1];
        i--; /* re-check if we just shifted a blank */
    }
    
    return fname;
}




/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
If scen_id == -1 convert all scenarios found in 'source_path'.
If scen_id >= 0 convert single scenario from current working 
directory.
====================================================================
*/
int scenarios_convert( int scen_id )
{
    int i, j, start, end;
    char dummy[256];
    int  day, month, year, turns, turns_per_day, days_per_turn, ibuf;
    int  unit_offset, unit_count;
    int  axis_orient, axis_strat, allied_strat;
    int  deploy_fields_count;
    struct prestige_info pi_axis, pi_allies;
    char path[MAXPATHLEN];
    FILE *dest_file = 0, *scen_file = 0, *aux_file = 0, *scenstat_file = 0;
    PData *pd = 0, *reinf, *unit;
    int def_str, def_exp, def_entr;
    char *str;
    int axis_ulimit = 0, allies_ulimit = 0;
    char scen_title[16], scen_desc[160], scen_author[32];
	
    printf( "Scenarios...\n" );
    
    if ( scen_id == -1 ) {
        snprintf( path, MAXPATHLEN, "%s/scenarios/%s", dest_path, target_name );
        mkdir( path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );

        if (strcmp(target_name, "pg") == 0) {
            /* write out order of preferred listing */
            snprintf( path, MAXPATHLEN, "%s/scenarios/pg/.order", dest_path );
            aux_file = fopen( path, "w" );
            if ( aux_file ) {
                for (i = 0; i < sizeof fnames/sizeof fnames[0]; i++)
                    fprintf( aux_file, "%s\n", fnames[i] );
                fclose( aux_file );
            } else
                fprintf( stderr, "Could not write sort order to %s\n", path );
        }
	
        /* get the reinforcements which are used later */
        snprintf( path, MAXPATHLEN, "%s/convdata/reinf", get_gamedir() );
        if ( ( pd = parser_read_file( "reinforcements", path ) ) == 0 ) {
            fprintf( stderr, "%s\n", parser_get_error() );
            goto failure;
        }
	
    }
    
    /* set loop range */
    if ( scen_id == -1 ) {
        start = 1;
        end = 38;
    } else {
        start = end = scen_id;
    }
    
    /* open scen stat file containing scenario names and descriptions */
    if ( scen_id == -1 ) {
        snprintf( path, MAXPATHLEN, "%s/scenstat.bin", source_path );
        if ((scenstat_file = fopen_ic(path, "r")) == NULL)
            goto failure;
    }
    
    /* go */
    for ( i = start; i <=end; i++ ) {
        /* open scenario file */
        snprintf( path, MAXPATHLEN, "%s/game%03d.scn", source_path, i );
        if ((scen_file = fopen_ic(path, "r")) == NULL) {
            fprintf( stderr, "%s: file not found\n", path );
            if (scen_id == -1 && strcmp(target_name,"pg")) {
                map_or_scen_files_missing = 1;
                continue;
            } else
                goto failure;
        }
        
        /* read title and description from scenstat file for campaign */
        if ( scen_id == -1 ) {
            fseek( scenstat_file, 40 + (i - 1) * 14, SEEK_SET );
            fread( dummy, 14, 1, scenstat_file );
            snprintf( scen_title, sizeof(scen_title), "%s", dummy);
            fseek( scenstat_file, 600 + (i - 1) * 160 , SEEK_SET );
            fread( dummy, 160, 1, scenstat_file );
            snprintf( scen_desc, sizeof(scen_desc), "%s", dummy);
            if (strcmp(target_name,"pg") == 0)
                snprintf( scen_author, sizeof(scen_author), "SSI" );
            else
                snprintf( scen_author, sizeof(scen_author), "unknown" );
        } else {
            snprintf( scen_title, sizeof(scen_title), "%s", target_name );
            snprintf( scen_desc, sizeof(scen_desc), "none" );
            snprintf( scen_author, sizeof(scen_author), "unknown" );
        }
        
        /* open dest file */
        if ( scen_id == -1 ) {
            if (strcmp(target_name,"pg") == 0)
                snprintf( path, MAXPATHLEN, "%s/scenarios/pg/%s",
                                                dest_path, fnames[i - 1] );
            else {
                char fname[32];
                snprintf(fname, 32, "%s", scentitle_to_filename(scen_title));
                if (fname[0] == 0) {
                    /* bad scenario name so use a default name */
                    snprintf(fname, 32, "scn%02d", i);
                    fprintf(stderr, "Using %s as filename for scenario %d as "
                                    "title '%s' is not suitable\n", fname, i, 
                                    scen_title);
                }
                snprintf( path, MAXPATHLEN, "%s/scenarios/%s/%s",
                                            dest_path, target_name, fname);
            }
        } else
            snprintf( path, MAXPATHLEN, "%s/scenarios/%s", 
                                                    dest_path, target_name );
        if ( ( dest_file = fopen( path, "w" ) ) == 0 ) {
            fprintf( stderr, "%s: access denied\n", path );
            goto failure;
        }
        
        /* file magic */
        fprintf( dest_file, "@\n" );
        
        /* scenario name and description */
        fprintf( dest_file, "name»%s\n", scen_title );
        fprintf( dest_file, "desc»%s\n", scen_desc );
        fprintf( dest_file, "authors»%s\n", scen_author );
        
        /* date */
        fseek( scen_file, 22, SEEK_SET );
        day = 0; fread( &day, 1, 1, scen_file );
        month = 0; fread( &month, 1, 1, scen_file );
        year = 0; fread( &year, 1, 1, scen_file );
        fprintf( dest_file, "date»%02i.%02i.19%i\n", day, month, year );
        
        /* turn limit */
        fseek( scen_file, 21, SEEK_SET );
        turns = 0; fread( &turns, 1, 1, scen_file );
        fprintf( dest_file, "turns»%i\n", turns );
        fseek( scen_file, 25, SEEK_SET );
        turns_per_day = 0; fread( &turns_per_day, 1, 1, scen_file );
        fprintf( dest_file, "turns_per_day»%i\n", turns_per_day );
        days_per_turn = 0; fread( &days_per_turn, 1, 1, scen_file );
        if ( turns_per_day == 0 && days_per_turn == 0 )
            days_per_turn = 1;
        fprintf( dest_file, "days_per_turn»%i\n", days_per_turn );
        
        /* domain */
        fprintf( dest_file, "domain»pg\n" );
        /* nations */
        if ( scen_id == -1 )
            fprintf( dest_file, "nation_db»%s.ndb\n", target_name );
        else
            fprintf( dest_file, "nation_db»pg.ndb\n" );
        /* units */
        if ( scen_id == -1 )
            fprintf( dest_file, "<unit_db\nmain»%s.udb\n>\n", target_name );
        else if (!units_find_panzequp())
            fprintf( dest_file, "<unit_db\nmain»pg.udb\n>\n");
        /* if there modified units they are added to the 
           scenario file. lgeneral loads from the scenario file
           if no unit_db was specified. */
        /* map:
           a custom scenario will have the map added to the same file which
           will be checked when no map was specified.
           */
        if ( scen_id == -1 )
            fprintf( dest_file, "map»%s/map%02d\n", target_name, i );
        /* weather */
        if (scen_id == -1 && strcmp(target_name,"pg") == 0)
            scen_create_pg_weather( dest_file, i-1, scen_file, turns );
        else
            scen_create_random_weather( dest_file, scen_file, month, turns );
        /* flags */
        fprintf( dest_file, "<flags\n" );
        if ( !scen_add_flags( dest_file, scen_file, i ) ) goto failure;
        fprintf( dest_file, ">\n" );
        /* get unit offset */
        fseek( scen_file, 117, SEEK_SET );
        ibuf = 0; fread( &ibuf, 1, 1, scen_file );
        unit_offset = ibuf * 4 + 135;
        /* get prestige data for axis and allies */
        read_prestige_info(scen_file, &pi_axis, &pi_allies );
	/* get unit limits for axis and allies */
	read_unit_limit( scen_file, &axis_ulimit, &allies_ulimit );
        /* players */
        fprintf( dest_file, "<players\n" );
        /* axis */
        fseek( scen_file, 12, SEEK_SET );
        /* orientation */
        axis_orient = 0; fread( &axis_orient, 1, 1, scen_file );
        if ( axis_orient == 1 ) 
            sprintf( dummy, "right" );
        else
            sprintf( dummy, "left" );
        /* strategy: -2 (very defensive) to 2 (very aggressive) */
        fseek( scen_file, 15, SEEK_SET );
        axis_strat = 0; fread( &axis_strat, 1, 1, scen_file );
        if ( axis_strat == 0 ) 
            axis_strat = 1;
        else
            axis_strat = -1;
        /* definition */
        if (axis_name)
            fprintf( dest_file, "<axis\nname»%s\n", axis_name );
        else 
            fprintf( dest_file, "<axis\nname»Axis\n" );
        if (axis_nations) {
            char *ptr, auxstr[256]; /* commata need conversion */
            snprintf(auxstr,256,"%s",axis_nations);
            for (ptr = auxstr; *ptr != 0; ptr++)
                if (ptr[0] == ',')
                    ptr[0] = '°';
            fprintf( dest_file, "nations»%s\n", auxstr );
        } else
            fprintf( dest_file, "nations»ger°aus°it°hun°bul°rum°fin°esp\n" );
        fprintf( dest_file, "allied_players»\n" );
        fprintf( dest_file, "unit_limit»%d\n", axis_ulimit );
        fprintf( dest_file, "orientation»%s\ncontrol»human\nstrategy»%i\n", dummy, axis_strat );
	{
		int ppt[turns]; /* prestige per turn with dynamic size */
		convert_prestige_info((scen_id==-1)?(i-1):-1, 0, &pi_axis, 
							axis_strat, turns, ppt);
		write_prestige_info( dest_file, turns, ppt );
	}
        if ( scen_id == -1 && strcmp(target_name,"pg") == 0 )
            fprintf( dest_file, "ai_module»%s\n", ai_modules[i*2] );
        else
            fprintf( dest_file, "ai_module»default\n" );
        /* transporter */
        fprintf( dest_file, "<transporters\n" );
        /* air */
        fseek( scen_file, unit_offset - 8, SEEK_SET );
        ibuf = 0; fread( &ibuf, 2, 1, scen_file );
        ibuf = SDL_SwapLE16(ibuf);
        if ( ibuf )
            fprintf( dest_file, "<air\nunit»%i\ncount»50\n>\n", ibuf - 1 );
        /* sea */
        fseek( scen_file, unit_offset - 4, SEEK_SET );
        ibuf = 0; fread( &ibuf, 2, 1, scen_file );
        ibuf = SDL_SwapLE16(ibuf);
        if ( ibuf )
            fprintf( dest_file, "<sea\nunit»%i\ncount»50\n>\n", ibuf - 1 );
        fprintf( dest_file, ">\n" );
        fprintf( dest_file, ">\n" );
        /* allies */
        if ( axis_orient == 1 )
            sprintf( dummy, "left" );
        else
            sprintf( dummy, "right" );
        if ( axis_strat == 1 )
            allied_strat = -1;
        else
            allied_strat = 1;
        if (allies_name)
            fprintf( dest_file, "<allies\nname»%s\n", allies_name );
        else 
            fprintf( dest_file, "<allies\nname»Allies\n" );
        if (allies_nations) {
            char *ptr, auxstr[256]; /* commata need conversion */
            snprintf(auxstr,256,"%s",allies_nations);
            for (ptr = auxstr; *ptr != 0; ptr++)
                if (ptr[0] == ',')
                    ptr[0] = '°';
            fprintf( dest_file, "nations»%s\n", auxstr );
        } else
            fprintf( dest_file, "nations»bel°lux°den°fra°gre°usa°tur°net°nor°pol°por°so°swe°swi°eng°yug\n" );
        fprintf( dest_file, "allied_players»\n" );
        fprintf( dest_file, "unit_limit»%d\n", allies_ulimit );
        fprintf( dest_file, "orientation»%s\ncontrol»cpu\nstrategy»%i\n", dummy, allied_strat );
	{
		int ppt[turns]; /* prestige per turn with dynamic size */
		convert_prestige_info((scen_id==-1)?(i-1):-1, 1, &pi_allies, 
						allied_strat, turns, ppt);
		write_prestige_info( dest_file, turns, ppt );
	}
        if ( scen_id == -1 && strcmp(target_name,"pg") == 0 )
            fprintf( dest_file, "ai_module»%s\n", ai_modules[i*2 + 1] );
        else
            fprintf( dest_file, "ai_module»default\n" );
        /* transporter */
        fprintf( dest_file, "<transporters\n" );
        /* air */
        fseek( scen_file, unit_offset - 6, SEEK_SET );
        ibuf = 0; fread( &ibuf, 2, 1, scen_file );
        ibuf = SDL_SwapLE16(ibuf);
        if ( ibuf )
            fprintf( dest_file, "<air\nunit»%i\ncount»50\n>\n", ibuf - 1 );
        /* sea */
        fseek( scen_file, unit_offset - 2, SEEK_SET );
        ibuf = 0; fread( &ibuf, 2, 1, scen_file );
        ibuf = SDL_SwapLE16(ibuf);
        if ( ibuf )
            fprintf( dest_file, "<sea\nunit»%i\ncount»50\n>\n", ibuf - 1 );
        fprintf( dest_file, ">\n" );
        fprintf( dest_file, ">\n" );
        fprintf( dest_file, ">\n" );
        /* victory conditions */
        if ( scen_id == -1 && strcmp(target_name,"pg") == 0 )
            scen_add_vic_conds( dest_file, i );
        else {
            /* and the default is that the attacker must capture
               all targets */
            fprintf( dest_file, "<result\ncheck»every_turn\n" );
            fprintf( dest_file, "<cond\n" );
            fprintf( dest_file, "<and\n<control_all_hexes\nplayer»%s\n>\n>\n",
                     (axis_strat > 0) ? "axis" : "allies" );
            fprintf( dest_file, "result»victory\n" );
            fprintf( dest_file, "message»%s\n", 
                     (axis_strat > 0) ? "Axis Victory" : "Allied Victory" );
            fprintf( dest_file, ">\n" );
            fprintf( dest_file, "<else\n" );
            fprintf( dest_file, "result»defeat\n" );
            fprintf( dest_file, "message»%s\n", 
                     (axis_strat > 0) ? "Axis Defeat" : "Allied Defeat" );
            fprintf( dest_file, ">\n" );
            fprintf( dest_file, ">\n" );
        }
        /* deployment fields */
        fseek( scen_file, 117, SEEK_SET );
        ibuf = 0; fread( &ibuf, 2, 1, scen_file );
        deploy_fields_count = SDL_SwapLE16(ibuf);
        fprintf( dest_file, "<deployfields\n<player\nid»axis\ncoordinates»default°" );
        /* last coordinate is always (-1, -1) */
        for (j = 0; j < deploy_fields_count - 1; j++) {
            int x, y;
            ibuf = 0; fread( &ibuf, 2, 1, scen_file );
            x = SDL_SwapLE16(ibuf);
            ibuf = 0; fread( &ibuf, 2, 1, scen_file );
            y = SDL_SwapLE16(ibuf);
            fprintf( dest_file, "%s%d,%d", j ? "°" : "", x, y );
        }
        fprintf( dest_file, "\n>\n" );
        if (scen_id == -1 && strcmp(target_name,"pg") == 0 && i == 19)
            /* MarketGarden's Allies may not deploy freely */
            fprintf( dest_file, "<player\nid»allies\ncoordinates»none\n>\n" );
        else
            fprintf( dest_file, "<player\nid»allies\ncoordinates»default\n>\n" );
        fprintf( dest_file, ">\n" );
        /* units */
        /* mark all id's that will be used from PANZEQUP.EQP
           for modified unit database */
        memset( unit_entry_used, 0, sizeof( unit_entry_used ) );
        /* count them */
        fseek( scen_file, 33, SEEK_SET );
        ibuf = 0; fread( &ibuf, 1, 1, scen_file );
        unit_count = ibuf; /* core */
        ibuf = 0; fread( &ibuf, 1, 1, scen_file );
        unit_count += ibuf; /* allies */
        ibuf = 0; fread( &ibuf, 1, 1, scen_file );
        unit_count += ibuf; /* auxiliary */
        /* build them */
        fseek( scen_file, unit_offset, SEEK_SET );
        fprintf( dest_file, "<units\n" );
        for ( j = 0; j < unit_count; j++ )
            scen_create_unit( (scen_id!=-1)?(-1):(i-1), dest_file, scen_file );
        /* reinforcements -- only for original PG scenarios */
        if ( scen_id == -1 && strcmp(target_name,"pg") == 0 ) {
            if ( parser_get_pdata( pd, fnames[i - 1], &reinf ) ) {
                /* units come unsorted with the proper nation stored for each
                   unit */
                def_str = 10; def_exp = 0; def_entr = 0;
                list_reset( reinf->entries );
                while ( ( unit = list_next( reinf->entries ) ) )
                    if ( !strcmp( "unit", unit->name ) ) {
                        /* add unit */
                        fprintf( dest_file, "<unit\n" );
                        if ( !parser_get_value( unit, "nation", &str, 0 ) )
                            goto failure;
                        fprintf( dest_file, "nation»%s\n", str );
                        if ( !parser_get_int( unit, "id", &ibuf ) )
                            goto failure;
                        fprintf( dest_file, "id»%i\n", ibuf );
                        ibuf = def_str;  parser_get_int( unit, "str", &ibuf );
                        fprintf( dest_file, "str»%i\n", ibuf );
                        ibuf = def_exp;  parser_get_int( unit, "exp", &ibuf );
                        fprintf( dest_file, "exp»%i\n", ibuf );
                        fprintf( dest_file, "entr»0\n" );
                        if ( parser_get_value( unit, "trsp", &str, 0 ) )
                            fprintf( dest_file, "trsp»%s\n", str );
                        if ( !parser_get_int( unit, "delay", &ibuf ) )
                            goto failure;
                        fprintf( dest_file, "delay»%i\n", ibuf );
                        fprintf( dest_file, ">\n" );
                    }
            }
        }
        fprintf( dest_file, ">\n" );
        fclose( scen_file );
        fclose( dest_file );
    }
    if ( scenstat_file ) 
        fclose( scenstat_file );
    parser_free( &pd );
    return 1;
failure:
    parser_free( &pd );
    if ( scenstat_file ) fclose( scenstat_file );
    if ( aux_file ) fclose( aux_file );
    if ( scen_file ) fclose( scen_file );
    if ( dest_file ) fclose( dest_file );
    return 0;
}

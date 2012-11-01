/***************************************************************************
                          units.c -  description
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
#include "shp.h"
#include "units.h"
#include "misc.h"

/*
====================================================================
Externals
====================================================================
*/
extern char *source_path;
extern char *dest_path;
extern char target_name[128];
extern int single_scen;
extern int apply_unit_mods;
extern int unit_entry_used[UDB_LIMIT];
extern int nation_count;
extern char *nations[];

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Write a line to file.
====================================================================
*/
#define WRITE( file, line ) fprintf( file, "%s\n", line )
#define DWRITE( line ) fprintf( file, "%s\n", line )

/*
====================================================================
Icon indices that must be mirrored terminated by -1
====================================================================
*/
int mirror_ids[] = {
    83, 84, 85, 86, 87, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    102, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
    115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
    127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
    139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
    151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162,
    163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
    175, 176, 177, 178, 179, 180, 181 ,182, 183, 184, 185, 186, 
    187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198,
    199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 221,
    232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
    244, 250, -1
};


/*
====================================================================
Unit entries are saved to this struct.
====================================================================
*/
typedef struct {     
    char name[20];   
    int  class;
    int  atk_soft;   
    int  atk_hard;   
    int  atk_air;    
    int  atk_naval;  
    int  def_ground; 
    int  def_air;    
    int  def_close;  
    int  target_type;
    int  aaf;        /* air attack flag */
    int  init;        
    int  range;      
    int  spot;       
    int  agf;        /* air ground flag */
    int  move_type;  
    int  move;       
    int  fuel;       
    int  ammo;       
    int  cost;       
    int  pic_id;    
    int  month;      
    int  year;       
    int  last_year; 
    int  nation;
} PG_UnitEntry;
/*
====================================================================
Panzer General Definitions.
====================================================================
*/
#define TARGET_TYPE_COUNT 4
char *target_types[] = { 
    "soft",  "Soft", 
    "hard",  "Hard", 
    "air",   "Air", 
    "naval", "Naval" };
enum { 
    INFANTRY = 0,
    TANK, RECON, ANTI_TANK,
    ARTILLERY, ANTI_AIRCRAFT,
    AIR_DEFENSE, FORT, FIGHTER,
    TACBOMBER, LEVBOMBER, SUBMARINE,
    DESTROYER, CAPITAL, CARRIER, 
    LAND_TRANS, AIR_TRANS, SEA_TRANS,
    UNIT_CLASS_COUNT
};
char *unit_classes[] = {
    "inf",      "Infantry",         "infantry",                         
    "tank",     "Tank",             "low_entr_rate°tank",                             
    "recon",    "Recon",            "recon°tank",                            
    "antitank", "Anti-Tank",        "anti_tank",                             
    "art",      "Artillery",        "artillery°suppr_fire°attack_first",                        
    "antiair",  "Anti-Aircraft",    "low_entr_rate",
    "airdef",   "Air-Defense",      "air_defense°attack_first",                      
    "fort",     "Fortification",    "low_entr_rate°suppr_fire",                             
    "fighter",  "Fighter",          "interceptor°carrier_ok°flying",    
    "tacbomb",  "Tactical Bomber",  "bomber°carrier_ok°flying",         
    "levbomb",  "Level Bomber",     "flying°suppr_fire°turn_suppr",                           
    "sub",      "Submarine",        "swimming°diving",                  
    "dest",     "Destroyer",        "destroyer°swimming°suppr_fire",               
    "cap",      "Capital Ship",     "swimming°suppr_fire",                         
    "carrier",  "Aircraft Carrier", "carrier°swimming",                 
    "landtrp",  "Land Transport",   "transporter",                      
    "airtrp",   "Air Transport",    "transporter°flying",               
    "seatrp",   "Sea Transport",    "transporter°swimming",             
};
int move_type_count = 8;
char *move_types[] = {
    "tracked",     "Tracked",       "pg/tracked.wav",
    "halftracked", "Halftracked",   "pg/tracked.wav",
    "wheeled",     "Wheeled",       "pg/wheeled.wav",
    "leg",         "Leg",           "pg/leg.wav",
    "towed",       "Towed",         "pg/leg.wav",
    "air",         "Air",           "pg/air.wav",
    "naval",       "Naval",         "pg/sea.wav",
    "allterrain",  "All Terrain",   "pg/wheeled.wav"
};
/*
====================================================================
Additional flags for special units.
====================================================================
*/
char *add_flags[] = {
    "6",   "jet",
    "7",   "jet",
    "9",   "jet",
    "47",  "ignore_entr",
    "108", "parachute",
    "109", "parachute",
    "168", "jet",
    "214", "bridge_eng°ignore_entr",
    "215", "parachute",
    "226", "parachute",
    "270", "parachute",
    "275", "bridge_eng°ignore_entr",
    "329", "bridge_eng°ignore_entr",
    "382", "parachute",
    "383", "parachute",
    "385", "ignore_entr",
    "386", "ignore_entr",
    "387", "bridge_eng°ignore_entr",
    "415", "parachute",
    "X"
};

/** Try to figure out nation from unit entry name. No field in PG
 * unit entry seems to hold the nation index. Bytes 30, 39, 40, 43
 * and 45 are just the same for all entries. 44 (what means ANI???)
 * seems to group certain units somehow according to class but it is
 * very jumpy and certainly not the nation id. Byte 49 varies
 * less but seems to have some other meaning, too. Pictures are also
 * not very sorted, so trying the unit name seems to be the easiest
 * approach for now:
 * Captions of non-german units start with 2-3 capital letters
 * indicating either the nation (GB, US, IT, ...) or allied usage 
 * (AF,AD,...). Check for the "big" nation ids and map to
 * nation number. Generic allied units will be used by different 
 * nations in scenarios but are not available for purchase (as it 
 * seems that they equal some unit of the major nations).
 * Return index in global nations or -1 if no match. */
static int get_nation_from_unit_name( const char *uname )
{
	struct {
		const char *id; /* first term in name */
		int idx; /* index in global nations */
	} id_map[] = {
		{ "PO ", 15 },
		{ "GB ", 22 },
		{ "FR ", 6 },
		{ "NOR ", 14 },
		{ "LC ", 1 }, /* assign low country units to belgia */
		{ "ST ", 19 },
		{ "IT ", 12 },
		{ "Bulgarian ", 2 },
		{ "Hungarian ", 10 },
		{ "Rumanian ", 17 },
		{ "Greek ", 8 },
		{ "Yugoslav ", 23 },
		{ "US ", 9 },
        { "Finn", 5}, /* codes for Finnland ... */
        { "FIN", 5}, /* ... not used in pg but some other campaigns */
		{ "AF ", -1 },
		{ "AD ", -1 },
		{ "FFR ", 6 },
		{ "FPO ", 15 },
		{ "Katyusha ", 19 }, /* here ST is missing */
		{ "x", }
	};
	int nation_idx = 7; /* germany */
	int i = 0;
	
    /* only guess for PG unit database */
    if ( !apply_unit_mods )
        return -1;
    
	while (id_map[i].id[0] != 'x') {
		if (strncmp(id_map[i].id, uname, strlen(id_map[i].id)) == 0) {
			nation_idx = id_map[i].idx;
			break;
		}
		i++;
	}
	
	return nation_idx;
}

/*
====================================================================
Load PG unit entry from file position.
DOS entry format (50 bytes):
 NAME   0
 CLASS 20
 SA    21
 HA    22
 AA    23
 NA    24
 GD    25
 AD    26
 CD    27
 TT    28
 AAF   29
 ???   30
 INI   31
 RNG   32
 SPT   33
 GAF   34    
 MOV_TYPE 35
 MOV   36
 FUEL  37
 AMMO  38
 ???   39
 ???   40
 COST  41    
 BMP   42
 ???   43
 ANI   44
 ???   45
 MON   46
 YR    47
 LAST_YEAR 48 
 ???   49
 ====================================================================
*/
static int units_read_entry( FILE *file, PG_UnitEntry *entry )
{
    unsigned char dummy[8];
    if ( feof( file ) ) return 0;
    fread(  entry->name,       1, 20, file );
    fread( &entry->class,      1, 1,  file );
    fread( &entry->atk_soft,   1, 1,  file );
    fread( &entry->atk_hard,   1, 1,  file );
    fread( &entry->atk_air,    1, 1,  file );
    fread( &entry->atk_naval,  1, 1,  file );
    fread( &entry->def_ground, 1, 1,  file );
    fread( &entry->def_air,    1, 1,  file );
    fread( &entry->def_close,  1, 1,  file );
    fread( &entry->target_type,1, 1,  file );
    fread( &entry->aaf,        1, 1,  file );
    fread( &dummy[0],          1, 1,  file );
    fread( &entry->init,       1, 1,  file );
    fread( &entry->range,      1, 1,  file );
    fread( &entry->spot,       1, 1,  file );
    fread( &entry->agf,        1, 1,  file );
    fread( &entry->move_type,  1, 1,  file );
    fread( &entry->move,       1, 1,  file );
    fread( &entry->fuel,       1, 1,  file );
    fread( &entry->ammo,       1, 1,  file );
    fread( &dummy[1],          2, 1,  file );
    fread( &entry->cost,       1, 1,  file );
    entry->cost = (entry->cost * 120) / 10;
    fread( &entry->pic_id,     1, 1,  file );
    fread( &dummy[3],          3, 1,  file );
    fread( &entry->month,      1, 1,  file );
    fread( &entry->year,       1, 1,  file );
    fread( &entry->last_year,  1, 1,  file );
    fread( &dummy[6],          1, 1,  file );
    entry->nation = get_nation_from_unit_name( entry->name );
    
    return 1;
}

/*
====================================================================
Replace a " with Inches and return the new string in buf.
====================================================================
*/
void string_replace_quote( char *source, char *buf )
{
    int i;
    int length = strlen( source );
    for ( i = 0; i < length; i++ )
        if ( source[i] == '"' ) {
            source[i] = 0;
            sprintf( buf, "%s Inches%s", source, source + i + 1 );
            return;
        }
    strcpy( buf, source );
}

/*
====================================================================
Fix spelling mistakes in unit names.
====================================================================
*/
static void fix_spelling_mistakes( char *name )
{
    /* replace "Jadg" with "Jagd" in unit name.
       Btw, this reminds me of a "Schinzel Wiener" in some Greek menu ;-) */
    char *pos = strstr( name, "Jadg" );
    if ( pos ) {
      printf("%s: ", name);
      memcpy( pos, "Jagd", 4 );
      printf("correct spelling to %s\n", name);
    }
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Check if 'source_path' contains a file PANZEQUP.EQP
====================================================================
*/
int units_find_panzequp()
{
    FILE *file;
    char path[MAXPATHLEN];
    
    snprintf( path, MAXPATHLEN, "%s/panzequp.eqp", source_path );
    if ( ( file = fopen_ic( path, "r" ) ) ) {
        fclose( file );
        return 1;
    }
    return 0;
}

/*
====================================================================
Check if 'source_path' contains a file TACICONS.SHP
====================================================================
*/
int units_find_tacicons()
{
    FILE *file;
    char path[MAXPATHLEN];
    
    snprintf( path, MAXPATHLEN, "%s/tacicons.shp", source_path );
    if ( ( file = fopen_ic( path, "r" ) ) ) {
        fclose( file );
        return 1;
    }
    return 0;
}

/*
====================================================================
Write unitclasses, target types, movement types to file.
====================================================================
*/
void units_write_classes( FILE *file )
{
    int i;
    fprintf( file, "<target_types\n" );
    for ( i = 0; i < TARGET_TYPE_COUNT; i++ )
        fprintf( file, "<%s\nname»%s\n>\n", target_types[i * 2], target_types[i * 2 + 1] );
    fprintf( file, ">\n" );
    fprintf( file, "<move_types\n" );
    for ( i = 0; i < move_type_count; i++ )
        fprintf( file, "<%s\nname»%s\nsound»%s\n>\n", 
                 move_types[i * 3], move_types[i * 3 + 1], move_types[i * 3 + 2] );
    fprintf( file, ">\n" );

	/* write unit classes, add purchase info */
	fprintf( file, "<unit_classes\n" );
	for ( i = 0; i < UNIT_CLASS_COUNT; i++ ) {
		fprintf( file, "<%s\n", unit_classes[i * 3] );
		fprintf( file, "name»%s\n", unit_classes[i * 3 + 1] );
		if (i == LAND_TRANS)
			fprintf( file, "purchase»trsp\n" );
		else if (i == FORT || i == SUBMARINE || i == DESTROYER || 
					i == CAPITAL || i == CARRIER ||	
					i == AIR_TRANS || i == SEA_TRANS)
			fprintf( file, "purchase»none\n" );
		else
			fprintf( file, "purchase»normal\n" );
		fprintf( file, ">\n" );
	}
	fprintf( file, ">\n" );
}

/*
====================================================================
Convert unit database either as a new file for a campaign or 
appended to single scenario.
'tac_icons' is the file name of the tactical icons.
====================================================================
*/
int units_convert_database( char *tac_icons )
{
    int id = 0, ini_bonus;
    short entry_count;
    char path[MAXPATHLEN];
    char flags[512];
    char buf[256];
    int i;
    PG_UnitEntry entry;
    FILE *source_file = 0, *dest_file = 0;
    
    printf( "Unit data base...\n" );
    
    /* open dest file */
    if ( single_scen )
        snprintf( path, MAXPATHLEN, "%s/scenarios/%s", dest_path, target_name );
    else
        snprintf( path, MAXPATHLEN, "%s/units/%s.udb", dest_path, target_name );
    if ( ( dest_file = fopen( path, single_scen?"a":"w" ) ) == 0 ) {
        fprintf( stderr, "%s: write access denied\n", path );
        goto failure;
    }
    
    /* open file 'panzequp.eqp' */
    snprintf( path, MAXPATHLEN, "%s/panzequp.eqp", source_path );
    if ( ( source_file = fopen_ic( path, "r" ) ) == NULL ) {
        fprintf( stderr, "%s: can't open file\n", path );
        goto failure;
    }
    
    /* DOS format:
     * count ( 2 bytes )
     * entries ( 50 bytes each ) 
     */
    fread( &entry_count, 2, 1, source_file );
    entry_count = SDL_SwapLE16(entry_count);
    if ( !single_scen )
        fprintf( dest_file, "@\n" ); /* only a new file needs this magic */
    /* domain */
    fprintf( dest_file, "domain»pg\n" );
    fprintf( dest_file, "icons»%s\nicon_type»%s\n", tac_icons,
                                        apply_unit_mods?"single":"fixed");
    fprintf( dest_file, "strength_icons»pg_strength.bmp\n" );
    fprintf( dest_file, "strength_icon_width»16\nstrength_icon_height»12\n" );
    fprintf( dest_file, "attack_icon»pg_attack.bmp\n" );
    fprintf( dest_file, "move_icon»pg_move.bmp\n" );
    fprintf( dest_file, "guard_icon»pg_guard.bmp\n" );
    units_write_classes( dest_file );
    fprintf( dest_file, "<unit_lib\n" );
    /* first entry is RESERVED */
    entry_count--;
    units_read_entry( source_file, &entry ); 
    /* convert */
    while ( entry_count-- > 0 ) {
        memset( &entry, 0, sizeof( PG_UnitEntry ) );
        if ( !units_read_entry( source_file, &entry ) ) {
            fprintf( stderr, "%s: unexpected end of file\n", path );
            goto failure;
        }
        /* sometimes a unit class seems to be screwed */
        if ( entry.class >= UNIT_CLASS_COUNT )
            entry.class = 0;
        /* adjust attack values according to unit class (add - for defense only) */
        switch ( entry.class ) {
            case INFANTRY:
            case TANK:
            case RECON:
            case ANTI_TANK:
            case ARTILLERY:
	        case FORT:
            case SUBMARINE:
            case DESTROYER:
            case CAPITAL:
	        case CARRIER:  
                entry.atk_air = -entry.atk_air;
                break;
            case AIR_DEFENSE:
                entry.atk_soft = -entry.atk_soft;
                entry.atk_hard = -entry.atk_hard;
                entry.atk_naval = -entry.atk_naval;
                break;
            case TACBOMBER:
            case LEVBOMBER:
                if ( entry.aaf )
                    entry.atk_air = -entry.atk_air;
                break;
        }
        /* all russian tanks get an initiative bonus */
        if ( apply_unit_mods ) {
            ini_bonus = 2;
            if ( entry.class == 1 && strncmp( entry.name, "ST ", 3 ) == 0 ) 
            {
                entry.init += ini_bonus;
                printf( "%s: initiative bonus +%i\n", entry.name, ini_bonus );
            }
        }
        /* get flags */
        sprintf( flags, "%s", unit_classes[entry.class * 3 + 2] );
        if ( apply_unit_mods ) {
            i = 0;
            while ( add_flags[i*2][0] != 'X' ) {
                if ( atoi( add_flags[i * 2] ) == id ) {
                    strcat( flags, "°" );
                    strcat( flags, add_flags[i * 2 + 1] );
                    i = -1;
                    break;
                }
                i++;
            }
        }
	/* whatever is legged or towed may use ground/air transporter */
        if ( entry.move > 0 && (entry.move_type == 3 || entry.move_type == 4) )
            strcat( flags, "°air_trsp_ok°ground_trsp_ok" );
        /* all artillery with range 1 has no attack_first */
        if (entry.class==4&&entry.range==1)
        {
            sprintf( flags, "artillery°suppr_fire" );
            printf( "%s: overwrite flags to: artillery,suppr_fire\n",
                                                                entry.name);
        }
        /* write entry */
        fprintf( dest_file, "<%i\n", id++ );
        string_replace_quote( entry.name, buf );
        if ( apply_unit_mods )
            fix_spelling_mistakes( buf );
        fprintf( dest_file, "name»%s\n", buf );
        fprintf( dest_file, "nation»%s\n", (entry.nation==-1)?"none":
						nations[entry.nation * 3] );
        fprintf( dest_file, "class»%s\n", unit_classes[entry.class * 3] );
        fprintf( dest_file, "target_type»%s\n", target_types[entry.target_type * 2] );
        fprintf( dest_file, "initiative»%i\n", entry.init );
        fprintf( dest_file, "spotting»%i\n", entry.spot );
        fprintf( dest_file, "movement»%i\n", entry.move );
        fprintf( dest_file, "move_type»%s\n", move_types[entry.move_type * 3] );
        fprintf( dest_file, "fuel»%i\n", entry.fuel );
        fprintf( dest_file, "range»%i\n", entry.range );
        fprintf( dest_file, "ammo»%i\n", entry.ammo );
        fprintf( dest_file, "<attack\ncount»1\nsoft»%i\nhard»%i\nair»%i\nnaval»%i\n>\n",
                 entry.atk_soft, entry.atk_hard, entry.atk_air, entry.atk_naval );
        fprintf( dest_file, "def_ground»%i\n", entry.def_ground );
        fprintf( dest_file, "def_air»%i\n", entry.def_air );
        fprintf( dest_file, "def_close»%i\n", entry.def_close );
        fprintf( dest_file, "flags»%s\n", flags );
        fprintf( dest_file, "icon_id»%i\n", entry.pic_id );
        if ( strstr( flags, "°jet" ) )
            fprintf( dest_file, "move_sound»%s\n", "pg/air2.wav" );
        fprintf( dest_file, "cost»%i\n", entry.cost );
        fprintf( dest_file, "start_year»19%i\n", entry.year );
        fprintf( dest_file, "start_month»%i\n", entry.month );
        fprintf( dest_file, "last_year»19%i\n", entry.last_year );
        fprintf( dest_file, ">\n" );
    }
    fprintf( dest_file, ">\n" );
    fclose( source_file );
    fclose( dest_file );
    return 1;
failure:
    if ( source_file ) fclose( source_file );
    if ( dest_file ) fclose( dest_file );
    return 0;
}

/*
====================================================================
Convert unit graphics.
'tac_icons' is file name of the tactical icons.
====================================================================
*/
int units_convert_graphics( char *tac_icons )
{
    int mirror;
    char path[MAXPATHLEN], path2[MAXPATHLEN];
    int i, height = 0, j;
    SDL_Rect srect, drect;
    PG_Shp *shp = 0;
    SDL_Surface *surf = 0;
    Uint32 ckey = MAPRGB( CKEY_RED, CKEY_GREEN, CKEY_BLUE ); /* transparency key */
    Uint32 mkey = MAPRGB( 0x0, 0xc3, 0xff ); /* size measurement key */
    
    printf( "Unit graphics...\n" );
    /* load tac icons */
    if ( ( shp = shp_load( "TACICONS.SHP" ) ) == 0 ) return 0;
    /* create new surface */
    for ( i = 0; i < shp->count; i++ )
        if ( shp->headers[i].valid )
            height += shp->headers[i].actual_height + 2;
        else
            height += 10;
    surf = SDL_CreateRGBSurface( SDL_SWSURFACE, shp->surf->w, height, shp->surf->format->BitsPerPixel,
                                 shp->surf->format->Rmask, shp->surf->format->Gmask, shp->surf->format->Bmask,
                                 shp->surf->format->Amask );
    if ( surf == 0 ) {
        fprintf( stderr, "error creating surface: %s\n", SDL_GetError() );
        goto failure;
    }
    SDL_FillRect( surf, 0, ckey );
    height = 0;
    for ( i = 0; i < shp->count; i++ ) {
        if ( !shp->headers[i].valid ) {
            set_pixel( surf, 0, height, mkey );
            set_pixel( surf, 9, height, mkey );
            set_pixel( surf, 0, height + 9, mkey );
            height += 10;
            continue;
        }
        srect.x = shp->headers[i].x1;
        srect.w = shp->headers[i].actual_width;
        srect.y = shp->headers[i].y1 + shp->offsets[i];
        srect.h = shp->headers[i].actual_height;
        drect.x = 0;
        drect.w = shp->headers[i].actual_width;
        drect.y = height + 1;
        drect.h = shp->headers[i].actual_height;
        mirror = 0;
        if ( apply_unit_mods ) {
            j = 0;
            while ( mirror_ids[j] != -1 ) {
                if ( mirror_ids[j] == i )
                    mirror = 1;
                j++;
            }
        }
        if ( mirror )
            copy_surf_mirrored( shp->surf, &srect, surf, &drect );
        else
            SDL_BlitSurface( shp->surf, &srect, surf, &drect );
        set_pixel( surf, drect.x, drect.y - 1, mkey );
        set_pixel( surf, drect.x + ( drect.w - 1 ), drect.y - 1, mkey );
        set_pixel( surf, drect.x, drect.y + drect.h, mkey );
        height += shp->headers[i].actual_height + 2;
    }
    snprintf( path, MAXPATHLEN, "%s/gfx/units/%s", dest_path, tac_icons );
    if ( SDL_SaveBMP( surf, path ) != 0 ) {
        fprintf( stderr, "%s: %s\n", path, SDL_GetError() );
        goto failure;
    }
    SDL_FreeSurface( surf );
    shp_free( &shp );
    if (!copy_pg_bmp("strength.bmp","pg_strength.bmp")) goto failure;
    if (!copy_pg_bmp("attack.bmp","pg_attack.bmp")) goto failure;
    if (!copy_pg_bmp("move.bmp","pg_move.bmp")) goto failure;
    if (!copy_pg_bmp("guard.bmp","pg_guard.bmp")) goto failure;
    /* sounds */
    snprintf( path, MAXPATHLEN, "%s/sounds/pg", dest_path );
    mkdir( path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
    snprintf( path, MAXPATHLEN, "%s/convdata/air2.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/air2.wav", dest_path );
    copy( path, path2 );
    snprintf( path, MAXPATHLEN, "%s/convdata/air.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/air.wav", dest_path );
    copy( path, path2 );
    snprintf( path, MAXPATHLEN, "%s/convdata/battle.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/explosion.wav", dest_path );
    copy( path, path2 );
    snprintf( path, MAXPATHLEN, "%s/convdata/leg.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/leg.wav", dest_path );
    copy( path, path2 );
    snprintf( path, MAXPATHLEN, "%s/convdata/sea.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/sea.wav", dest_path );
    copy( path, path2 );
    snprintf( path, MAXPATHLEN, "%s/convdata/select.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/select.wav", dest_path );
    copy( path, path2 );
    snprintf( path, MAXPATHLEN, "%s/convdata/tracked.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/tracked.wav", dest_path );
    copy( path, path2 );
    snprintf( path, MAXPATHLEN, "%s/convdata/wheeled.wav", get_gamedir() );
    snprintf( path2, MAXPATHLEN, "%s/sounds/pg/wheeled.wav", dest_path );
    copy( path, path2 );
    return 1;
failure:
    if ( surf ) SDL_FreeSurface( surf );
    if ( shp ) shp_free( &shp );
    return 0;
}

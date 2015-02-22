/***************************************************************************
                          unit_lib.c  -  description
                             -------------------
    begin                : Sat Mar 16 2002
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

#include "lgeneral.h"
#include "parser.h"
#include "unit_lib.h"
#include "localize.h"
#include "nation.h"

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern int log_x, log_y;
extern Font *log_font;

/*
====================================================================
Target types, movement types, unit classes
These may only be loaded if unit_lib_main_loaded is False.
====================================================================
*/
int unit_lib_main_loaded = 0;
Trgt_Type *trgt_types = 0;
int trgt_type_count = 0;
Mov_Type *mov_types = 0;
int mov_type_count = 0;
Unit_Class *unit_classes= 0;
int unit_class_count = 0;

/*
====================================================================
Unit map tile icons (strength, move, attack ...)
====================================================================
*/
Unit_Info_Icons *unit_info_icons = 0;

/*
====================================================================
Unit library list which is created by the UNIT_LIB_MAIN call
of unit_lib_load().
====================================================================
*/
List *unit_lib = 0;

/*
====================================================================
Convertion table for string -> flag.
====================================================================
*/
StrToFlag fct_units[] = {
    { "swimming", SWIMMING },           
    { "flying", FLYING },               
    { "diving", DIVING },               
    { "parachute", PARACHUTE },         
    { "transporter", TRANSPORTER },     
    { "recon", RECON },                 
    { "artillery", ARTILLERY },         
    { "interceptor", INTERCEPTOR },     
    { "air_defense", AIR_DEFENSE },     
    { "bridge_eng", BRIDGE_ENG },       
    { "infantry", INFANTRY },           
    { "air_trsp_ok", AIR_TRSP_OK },     
    { "destroyer", DESTROYER },         
    { "ignore_entr", IGNORE_ENTR },     
    { "carrier", CARRIER },             
    { "carrier_ok", CARRIER_OK },       
    { "bomber", BOMBER },               
    { "attack_first", ATTACK_FIRST },
    { "low_entr_rate", LOW_ENTR_RATE },
    { "tank", TANK },
    { "anti_tank", ANTI_TANK },
    { "suppr_fire", SUPPR_FIRE },
    { "turn_suppr", TURN_SUPPR },
    { "jet", JET },
    { "ground_trsp_ok", GROUND_TRSP_OK },
	{ "carpet_bombing", CARPET_BOMBING },
    { "X", 0}
};

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Get the geometry of an icon in surface 'icons' by using the three
measure dots in the left upper, right upper, left lower corner.
====================================================================
*/
static void unit_get_icon_geometry( int icon_id, SDL_Surface *icons, int *width, int *height, int *offset, Uint32 *key )
{
    Uint32 mark;
    int y;
    int count = icon_id * 2; /* there are two pixels for one icon */

    /* nada white dot! take the first pixel found in the upper left corner as mark */
    mark = get_pixel( icons, 0, 0 );
    /* compute offset */
    for ( y = 0; y < icons->h; y++ )
        if ( get_pixel( icons, 0, y ) == mark ) {
            if ( count == 0 ) break;
            count--;
        }
    *offset = y;
    /* compute height */
    y++;
    while ( y < icons->h && get_pixel( icons, 0, y ) != mark )
        y++;
   (*height) = y - (*offset) + 1;
    /* compute width */
    y = *offset;
    *width = 1;
    while ( get_pixel( icons, (*width ), y ) != mark )
        (*width)++;
    (*width)++;
    /* pixel beside left upper measure key is color key */
    *key = get_pixel( icons, 1, *offset );
}

/*
====================================================================
Delete unit library entry.
====================================================================
*/
static void unit_lib_delete_entry( void *ptr )
{
    Unit_Lib_Entry *entry = (Unit_Lib_Entry*)ptr;
    if ( entry == 0 ) return;
    if ( entry->id ) free( entry->id );
    if ( entry->name ) free( entry->name );
    if ( entry->icon ) SDL_FreeSurface( entry->icon );
    if ( entry->icon_tiny ) SDL_FreeSurface( entry->icon_tiny );
#ifdef WITH_SOUND
    if ( entry->wav_alloc && entry->wav_move ) 
        wav_free( entry->wav_move );
#endif
    free( entry );
}

/*
====================================================================
Evaluate the unit. This score will become a relative one whereas
the best rating will be 1000. Each operational region 
ground/sea/air will have its own reference.
This evaluation is PG specific. Cost is not considered.
====================================================================
*/
void unit_lib_eval_unit( Unit_Lib_Entry *unit )
{
    int attack = 0, defense = 0, misc = 0;
    /* The score is computed by dividing the unit's properties
       into categories. The subscores are added with different
       weights. */
    /* The first category covers the attack skills. */
    if ( unit->flags & FLYING ) {
        attack = unit->atks[0] + unit->atks[1] + /* ground */
                 2 * MAXIMUM( unit->atks[2], abs( unit->atks[2] ) / 2 ) + /* air */
                 unit->atks[3]; /* sea */
    }
    else {
        if ( unit->flags & SWIMMING ) {
            attack = unit->atks[0] + unit->atks[1] + /* ground */
                     unit->atks[2] + /* air */
                     2 * unit->atks[3]; /* sea */
        }
        else {
            attack = 2 * MAXIMUM( unit->atks[0], abs( unit->atks[0] ) / 2 ) + /* soft */
                     2 * MAXIMUM( unit->atks[1], abs( unit->atks[1] ) / 2 ) + /* hard */
                     MAXIMUM( unit->atks[2], abs( unit->atks[2] ) / 2 ) + /* air */
                     unit->atks[3]; /* sea */
        }
    }
    attack += unit->ini;
    attack += 2 * unit->rng;
    /* The second category covers defensive skills. */
    if ( unit->flags & FLYING )
        defense = unit->def_grnd + 2 * unit->def_air;
    else {
        defense = 2 * unit->def_grnd + unit->def_air;
        if ( unit->flags & INFANTRY )
            /* hype infantry a bit as it doesn't need the
               close defense value */
            defense += 5;
        else
            defense += unit->def_cls;
    }
    /* The third category covers miscellany skills. */
    if ( unit->flags & FLYING )
        misc = MINIMUM( 12, unit->ammo ) + MINIMUM( unit->fuel, 80 ) / 5 + unit->mov / 2;
    else
        misc = MINIMUM( 12, unit->ammo ) + MINIMUM( unit->fuel, 60 ) / 4 + unit->mov;
    /* summarize */
    unit->eval_score = ( 2 * attack + 2 * defense + misc ) / 5;
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Load a unit library. If UNIT_LIB_MAIN is passed target_types,
mov_types and unit classes will be loaded (may only happen once)
====================================================================
*/
int unit_lib_load( char *fname, int main )
{
    int i, j, icon_id;
    SDL_Surface *icons = NULL;
    int icon_type;
    int width, height, offset;
    Uint32 color_key;
    int byte_size, y_offset;
    char *pix_buffer;
    Unit_Lib_Entry *unit;
    int best_ground = 0,
        best_air = 0,
        best_sea = 0; /* used to relativate evaluations */
    List *entries, *flags;
    PData *pd, *sub, *subsub;
    char path[512];
    char *str, *flag;
    char *domain = 0;
    float scale;
    /* log info */
    int  log_dot_limit = 40; /* maximum of dots */
    int  log_dot_count = 0; /* actual number of dots displayed */
    int  log_units_per_dot = 0; /* number of units a dot represents */
    int  log_unit_count = 0; /* if > units_per_dot a new dot is added */
    char log_str[128];
    /* there can be only one main library */
    if ( main == UNIT_LIB_MAIN && unit_lib_main_loaded ) {
        fprintf( stderr, tr("%s: can't load as main unit library (which is already loaded): loading as 'additional' instead\n"),
                 fname );
        main = UNIT_LIB_ADD;
    }
    /* parse file */
    sprintf( path, "%s/units/%s", get_gamedir(), fname );
    sprintf( log_str, tr("  Parsing '%s'"), fname );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    if ( ( pd = parser_read_file( fname, path ) ) == 0 ) goto parser_failure;
    domain = determine_domain(pd, fname);
    locale_load_domain(domain, 0/*FIXME*/);
    /* if main read target types & co */
    if ( main == UNIT_LIB_MAIN ) {
        write_line( sdl.screen, log_font, tr("  Loading Main Definitions"), log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
        unit_lib = list_create( LIST_AUTO_DELETE, unit_lib_delete_entry );
        /* target types */
        if ( !parser_get_entries( pd, "target_types", &entries ) ) goto parser_failure;
        trgt_type_count = entries->count;
        if ( trgt_type_count > TARGET_TYPE_LIMIT ) {
            fprintf( stderr, tr("%i target types is the limit!\n"), TARGET_TYPE_LIMIT );
            trgt_type_count = TARGET_TYPE_LIMIT;
        }
        trgt_types = calloc( trgt_type_count, sizeof( Trgt_Type ) );
        list_reset( entries ); i = 0;
        while ( ( sub = list_next( entries ) ) ) {
            trgt_types[i].id = strdup( sub->name );
            if ( !parser_get_value( sub, "name", &str, 0 ) ) goto parser_failure;
            trgt_types[i].name = strdup(trd(domain, str));
            i++;
        }
        /* movement types */
        if ( !parser_get_entries( pd, "move_types", &entries ) ) goto parser_failure;
        mov_types = calloc( entries->count, sizeof( Mov_Type ) );
        list_reset( entries ); mov_type_count = 0;
        while ( ( sub = list_next( entries ) ) ) {
            mov_types[mov_type_count].id = strdup( sub->name );
            if ( !parser_get_value( sub, "name", &str, 0 ) ) goto parser_failure;
            mov_types[mov_type_count].name = strdup(trd(domain, str));
#ifdef WITH_SOUND                
            if ( parser_get_value( sub, "sound", &str, 0 ) ) {
                mov_types[mov_type_count].wav_move = wav_load( str, 0 );
            }
#endif            
            mov_type_count++;
        }
        /* unit classes */
        if ( !parser_get_entries( pd, "unit_classes", &entries ) ) goto parser_failure;
        unit_classes = calloc( entries->count, sizeof( Unit_Class ) );
        list_reset( entries ); unit_class_count = 0;
        while ( ( sub = list_next( entries ) ) ) {
            unit_classes[unit_class_count].id = strdup( sub->name );
            if ( !parser_get_value( sub, "name", &str, 0 ) ) goto parser_failure;
            unit_classes[unit_class_count].name = strdup(trd(domain, str));
	    unit_classes[unit_class_count].purchase = UC_PT_NONE;
	    if (parser_get_value( sub, "purchase", &str, 0 )) {
		    if (strcmp(str,"trsp") == 0)
			    unit_classes[unit_class_count].purchase = UC_PT_TRSP;
		    else if (strcmp(str,"normal") == 0)
			    unit_classes[unit_class_count].purchase = UC_PT_NORMAL;
	    }
	    unit_class_count++;
            /* ignore sounds so far */
        }
        /* unit map tile icons */
        unit_info_icons = calloc( 1, sizeof( Unit_Info_Icons ) );
        if ( !parser_get_value( pd, "strength_icons", &str, 0 ) ) goto parser_failure;
        sprintf( path, "units/%s", str );
        if ( ( unit_info_icons->str = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure; 
        if ( !parser_get_int( pd, "strength_icon_width", &unit_info_icons->str_w ) ) goto parser_failure;
        if ( !parser_get_int( pd, "strength_icon_height", &unit_info_icons->str_h ) ) goto parser_failure;
        if ( !parser_get_value( pd, "attack_icon", &str, 0 ) ) goto parser_failure;
        sprintf( path, "units/%s", str );
        if ( ( unit_info_icons->atk = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure; 
        if ( !parser_get_value( pd, "move_icon", &str, 0 ) ) goto parser_failure;
        sprintf( path, "units/%s", str );
        if ( ( unit_info_icons->mov = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure; 
        if ( !parser_get_value( pd, "guard_icon", &str, 0 ) ) str = "pg_guard.bmp";
        sprintf( path, "units/%s", str );
        if ( ( unit_info_icons->guard = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure; 
    }
    /* icons */
    if ( !parser_get_value( pd, "icon_type", &str, 0 ) ) goto parser_failure;
    if (STRCMP( str, "fixed" ) )
        icon_type = UNIT_ICON_FIXED;
    else if ( STRCMP( str, "single" ) )
        icon_type = UNIT_ICON_SINGLE;
    else
        icon_type = UNIT_ICON_ALL_DIRS;
    if ( !parser_get_value( pd, "icons", &str, 0 ) ) goto parser_failure;
    sprintf( path, "units/%s", str );
    write_line( sdl.screen, log_font, tr("  Loading Tactical Icons"), log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    if ( ( icons = load_surf( path, SDL_SWSURFACE ) ) == 0 ) goto failure; 
    /* unit lib entries */
    if ( !parser_get_entries( pd, "unit_lib", &entries ) ) goto parser_failure;
      /* LOG INIT */
      log_units_per_dot = entries->count / log_dot_limit;
      log_dot_count = 0;
      log_unit_count = 0;
      /* (LOG) */
    list_reset( entries );
    while ( ( sub = list_next( entries ) ) ) {
        /* read unit entry */
        unit = calloc( 1, sizeof( Unit_Lib_Entry ) );
        /* identification */
        unit->id = strdup( sub->name );
        /* name */
        if ( !parser_get_value( sub, "name", &str, 0) ) goto parser_failure;
        unit->name = strdup(trd(domain, str));
	/* nation (if not found or 'none' unit can't be purchased) */
	unit->nation = -1; /* undefined */
	if ( parser_get_value( sub, "nation", &str, 0) && strcmp(str,"none") ) {
		Nation *n = nation_find( str );
		if (n)
			unit->nation = nation_get_index( n );
	}
        /* class id */
        unit->class = 0;
        if ( !parser_get_value( sub, "class", &str, 0 ) ) goto parser_failure;
        for ( i = 0; i < unit_class_count; i++ )
            if ( STRCMP( str, unit_classes[i].id ) ) {
                unit->class = i;
                break;
            }
        /* target type id */
        unit->trgt_type = 0;
        if ( !parser_get_value( sub, "target_type", &str, 0 ) ) goto parser_failure;
        for ( i = 0; i < trgt_type_count; i++ )
            if ( STRCMP( str, trgt_types[i].id ) ) {
                unit->trgt_type = i;
                break;
            }
        /* initiative */
        if ( !parser_get_int( sub, "initiative", &unit->ini ) ) goto parser_failure;
        /* spotting */
        if ( !parser_get_int( sub, "spotting", &unit->spt ) ) goto parser_failure;
        /* movement */
        if ( !parser_get_int( sub, "movement", &unit->mov ) ) goto parser_failure;
        /* move type id */
        unit->mov_type = 0;
        if ( !parser_get_value( sub, "move_type", &str, 0 ) ) goto parser_failure;
        for ( i = 0; i < mov_type_count; i++ )
            if ( STRCMP( str, mov_types[i].id ) ) {
                unit->mov_type = i;
                break;
            }
        /* fuel */
        if ( !parser_get_int( sub, "fuel", &unit->fuel ) ) goto parser_failure;
        /* range */
        if ( !parser_get_int( sub, "range", &unit->rng ) ) goto parser_failure;
        /* ammo */
        if ( !parser_get_int( sub, "ammo", &unit->ammo ) ) goto parser_failure;
        /* attack count */
        if ( !parser_get_int( sub, "attack/count", &unit->atk_count ) ) goto parser_failure;
        /* attack values */
        if ( !parser_get_pdata( sub, "attack", &subsub ) ) goto parser_failure;
        for ( i = 0; i < trgt_type_count; i++ )
            if ( !parser_get_int( subsub, trgt_types[i].id, &unit->atks[i] ) ) goto parser_failure;
        /* ground defense */
        if ( !parser_get_int( sub, "def_ground", &unit->def_grnd ) ) goto parser_failure;
        /* air defense */
        if ( !parser_get_int( sub, "def_air", &unit->def_air ) ) goto parser_failure;
        /* close defense */
        if ( !parser_get_int( sub, "def_close", &unit->def_cls ) ) goto parser_failure;
        /* flags */
        if ( parser_get_values( sub, "flags", &flags ) ) {
            list_reset( flags ); 
            while ( ( flag = list_next( flags ) ) )
                unit->flags |= check_flag( flag, fct_units );
        }
        /* set the entrenchment rate */
        unit->entr_rate = 2;
        if ( unit->flags & LOW_ENTR_RATE )
            unit->entr_rate = 1;
        else
            if ( unit->flags & INFANTRY )
                unit->entr_rate = 3;
        /* time period of usage (0 == cannot be purchased) */
        unit->start_year = unit->start_month = unit->last_year = 0;
        parser_get_int( sub, "start_year", &unit->start_year );
        parser_get_int( sub, "start_month", &unit->start_month );
        parser_get_int( sub, "last_year", &unit->last_year );
	/* cost of unit (0 == cannot be purchased) */
	unit->cost = 0;
	parser_get_int( sub, "cost", &unit->cost );
        /* icon */
        /* icon id */
        if ( !parser_get_int( sub, "icon_id", &icon_id ) ) goto parser_failure;
        /* icon_type */
        unit->icon_type = icon_type;
        /* get position and size in icons surface */
        unit_get_icon_geometry( icon_id, icons, &width, &height, &offset, &color_key );
        /* picture is copied from unit_pics first
         * if picture_type is not ALL_DIRS, picture is a single picture looking to the right;
         * add a flipped picture looking to the left 
         */
        if ( unit->icon_type == UNIT_ICON_ALL_DIRS ) {
            unit->icon = create_surf( width * 6, height, SDL_SWSURFACE );
            unit->icon_w = width;
            unit->icon_h = height;
            FULL_DEST( unit->icon );
            SOURCE( icons, 0, offset );
            blit_surf();
            /* remove measure dots */
            set_pixel( unit->icon, 0, 0, color_key );
            set_pixel( unit->icon, 0, unit->icon_h - 1, color_key );
            set_pixel( unit->icon, unit->icon_w - 1, 0, color_key );
            /* set transparency */
            SDL_SetColorKey( unit->icon, SDL_SRCCOLORKEY, color_key );
        }
        else {
            /* set size */
            unit->icon_w = width;
            unit->icon_h = height;
            /* create pic and copy first pic */
            unit->icon = create_surf( unit->icon_w * 2, unit->icon_h, SDL_SWSURFACE );
            DEST( unit->icon, 0, 0, unit->icon_w, unit->icon_h );
            SOURCE( icons, 0, offset );
            blit_surf();
            /* remove measure dots */
            set_pixel( unit->icon, 0, 0, color_key );
            set_pixel( unit->icon, 0, unit->icon_h - 1, color_key );
            set_pixel( unit->icon, unit->icon_w - 1, 0, color_key );
            /* set transparency */
            SDL_SetColorKey( unit->icon, SDL_SRCCOLORKEY, color_key );
            /* get format info */
            byte_size = icons->format->BytesPerPixel;
            y_offset = 0;
            pix_buffer = calloc( byte_size, sizeof( char ) );
            /* get second by flipping first one */
            for ( j = 0; j < unit->icon_h; j++ ) {
                for ( i = 0; i < unit->icon_w; i++ ) {
                    memcpy( pix_buffer,
                            unit->icon->pixels +
                            y_offset +
                            ( unit->icon_w - 1 - i ) * byte_size,
                            byte_size );
                    memcpy( unit->icon->pixels +
                            y_offset +
                            unit->icon_w * byte_size +
                            i * byte_size,
                            pix_buffer, byte_size );
                }
                y_offset += unit->icon->pitch;
            }
            /* free mem */
            free( pix_buffer );
        }
        scale = 1.5;
        unit->icon_tiny = create_surf( unit->icon->w * ( 1.0 / scale ), unit->icon->h * ( 1.0 / scale ), SDL_SWSURFACE );
        unit->icon_tiny_w = unit->icon_w * ( 1.0 / scale ); unit->icon_tiny_h = unit->icon_h * ( 1.0 / scale );
        for ( j = 0; j < unit->icon_tiny->h; j++ ) {
            for ( i = 0; i < unit->icon_tiny->w; i++ )
                set_pixel( unit->icon_tiny,
                           i, j, 
                           get_pixel( unit->icon, scale * i, scale * j ) );
        }
        /* use color key of 'big' picture */
        SDL_SetColorKey( unit->icon_tiny, SDL_SRCCOLORKEY, color_key );
        /* read sounds -- well as there are none so far ... */
#ifdef WITH_SOUND
        if ( parser_get_value( sub, "move_sound", &str, 0 ) ) {
            // FIXME reloading the same sound more than once is a
            // big waste of loadtime, runtime, and memory
            if ( ( unit->wav_move = wav_load( str, 0 ) ) )
                unit->wav_alloc = 1;
            else {
                unit->wav_move = mov_types[unit->mov_type].wav_move;
                unit->wav_alloc = 0;
            }
        }
        else {
            unit->wav_move = mov_types[unit->mov_type].wav_move;
            unit->wav_alloc = 0;
        }
#endif      
        /* add unit to database */
        list_add( unit_lib, unit );
        /* absolute evaluation */
        unit_lib_eval_unit( unit );
        /* LOG */
        log_unit_count++;
        if ( log_unit_count >= log_units_per_dot ) {
            log_unit_count = 0;
            if ( log_dot_count < log_dot_limit ) {
                log_dot_count++;
                strcpy( log_str, "  [                                        ]" );
                for ( i = 0; i < log_dot_count; i++ )
                    log_str[3 + i] = '*';
                write_text( log_font, sdl.screen, log_x, log_y, log_str, 255 );
                SDL_UpdateRect( sdl.screen, log_font->last_x, log_font->last_y, log_font->last_width, log_font->last_height );
            }
        }
    }
    parser_free( &pd );
    /* LOG */
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    /* relative evaluate of units */
    list_reset( unit_lib );
    while ( ( unit = list_next( unit_lib ) ) ) {
        if ( unit->flags & FLYING )
            best_air = MAXIMUM( unit->eval_score, best_air );
        else {
            if ( unit->flags & SWIMMING )
                best_sea = MAXIMUM( unit->eval_score, best_sea );
            else
                best_ground = MAXIMUM( unit->eval_score, best_ground );
        }
    }
    list_reset( unit_lib );
    while ( ( unit = list_next( unit_lib ) ) ) {
        unit->eval_score *= 1000;
        if ( unit->flags & FLYING )
            unit->eval_score /= best_air;
        else {
            if ( unit->flags & SWIMMING )
                unit->eval_score /= best_sea;
            else
                unit->eval_score /= best_ground;
        }
    }
    free(domain);
    SDL_FreeSurface(icons);
    return 1;
parser_failure:        
    fprintf( stderr, "%s\n", parser_get_error() );
failure:
    unit_lib_delete();
    if ( pd ) parser_free( &pd );
    free(domain);
    SDL_FreeSurface(icons);
    return 0;
}

/*
====================================================================
Delete unit library.
====================================================================
*/
void unit_lib_delete( void )
{
    int i;
    if ( unit_lib ) {
        list_delete( unit_lib ); 
        unit_lib = 0;
    }
    if ( trgt_types ) {
        for ( i = 0; i < trgt_type_count; i++ ) {
            if ( trgt_types[i].id ) free( trgt_types[i].id );
            if ( trgt_types[i].name ) free( trgt_types[i].name );
        }
        free( trgt_types );
        trgt_types = 0; trgt_type_count = 0;
    }
    if ( mov_types ) {
        for ( i = 0; i < mov_type_count; i++ ) {
            if ( mov_types[i].id ) free( mov_types[i].id );
            if ( mov_types[i].name ) free( mov_types[i].name );
#ifdef WITH_SOUND
            if ( mov_types[i].wav_move )
                wav_free( mov_types[i].wav_move );
#endif            
        }
        free( mov_types );
        mov_types = 0; mov_type_count = 0;
    }
    if ( unit_classes ) {
        for ( i = 0; i < unit_class_count; i++ ) {
            if ( unit_classes[i].id ) free( unit_classes[i].id );
            if ( unit_classes[i].name ) free( unit_classes[i].name );
        }
        free( unit_classes );
        unit_classes = 0; unit_class_count = 0;
    }
    if ( unit_info_icons ) {
        if ( unit_info_icons->str ) SDL_FreeSurface( unit_info_icons->str );
        if ( unit_info_icons->atk ) SDL_FreeSurface( unit_info_icons->atk );
        if ( unit_info_icons->mov ) SDL_FreeSurface( unit_info_icons->mov );
        if ( unit_info_icons->guard ) SDL_FreeSurface( unit_info_icons->guard );
        free( unit_info_icons );
        unit_info_icons = 0;
    }
    unit_lib_main_loaded = 0;
}

/*
====================================================================
Find unit lib entry by id string.
====================================================================
*/
Unit_Lib_Entry* unit_lib_find( char *id )
{
    Unit_Lib_Entry *entry;
    list_reset( unit_lib );
    while ( ( entry = list_next( unit_lib ) ) )
        if ( STRCMP( entry->id, id ) )
            return entry;
    return 0;
}

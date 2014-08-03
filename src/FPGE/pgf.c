/*
 * PGF - PG Forever load/save routines
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

#include <stdio.h>
#include <ctype.h>
#include "fpge.h"
#include "load.h"
#include "pg.h"
#include "pgf.h"
#include "lgeneral.h"
#include "parser.h"
#include "date.h"
#include "nation.h"
#include "unit.h"
#include "file.h"
#include "map.h"
#include "scenario.h"
#include "localize.h"
#include "campaign.h"
#include "unit_lib.h"

extern Sdl sdl;
extern Font *log_font;
extern List *unit_lib;
extern Config config;
extern Setup setup;
extern int log_x, log_y;       /* position where to draw log info */
extern Scen_Info *scen_info;
extern List *players;
extern Player *player;
extern Trgt_Type *trgt_types;
extern int trgt_type_count;
extern Mov_Type *mov_types;
extern int mov_type_count;
extern Unit_Class *unit_classes;
extern int unit_class_count;
extern int icon_type;
extern SDL_Surface *icons;
extern StrToFlag fct_units[];
extern int *weather;
extern int map_w, map_h;
extern Map_Tile **map;
extern List *units;        /* active units */
extern List *vis_units;    /* all units spotted by current player */
extern List *avail_units;  /* available units for current player to deploy */
extern List *reinf;
extern VCond *vconds;
extern int vcond_count;
extern int *casualties; /* sum of casualties grouped by unit class and player */
extern int deploy_turn;
extern char scen_result[64];  /* the scenario result is saved here */
extern char scen_message[128]; /* the final scenario message is saved here */
extern int vcond_check_type;
extern int camp_loaded;
extern char *camp_fname;
extern char *camp_name;
extern char *camp_desc;
extern char *camp_authors;
extern List *camp_entries; /* scenario entries */
extern Camp_Entry *camp_cur_scen;
extern char *camp_first;

unsigned short UCS2_header=0xfeff;
unsigned short axis_experience=200;
unsigned short allied_experience=200;
unsigned short allies_move_first;

/*
====================================================================
Campaign scenario entry.
====================================================================
*/
typedef struct {
    char *id;       /* scenario id */
    char *fname;    /* scenario file name */
} Scen_Entry;

/*
====================================================================
Delete scenario entry.
====================================================================
*/
void scen_entry_delete( void *ptr )
{
    Scen_Entry *entry = (Scen_Entry*)ptr;
    if ( entry ) {
        if ( entry->id ) free( entry->id );
        if ( entry->fname ) free( entry->fname );
        free( entry );
    }
}

int read_utf16_line_convert_to_utf8(FILE *inf, char *line){

	int gcursor=0;
	int eol=0,eof=0;
	unsigned short u1, u2;
	short digit;

	//we expect 2 bytes per char
	while(!eol){
		eof=fread(&digit,sizeof(digit),1,inf);
		if (digit == 0x0d) continue; //ignore 0d
		if (digit == 0x0a || eof==0) {
			line[gcursor]=0;
			eol=1;
			if (gcursor==0) eof-=1;// eof may be 1 or 0 -> 0 or -1
			//printf("eof=%d:>>>>>%s<<<<<\n",eof,line);

			return eof;
		}
		if (digit>0 && digit<0x80) {
			line[gcursor]=(unsigned char)digit;
			gcursor++;
		}
		//big char
		if (digit>=0x80 && digit<0x7ff){
			//we encode to UTF-8
			u1 = (digit & 0x03F) | 0x80;
			u2 = ((digit & 0x007C0) | 0x3000) >> 6;
			line[gcursor]=(unsigned char)u2;
			gcursor++;
			line[gcursor]=(unsigned char)u1;
			gcursor++;
			  //printf("%d=%2x%2x\n",digit,line[gcursor-2],line[gcursor-1]);
		}
		//printf("eof=%d eol=%d digit=%d\n",eof,eol,digit);
		//readkey();
	}
	//printf("eof=%d:>>>>>%s<<<<<\n",eof,line);
	return eof;
}

int read_utf8_line_convert_to_utf8(FILE *inf, char *line){

	int gcursor=0;
	int eol=0,eof=0;
	unsigned char digit;

	//we expect 2 bytes per char
	while(!eol){
		eof=fread(&digit,sizeof(digit),1,inf);
		if (digit == 0x0d) continue; //ignore 0d
		if (digit == 0x0a || eof==0) {
			line[gcursor]=0;
			eol=1;
			if (gcursor==0) eof-=1;// eof may be 1 or 0 -> 0 or -1
			return eof;
		}
//		if (digit==59) digit=9; //convert ; to tabs
		//normal char
		//if (digit>=0 && digit<=0xFF) {
		  //protect from longlines
		line[gcursor]=(unsigned char)digit;
		gcursor++;
		//}
	}

	return eof;
}


int load_line(FILE *inf, char *line, int isUTF16){

	if (isUTF16){
		return read_utf16_line_convert_to_utf8(inf,line);
	}else{
		return read_utf8_line_convert_to_utf8(inf,line);
	}
}

char **parse_pgbrf_campaign_select( const char *path )
{
    FILE *inf;
    char **str, line[1024];
    int state = 0, i = 0;

    inf=fopen(path,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open briefing file\n" );
        return 0;
    }

    str = calloc( 6, sizeof( char *) );

    while (read_utf8_line_convert_to_utf8(inf,line)>=0)
    {
        if ( strchr( line, '<' ) )
        {
            char *pos = strchr( line, '<' );
            pos++;
            if ( pos[0] == 'a' )
            {
                pos++;
                while ( pos[0] == ' ' )
                    pos++;
                if ( pos[0] == '-' )
                {
                    pos = strchr( pos, '\"' );
                    pos++;
                    str[3 * i + 2] = calloc( 10, sizeof( char ) );
                    state = strcspn( pos, "\"" );
                    state++;
                    snprintf( str[3 * i + 2], state, "%s", pos );
                    pos = strchr( pos, '\"' );
                    pos++;
                }
                while ( pos[0] == ' ' )
                    pos++;
                if ( pos[0] == 'h' )
                {
                    pos = strchr( pos, '\"' );
                    pos++;
                    str[3 * i] = calloc( 25, sizeof( char ) );
                    state = strcspn( pos, "\"" );
                    state++;
                    snprintf( str[3 * i], state, "%s", pos );
                }
                pos = strchr( pos, '>' );
                pos++;
                str[3 * i + 1] = calloc( 100, sizeof( char ) );
                state = strcspn( pos, "<" );
                state++;
                snprintf( str[3 * i + 1], state, "%s", pos );
                i++;
            }
        }
    }
    fclose(inf);
    return str;
}

int load_pgf_victory_conditions( const char *full_name, const char *scenario_name )
{
    FILE *inf;
    char log_str[256];
    char outcomes[10][256];
    char line[1024],tokens[20][256];
    int j,i,block=0,last_line_length=-1,cursor=0,token=0, current_outcome = 0;
    int lines=0;
    int vcond_counter = 0;

    inf=fopen(full_name,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open campaign data file\n");
        return 0;
    }

    while (read_utf16_line_convert_to_utf8(inf,line)>=0)
    {
        //count lines so error can be displayed with line number
        lines++;

        //strip comments
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x23) { line[i]=0; break; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)

            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        //Block#3 outcomes
        if (block == 3 && token > 1)
        {
            snprintf( outcomes[current_outcome], 256, "%s", tokens[0] );
            current_outcome++;
        }

        //Block#4 scenario entries
        if (block == 4 && token > 1)
        {
            if ( strcmp( scenario_name, tokens[1] ) == 0 )
            {
                /* create conditions */
                if ( vconds == 0 )
                {
                    /* victory conditions */
                    scen_result[0] = 0;
                    scen_message[0] = 0;
                    sprintf( log_str, tr("Loading Victory Conditions") );
                    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );

                    vcond_count = current_outcome;
                    vconds = calloc( vcond_count, sizeof( VCond ) );
                    /* check type */
                    vcond_check_type = VCOND_CHECK_EVERY_TURN;
                    if ( atoi(tokens[2]) == 0 )
                    {
                        vcond_check_type = VCOND_CHECK_LAST_TURN;
                    }
                }
                for ( i = 1; i < vcond_count; i++ )
                {
                    if ( atoi(tokens[3 * i - 1]) == 0 )
                    {
                        strcpy_lt( vconds[i].result, outcomes[i - 1], 63 );
                        strcpy_lt( vconds[i].message, outcomes[i - 1], 127 );
                        vconds[i].sub_and_count = count_characters( tokens[3 * i + 1], '(' ) +  + 1;
                        /* create subconditions */
                        vconds[i].subconds_and = calloc( vconds[i].sub_and_count, sizeof( VSubCond ) );
                        j = 0;
                        vconds[i].subconds_and[j].type = VSUBCOND_CTRL_HEX_NUM;
                        vconds[i].subconds_and[j].count = atoi( tokens[3 * i] );
                        vconds[i].subconds_and[j].player = player_get_by_index( 0 );
                        for ( j = 1; j < vconds[i].sub_and_count; j++ )
                        {
                            char *temp;
                            vconds[i].subconds_and[j].type = VSUBCOND_CTRL_HEX;
                            sscanf( tokens[3 * i + 1], "(%8d:%8d)", &vconds[i].subconds_and[j].x,
                                    &vconds[i].subconds_and[j].y );
                            temp = strchr( tokens[3 * i + 1], ')' );
                            temp++;
                            vconds[i].subconds_and[j].player = player_get_by_index( 0 );
                        }
                    }
                    else
                    {
                        strcpy_lt( vconds[i].result, outcomes[i - 1], 63 );
                        strcpy_lt( vconds[i].message, outcomes[i - 1], 127 );
                        vconds[i].sub_and_count = 2;
                        /* create subconditions */
                        vconds[i].subconds_and = calloc( vconds[i].sub_and_count, sizeof( VSubCond ) );
                        vconds[i].subconds_and[0].type = VSUBCOND_CTRL_ALL_HEXES;
                        vconds[i].subconds_and[0].player = player_get_by_index( 0 );
                        vconds[i].subconds_and[1].type = VSUBCOND_TURNS_LEFT;
                        vconds[i].subconds_and[1].count = atoi( tokens[3 * i - 1] );
                    }
                }
                /* else condition (used if no other condition is fullfilled and scenario ends) */
                strcpy( vconds[0].result, outcomes[i - 1] );
                strcpy( vconds[0].message, outcomes[i - 1] );
            }
            vcond_counter++;
        }
    }
    //end node
    fclose(inf);
    return 1;
}

int load_pgf_equipment(char *fullName){
    FILE *inf;

    Unit_Lib_Entry *unit;
    char line[1024],tokens[50][256];
    int i,cursor=0,token=0,lines, icon_id;
    int token_len, token_write;
    unsigned short file_type_probe;
    int utf16 = 0, NumberInArray, flag;

    inf=fopen(fullName,"rb");
    if (!inf)
    {
        return 0;
    }
    lines=0;

    //probe for UTF16 file magic bytes
    fread(&file_type_probe, 2, 1, inf);
    if (UCS2_header==file_type_probe) { utf16=1;}
    fseek(inf,0,SEEK_SET);
    unit_lib = list_create( LIST_AUTO_DELETE, unit_lib_delete_entry );

    if ( !unit_lib_load( "basic_unit_data.udb", UNIT_LIB_BASE_DATA ) )
        return 0;

    while (load_line(inf,line,utf16)>=0)
    {
        //count lines so error can be displayed with line number
        lines++;

        for(i=0;i<strlen(line);i++)
            if (line[i]==0x23)
            {
                line[i]=0;
                break;
            }
            //tokenize
        token=0;
        cursor=0;
        for (i = 0; i < strlen(line); i++)
            // 'tab' and 'comma' delimiters
            if (line[i] == 0x09 || line[i] == 0x2c)
            {
                tokens[token][cursor] = 0;
                token++;
                cursor = 0;
            }
            else
            {
                tokens[token][cursor] = line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        //remove quoting
        if (tokens[1][0]==0x22)
        {
            //check ending quote
            token_len=strlen(tokens[1]);

            if (tokens[1][token_len-1]!=0x22)
            {
                printf("Error. Line %d. Check quotation mark in name >%s<. Line skipped.\n",lines,tokens[1]);
                continue;
            }
            //remove start/end quotation marks
            for (i=1;i<token_len-1;i++)
                tokens[1][i-1]=tokens[1][i];
            tokens[1][token_len-2]=0;
            //remove double quote
            // get new length
            token_len=strlen(tokens[1]);
            token_write=0;
            for (i=0;i<token_len+1;i++)
            {
                tokens[1][token_write]=tokens[1][i];
                if (tokens[1][i]==0x22 && tokens[1][i+1]==0x22) i++; //skip next char
                token_write++;
            }
            //all done
        }
        if (token==33)
        {
            /* read unit entry */
            unit = calloc( 1, sizeof( Unit_Lib_Entry ) );
            /* identification */
            unit->id = strdup( tokens[0] );
            /* name */
            unit->name = strdup(tokens[1]);
            /* nation (if not found or 'none' unit can't be purchased) */
            unit->nation = -1; /* undefined */
            /* class id */
            for ( i = 0; i < unit_class_count; i++ )
                if ( STRCMP( tokens[2], unit_classes[i].id ) ) {
                    unit->class = i;
                    break;
                }
            //attack
            for ( i = 0; i < trgt_type_count; i++ )
                unit->atks[i] = atoi(tokens[i + 3]);
            /* attack count */
            unit->atk_count = 1;
            /* ground defense */
            unit->def_grnd = atoi(tokens[7]);
            /* air defense */
            unit->def_air = atoi(tokens[8]);
            /* close defense */
            unit->def_cls = atoi(tokens[9]);
            /* move type id */
            unit->mov_type = 0;
            for ( i = 0; i < mov_type_count; i++ )
                if ( STRCMP( tokens[10], mov_types[i].id ) ) {
                    unit->mov_type = i;
                    break;
                }
            /* initiative */
            unit->ini = atoi(tokens[11]);
            /* range */
            unit->rng = atoi(tokens[12]);
            /* spotting */
            unit->spt = atoi(tokens[13]);
            /* target type id */
            unit->trgt_type = 0;
            for ( i = 0; i < trgt_type_count; i++ )
                if ( STRCMP( tokens[14], trgt_types[i].id ) ) {
                    unit->trgt_type = i;
                    break;
                }
            /* movement */
            unit->mov = atoi(tokens[15]);
            /* fuel */
            unit->fuel = atoi(tokens[16]);
            /* ammo */
            unit->ammo = atoi(tokens[17]);
            /* cost of unit (0 == cannot be purchased) */
            unit->cost = atoi(tokens[18]);
            /* icon id */
            icon_id = atoi(tokens[19]);
            /* icon_type */
            unit->icon_type = icon_type;
            /* set small and large icons */
            lib_entry_set_icons( icon_id, unit );
            /* time period of usage (0 == cannot be purchased) */
            unit->start_month = atoi(tokens[21]);
            unit->start_year = 1900 + atoi(tokens[22]);
            unit->last_year = 1900 + atoi(tokens[23]);

            /* nation (if not found unit can't be purchased) */
            unit->nation = -1;
            Nation *n = nation_find_by_id( atoi(tokens[32]) - 1 );
            if (n)
                unit->nation = nation_get_index( n );

            /* standard flags based on unit class */
            switch ( unit->class )
            {
                case 0: // infantry (check cavalry flags)
                    flag = check_flag( "infantry", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 1: //tank
                    flag = check_flag( "low_entr_rate", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "tank", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 2: // recon
                    flag = check_flag( "recon", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "tank", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 3: // anti-tank
                    flag = check_flag( "anti_tank", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 4: // artillery
                    flag = check_flag( "artillery", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "suppr_fire", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "attack_first", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 5: // anti-aircraft
                    flag = check_flag( "low_entr_rate", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    break;
                case 6: // air defense
                    flag = check_flag( "air_defense", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "attack_first", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    // air defense unit can't attack ground units
                    for ( i = 0; i < trgt_type_count; i++ )
                        if ( i != 2)
                            unit->atks[i] = -unit->atks[i];
                    break;
                case 7: // fortification
                    flag = check_flag( "low_entr_rate", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "suppr_fire", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    break;
                case 8: // fighter
                    flag = check_flag( "interceptor", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "carrier_ok", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "flying", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    break;
                case 9: // tactical bomber
                    flag = check_flag( "bomber", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "carrier_ok", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "flying", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    break;
                case 10: // level bomber
                    flag = check_flag( "suppr_fire", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "turn_suppr", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "flying", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 11: // submarine
                    flag = check_flag( "swimming", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "diving", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 12: // destroyer
                    flag = check_flag( "destroyer", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "swimming", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "suppr_fire", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 13: // capital ship
                    flag = check_flag( "swimming", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "suppr_fire", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 14: // aircraft carrier
                    flag = check_flag( "swimming", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "carrier", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 15: // land transport
                    flag = check_flag( "transporter", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 16: // air transport
                    flag = check_flag( "transporter", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "flying", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
                case 17: // sea transport
                    flag = check_flag( "transporter", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    flag = check_flag( "swimming", fct_units, &NumberInArray );
                    unit->flags[(NumberInArray + 1) / 32] |= flag;
                    unit->atks[2] = -unit->atks[2];
                    break;
            }
            // flags from equipment.pgeqp
            if ( atoi(tokens[25]) )
            {
                flag = check_flag( "ignore_entr", fct_units, &NumberInArray );
                unit->flags[(NumberInArray + 1) / 32] |= flag;
            }
            if ( atoi(tokens[26]) )
            {
                flag = check_flag( "ground_trsp_ok", fct_units, &NumberInArray );
                unit->flags[(NumberInArray + 1) / 32] |= flag;
            }
//            if ( atoi(tokens[27]) )
//                unit->flags[(NumberInArray + 1) / 32] |= check_flag( "sea_trsp_ok", fct_units, &NumberInArray );
            if ( atoi(tokens[28]) )
            {
                flag = check_flag( "air_trsp_ok", fct_units, &NumberInArray );
                unit->flags[(NumberInArray + 1) / 32] |= flag;
            }
            if ( atoi(tokens[29]) )
            {
                flag = check_flag( "parachute", fct_units, &NumberInArray );
                unit->flags[(NumberInArray + 1) / 32] |= flag;
            }
            if ( atoi(tokens[30]) )
            {
                flag = check_flag( "bridge_eng", fct_units, &NumberInArray );
                unit->flags[(NumberInArray + 1) / 32] |= flag;
            }
            if ( atoi(tokens[31]) )
            {
                flag = check_flag( "jet", fct_units, &NumberInArray );
                unit->flags[(NumberInArray + 1) / 32] |= flag;
            }
//            fprintf( stderr, "%s: flags %d\n", unit->name, unit->flags );

#ifdef WITH_SOUND
            unit->wav_move = mov_types[unit->mov_type].wav_move;
            unit->wav_alloc = 0;
#endif

            /* add unit to database */
            list_add( unit_lib, unit );
            /* absolute evaluation */
            unit_lib_eval_unit( unit );
/* unused
            equip[i][AAF] = (unsigned char) atoi(tokens[24]);
            equip_flags[i] |= (unsigned char) atoi(tokens[26])?EQUIPMENT_CAN_HAVE_ORGANIC_TRANSPORT:0;
            equip[i][ALLOWED_TRANSPORT]=0;
            if (atoi(tokens[27])==1) equip[i][ALLOWED_TRANSPORT]=1; //naval transport
            if (atoi(tokens[28])==1) equip[i][ALLOWED_TRANSPORT]=2; //air mobile transport
            if (atoi(tokens[29])==1) equip[i][ALLOWED_TRANSPORT]=3; //paradrop
*/
        }
    }
    relative_evaluate_units();
    /* icons are loaded in unit_lib.c and now we have to free large sheet */
    SDL_FreeSurface(icons);
    fclose(inf);
    return 1;
}

int load_pgf_pgscn(char *fname, char *fullName, int scenNumber){

    FILE *inf;
    char line[1024],tokens[20][1024], log_str[256], SET_file[MAX_PATH], STM_file[MAX_PATH];
    int i, block=0,last_line_length=-1,cursor=0,token=0,x,y,error,lines;
    int flag_map_loaded = 0, flag_unit_load_started = 0, flag_vict_cond_loaded = 0;
    unsigned char t1,t2;
    int unit_ref = 0, auxiliary_units_count = 0;
    Unit_Lib_Entry *unit_prop = 0, *trsp_prop = 0, *land_trsp_prop = 0;
    Unit *unit;
    Unit unit_base; /* used to store values for unit */
    int unit_delayed = 0, current_weather;

    allies_move_first = 0;
    scen_delete();
    player = 0;
    SDL_FillRect( sdl.screen, 0, 0x0 );
    log_font->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    log_x = 2; log_y = 2;
    scen_info = calloc( 1, sizeof( Scen_Info ) );
    scen_info->fname = strdup( fname );
    scen_info->mod_name = strdup( config.mod_name );
    inf=fopen(fullName,"rb");
    if (!inf)
    {
        //printf("Couldn't open scenario file\n");
        return 0;
    }

    sprintf( log_str, tr("*** Loading scenario '%s' ***"), fullName );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );

    lines=0;

    while (read_utf16_line_convert_to_utf8(inf,line)>=0)
    {
        //count lines so error can be displayed with line number
        lines++;

        //strip comments
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x23)
            {
                line[i]=0;
                break;
            }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        //Block#1  +: General scenario data : 2 col, 14 rows
        if (block==1 && token>1)
        {
            if (token!=2)
                fprintf(stderr, "Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,2,token);
            strlwr(tokens[0]);
            if (strcmp(tokens[0],"name")==0)
            {
                /* get scenario info */
                sprintf( log_str, tr("Loading Scenario Info"));
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                scen_info->name = strdup( tokens[1] );
            }
            if (strcmp(tokens[0],"description")==0)
                scen_info->desc = strdup( tokens[1] );
            if (strcmp(tokens[0],"set file")==0)
            {
                /* nations */
                sprintf( log_str, tr("Loading Nations") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                if ( !nations_load( "nations.lgdndb" ) )
                {
                    terrain_delete();
                    scen_delete();
                    if ( player ) player_delete( player );
                    return 0;
                }

                /* unit libs NOT reloaded if continuing campaign */
                if (camp_loaded <= 1)
                {
                    search_file_name_exact( SET_file, "equipment.pgeqp", config.mod_name, "Scenario" );
                    if ( !load_pgf_equipment( SET_file ) )
                    {
                        terrain_delete();
                        scen_delete();
                        if ( player ) player_delete( player );
                        return 0;
                    }
                }
                snprintf( STM_file, MAX_PATH, "%s", tokens[1] );
            }
            if (strcmp(tokens[0],"turns")==0)
                scen_info->turn_limit=atoi(tokens[1]);
            if (strcmp(tokens[0],"year")==0)
                scen_info->start_date.year=atoi(tokens[1]) + 1900;
            if (strcmp(tokens[0],"month")==0)
                scen_info->start_date.month=atoi(tokens[1]) - 1;
            if (strcmp(tokens[0],"day")==0)
                scen_info->start_date.day=atoi(tokens[1]);
            if (strcmp(tokens[0],"days per turn")==0)
                scen_info->days_per_turn=atoi(tokens[1]);
            if (strcmp(tokens[0],"turns per day")==0)
            {
                scen_info->turns_per_day=atoi(tokens[1]);
                if ( scen_info->turns_per_day == 0 )
                    scen_info->turns_per_day = 1; // fix for some scenarios
            }
            if (strcmp(tokens[0],"current weather")==0)
            {
                current_weather = atoi(tokens[1]);
            }
            if (strcmp(tokens[0],"weather zone")==0)
                scen_info->weather_zone = atoi(tokens[1]);
/*            if (strcmp(tokens[0],"max unit strength")==0)
                if (probe_file_only!=SCAN_FOR_MAP_NUMBER) strncpy(block1_Max_Unit_Strength,tokens[1],256);
            if (strcmp(tokens[0],"max unit experience")==0)
                if (probe_file_only!=SCAN_FOR_MAP_NUMBER) strncpy(block1_Max_Unit_Experience,tokens[1],256);
*/
            if (strcmp(tokens[0],"allies move first")==0)
                allies_move_first=atoi(tokens[1]);
        }
        //Block#2  +: Sides : 11 col, 2 rows
        if (block==2 && token>1)
        {
            if (token!=11)
                fprintf(stderr,"Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,11,token);
            if (atoi(tokens[0])==0)
            {
                /* weather settings */
                int water_level;
                weather = calloc( scen_info->turn_limit, sizeof( int ) );
                if ( config.weather == OPTION_WEATHER_OFF )
                    for ( i = 0; i < scen_info->turn_limit; i++ )
                        weather[i] = 0;
                else
                {
                    if ( ( scen_info->start_date.month + 1 >= 11 && scen_info->weather_zone > 0 ) ||
                         ( scen_info->start_date.month <= 2  && scen_info->weather_zone > 0 ) )
                        water_level = 5;
                    else
                        water_level = 0;
                    if ( config.weather == OPTION_WEATHER_PREDETERMINED )
                    {
                        /* starting weather condition stays until the end of scenario */
                        for ( i = 0; i < scen_info->turn_limit; i++ )
                            weather[i] = current_weather + 4 * (int)( water_level / 2 );
                    }
                    else
                        scen_create_random_weather( water_level, current_weather );
                }

                /* players */
                sprintf( log_str, tr("Loading Players") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                scen_info->player_count = 2;

                /* create player */
                player = calloc( 1, sizeof( Player ) );
                player->nations = calloc( 6, sizeof( Nation* ) );
                player->nation_count = 0;
                player->uber_units = 0;
                player->force_retreat = 0;
                player_add( player ); player = 0;
                player = calloc( 1, sizeof( Player ) );
                player->nations = calloc( 6, sizeof( Nation* ) );
                player->nation_count = 0;
                player->uber_units = 0;
                player->force_retreat = 0;
                player_add( player ); player = 0;

                list_reset( players );
                player = list_next( players );
                if ( allies_move_first )
                {
                    player = list_next( players );
                    player->ctrl = PLAYER_CTRL_CPU;
                    player->core_limit = -1;
                }
                else
                {
                    player->ctrl = PLAYER_CTRL_HUMAN;
                    if ((camp_loaded != NO_CAMPAIGN) && config.use_core_units)
                    {
                        player->core_limit = atoi(tokens[2]);
                    }
                    else
                        player->core_limit = -1;
                }
//                fprintf( stderr, "Core limit:%d\n", player->core_limit );

                player->id = strdup( "axis" );
                player->name = strdup(trd(domain, "Axis"));
                player->ai_fname = strdup( "default" );
                player->strength_row = 0;

                player->unit_limit = atoi(tokens[2]) + atoi(tokens[3]);
//                fprintf( stderr, "Unit limit:%d\n", player->unit_limit );
                if ( atoi(tokens[4]) == 0)
                    player->strat = -1;
                else
                    player->strat = 1;
//                fprintf( stderr, "Strategy:%d\n", player->strat );

                player->air_trsp_count = (unsigned char)atoi(tokens[8]);
//                fprintf( stderr, "Air transport count: %d\n", player->air_trsp_count );
                player->air_trsp = unit_lib_find( tokens[9] );
//                fprintf( stderr, "Air transport type: %d\n", air_trsp_player1 );
//                s4_buffer[AXIS_AIR_TYPE+1]=(unsigned char)(atoi(tokens[9])>>8);

                player->sea_trsp_count = (unsigned char)atoi(tokens[6]);
//                fprintf( stderr, "Sea transport count: %d\n", player->sea_trsp_count );
                player->sea_trsp = unit_lib_find( tokens[7] );
//                s4_buffer[AXIS_SEA_TYPE+1]=(unsigned char)(atoi(tokens[7])>>8);

                player->prestige_per_turn = calloc( scen_info->turn_limit, sizeof(int));
                player->prestige_per_turn[0] = (unsigned char)atoi(tokens[1]);
//                fprintf( stderr, "Prestige: %d\n", player->prestige_per_turn[0] );
//                s4_buffer[AXIS_PRESTIGE+1]=(unsigned char)(atoi(tokens[1])>>8);


//                axis_experience = atoi(tokens[10]);
            }
            if (atoi(tokens[0])==1)
            {
                list_reset( players );
                player = list_next( players );
                if ( allies_move_first )
                {
                    player->ctrl = PLAYER_CTRL_HUMAN;
                    if ((camp_loaded != NO_CAMPAIGN) && config.use_core_units)
                    {
                        player->core_limit = atoi(tokens[2]);
                    }
                    else
                        player->core_limit = -1;
                }
                else
                {
                    player = list_next( players );
                    player->ctrl = PLAYER_CTRL_CPU;
                        player->core_limit = -1;
                }
//                fprintf( stderr, "Core limit:%d\n", player->core_limit );

                player->id = strdup( "allies" );
                player->name = strdup(trd(domain, "Allies"));
                player->ai_fname = strdup( "default" );
                player->strength_row = 2;

                player->unit_limit = atoi(tokens[2]) + atoi(tokens[3]);
//                fprintf( stderr, "Unit limit:%d\n", player->unit_limit );
                if ( atoi(tokens[4]) == 0)
                    player->strat = -1;
                else
                    player->strat = 1;
//                fprintf( stderr, "Strategy:%d\n", player->strat );

                player->air_trsp_count = atoi(tokens[8]);
//                fprintf( stderr, "Air transport count: %d\n", player->air_trsp_count );
                player->air_trsp = unit_lib_find( tokens[9] );
//                fprintf( stderr, "Air transport type: %d\n", air_trsp_player1 );
//                s4_buffer[ALLIED_AIR_TYPE+1]=(unsigned chaallies_move_firstr)(atoi(tokens[9])>>8);

                player->sea_trsp_count = atoi(tokens[6]);
//                fprintf( stderr, "Sea transport count: %d\n", player->sea_trsp_count );
                player->sea_trsp = unit_lib_find( tokens[7] );
//                fprintf( stderr, "Sea transport type: %d\n", sea_trsp_player1 );
//                s4_buffer[ALLIED_SEA_TYPE+1]=(unsigned char)(atoi(tokens[7])>>8);

                player->prestige_per_turn = calloc( scen_info->turn_limit, sizeof(int));
                player->prestige_per_turn[0] = atoi(tokens[1]);
//                fprintf( stderr, "Prestige: %d\n", player->prestige_per_turn[0] );
//                s4_buffer[ALLIED_PRESTIGE+1]=(unsigned char)(atoi(tokens[1])>>8);

//                allied_experience = atoi(tokens[10]);

                player->nations = calloc( 6, sizeof( Nation* ) );
                player->nation_count = 0;

                /* set alliances */
                list_reset( players );
                for ( i = 0; i < players->count; i++ ) {
                    player = list_next( players );
                    if ( ( atoi(tokens[5]) == -1 && strcmp( player->id, "allies" ) ) ||
                         ( atoi(tokens[5]) == 1 && strcmp( player->id, "axis" ) ) )
                        player->orient = UNIT_ORIENT_LEFT;
                    else
                        player->orient = UNIT_ORIENT_RIGHT;
                    player->allies = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
                }
            }
        }
        //Block#3  +: Alliances: 2 col, 2 or more rows
        if (block==3 && token>1)
        {
            if (token!=2)
            {
                fprintf(stderr, "Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,2,token);
            }
            else
            {
                t1=atoi(tokens[0]);
                t2=atoi(tokens[1]);

                list_reset( players );
                player = list_next( players );
                if ( (t2==1 && !allies_move_first) || (t2==0 && allies_move_first) )
                {
                    player = list_next( players );
                }
                player->nations[player->nation_count] = nation_find_by_id( t1 - 1 );
//                fprintf( stderr, "Player %s: %s\n", player->name, player->nations[player->nation_count]->name );
                player->nation_count++;

                if (player->nation_count > 6)
                    fprintf(stderr, "Error. Line %d. Too many nations (max=6 per side).\n",lines);
            }
        }
        //Block#4   : Per-turn prestige allotments: 3 col, rows = no of turns
        if (block==4 && token>1)
        {
            if ( !flag_map_loaded )
            {
                list_reset( players );
                for ( i = 0; i < players->count; i++ )
                {
                    player = list_next( players );
                    if ( player->air_trsp != 0 )
                        player->air_trsp->nation = nation_get_index( player->nations[0] );
                    if ( player->sea_trsp != 0 )
                        player->sea_trsp->nation = nation_get_index( player->nations[0] );
                }
                /* flip icons if scenario demands it */
                adjust_fixed_icon_orientation();

                /* map and weather */
                search_file_name_exact( SET_file, STM_file, config.mod_name, "Scenario" );
                sprintf( log_str, tr("Loading Map '%s'"), SET_file );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                if ( !load_map_pg( SET_file ) )
                {
                    terrain_delete();
                    scen_delete();
                    if ( player ) player_delete( player );
                    return 0;
                }
                flag_map_loaded = 1;
            }

            list_reset( players );
            for ( i = 0; i < players->count; i++ )
            {
                player = list_next( players );
                player->prestige_per_turn[atoi(tokens[0]) - 1] += atoi( tokens[i + 1] );
//                fprintf( stderr, "%s prestige: %d\n", player->name, player->prestige_per_turn[atoi(tokens[0]) - 1] );
            }
        }
/*			  //Block#5  ?: Supply hexes: 2 col, rows - many ** UNUSED **
			  if (block == 5 && token > 1) {
				if (block5_lines > MAX_SUPPLY)
					printf(
							"Error. Line %d. Too many lines in 'Supply hexes' block.\n",
							lines);
				else {
					strncpy(block5[block5_lines], line,MAX_LINE_SIZE);
					block5_lines++;
				}
			}*/
        //Block#6  +: Victory hexes: 1 col, rows - many, limit 20
        if (block==6 && strlen(tokens[0])>0)
        {
            if (token!=1)
                printf("Error. Line %d. Victory hexes. Expected no of columns %d while %d columns detected.\n",lines,1,token);

            error=sscanf(tokens[0],"(%8d:%8d)",&x,&y);
            //some protection
            if (x>=0 && y>=0 && x<map_w && y<map_h){
                map[x][y].obj = 1;
            }
        }
        //Block#7   : Victory conditions: 3 col, rows 2
        if (block == 7 && token > 1 && !flag_vict_cond_loaded )
        {
            char camp_file_path[MAX_PATH], scenario_name[MAX_NAME];
            snprintf( scenario_name, MAX_NAME, "%s.pgscn", fname );
            search_file_name_exact( camp_file_path, "pg.pgcam", config.mod_name, "Scenario" );
            load_pgf_victory_conditions( camp_file_path, scenario_name );
            flag_vict_cond_loaded = 1;
        }
        //Block#8  +: Deploy hexes: 1 col, rows - many
        if (block==8 && strlen(tokens[0])>0)
        {
            error=sscanf(tokens[0],"(%8d:%8d)",&x,&y);
            if (error == 2 && x >= 0 && y >= 0 && x < map_w && y < map_h)
            {
                map_set_deploy_field( x, y, 0 );
            }
        }
        //Block#9   : Purchasable classes: 2 col, rows up to 16 ?
        if (block == 9 && token > 1)
        {
            if ( atoi( tokens[0] ) < 15 )
            {
                if ( atoi( tokens[1] ) == 1 )
                    unit_classes[ atoi( tokens[0] ) ].purchase = UC_PT_NORMAL;
                else
                    unit_classes[ atoi( tokens[0] ) ].purchase = UC_PT_NONE;
            }
        }
        //Block#10 +: Units: 12 col, rows - many, limit
        if (block==10 && token>1)
        {
            if (token!=12)
                fprintf(stderr, "Error. Line %d. Units. Expected no of columns %d while %d columns detected.\n",lines,12,token);

            //0 #Hex
            //1 #Type
            //2 #Organic Transport Type
            //3 #Sea/Air Transport Type
            //4 #Side
            //5 #Flag
            //6 #Strength
            //7 #Experience
            //8 #Entrenchment
            //9 #Fuel
            //10 #Ammo
            //11 #Auxiliary

            else
            {
                if ( !flag_unit_load_started )
                {
                    /* units */
                    sprintf( log_str, tr("Loading Units") );
                    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                    units = list_create( LIST_AUTO_DELETE, unit_delete );
                    /* load core units from earlier scenario or create reinf list - unit status is checked elsewhere */
                    if (camp_loaded > 1 && config.use_core_units)
                    {
                        Unit *entry;
                        list_reset( reinf );
                        while ( ( entry = list_next( reinf ) ) )
                            if ( (camp_loaded == CONTINUE_CAMPAIGN) || ((camp_loaded == RESTART_CAMPAIGN) && entry->core == STARTING_CORE) )
                            {
                                entry->nation = nation_find_by_id( entry->prop.nation );
                                if ( ( entry->player = player_get_by_nation( entry->nation ) ) == 0 ) {
                                    fprintf( stderr, tr("%s: no player controls this nation\n"), entry->nation->name );
                                }
                                entry->orient = entry->player->orient;
                                entry->core = STARTING_CORE;
                                unit_reset_attributes(entry);
                                if (camp_loaded != NO_CAMPAIGN)
                                {
                                    unit = unit_duplicate( entry,0 );
                                    unit->killed = 2;
                                    /* put unit to available units list */
                                    list_add( units, unit );
                                }
                                entry->core = CORE;
                            }
                    }
                    else
                        reinf = list_create( LIST_AUTO_DELETE, unit_delete );
                    avail_units = list_create( LIST_AUTO_DELETE, unit_delete );
                    vis_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );

                    flag_unit_load_started = 1;
                }
                /* unit type */
                if ( ( unit_prop = unit_lib_find( tokens[1] ) ) == 0 ) {
                    fprintf( stderr, tr("%s: unit entry not found\n"), tokens[1] );
                    goto failure;
                }
                memset( &unit_base, 0, sizeof( Unit ) );
                /* nation & player */
                if ( ( unit_base.nation = nation_find_by_id( atoi(tokens[5]) - 1 ) ) == 0 ) {
                    fprintf( stderr, tr("%s: not a nation\n"), tokens[5] );
                    goto failure;
                }
                if ( ( unit_base.player = player_get_by_nation( unit_base.nation ) ) == 0 ) {
                    fprintf( stderr, tr("%s: no player controls this nation\n"), unit_base.nation->name );
                    goto failure;
                }
//                fprintf( stderr, "%s %s %s\n", unit_prop->name, unit_base.nation->name, unit_base.player->name );
                /* name */
                unit_set_generic_name( &unit_base, unit_ref + 1, unit_prop->name );
                /* delay */
                unit_delayed = 0;
                /* position */
                error=sscanf(tokens[0],"(%8d:%8d)",&unit_base.x,&unit_base.y);
                if (x>=map_w || y>=map_h || x<0 || y<0) //this is an error!
                {
                    printf("Warning. Ignoring line %d. Check unit x,y !\n",lines);
                }
                if ( !unit_delayed && ( unit_base.x <= 0 || unit_base.y <= 0 || unit_base.x >= map_w - 1 || unit_base.y >= map_h - 1 ) ) {
                    fprintf( stderr, tr("%s: out of map: ignored\n"), unit_base.name );
                    continue;  
                }
                /* strength, entrenchment, experience */
                unit_base.str = atoi(tokens[6]);
                if (unit_base.str > 10 )
                    unit_base.max_str = 10;
                else
                    unit_base.max_str = unit_base.str;
                unit_base.entr = atoi(tokens[8]);
                unit_base.exp_level = atoi(tokens[7])/100;
                /* transporter */
                trsp_prop = 0;
                land_trsp_prop = 0;
                if ( atoi(tokens[3]) == 0 )
                    trsp_prop = unit_lib_find( tokens[2] );
                else
                {
                    land_trsp_prop = unit_lib_find( tokens[2] );
                    trsp_prop = unit_lib_find( tokens[3] );
                }
                /* core */
                unit_base.core = !atoi(tokens[11]);
                if ( !config.use_core_units )
                    unit_base.core = 0;
//                fprintf( stderr, "%s %d\n", unit_prop->name, unit_base.core );
                /* orientation */
                unit_base.orient = unit_base.player->orient;
                /* tag if set */
                unit_base.tag[0] = 0;
                /* actual unit */
                if ( !(unit_base.player->ctrl == PLAYER_CTRL_HUMAN && auxiliary_units_count >= 
                       unit_base.player->unit_limit - unit_base.player->core_limit) ||
                       ( (camp_loaded != NO_CAMPAIGN) && STRCMP(camp_cur_scen->id, camp_first) && config.use_core_units ) )
                {
                    unit = unit_create( unit_prop, trsp_prop, land_trsp_prop, &unit_base );
                    /* put unit to active or reinforcements list or available units list */
                    if ( !unit_delayed ) {
                        list_add( units, unit );
                        /* add unit to map */
                        map_insert_unit( unit );
                    }
                }
                else if (config.purchase == NO_PURCHASE) /* no fixed reinfs with purchase enabled */
                    list_add( reinf, unit );
                /* adjust transporter count */
                if ( unit->embark == EMBARK_SEA ) {
                    unit->player->sea_trsp_count++;
                    unit->player->sea_trsp_used++;
                }
                else
                    if ( unit->embark == EMBARK_AIR ) {
                        unit->player->air_trsp_count++;
                        unit->player->air_trsp_used++;
                    }
                unit_ref++;
                if (unit_base.player->ctrl == PLAYER_CTRL_HUMAN)
                    auxiliary_units_count++;
				    }
			  }
			  continue;
    }

    casualties = calloc( scen_info->player_count * unit_class_count, sizeof casualties[0] );
    deploy_turn = config.deploy_turn;

    /* check which nations may do purchase for this scenario */
    update_nations_purchase_flag();

    return 1;
failure:
    fclose(inf);
    terrain_delete();
    scen_delete();
    if ( player ) player_delete( player );
    return 0;
}

char *load_pgf_pgscn_info( const char *fname, char *path, int name_only )
{
    FILE *inf;
    char line[1024],tokens[20][1024];
    int i, block=0,last_line_length=-1,cursor=0,token=0,lines;
    char name[256], desc[1024], turns[10], *info;
    allies_move_first = 0;

    inf=fopen(path,"rb");
    if (!inf)
    {
        //printf("Couldn't open scenario file\n");
        return 0;
    }

    while (read_utf16_line_convert_to_utf8(inf,line)>=0)
    {
        //count lines so error can be displayed with line number
        lines++;

        //strip comments
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x23)
            {
                line[i]=0;
                break;
            }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
               tokens[token][cursor]=0;
               token++;cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        //Block#1  +: General scenario data : 2 col, 14 rows
        if (block==1 && token>1)
        {
            if (token!=2)
                printf("Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,2,token);
            strlwr(tokens[0]);
            if (strcmp(tokens[0],"name")==0)
                strncpy(name,tokens[1],256);
            if ( name_only )
            {
                /* we need only scenario name */
                info = calloc( strlen( name ) + 1, sizeof( char ) );
                sprintf( info, tr("%s"), name );
                fclose(inf);
                return info;
            }
            if (strcmp(tokens[0],"description")==0)
                strncpy(desc,tokens[1],1024);
            if (strcmp(tokens[0],"turns")==0)
                strncpy(turns,tokens[1],10);
            if (strcmp(tokens[0],"allies move first")==0)
            {
                allies_move_first=atoi(tokens[1]);
            }
        }
        if (block==2)
        {
            /* set setup */
            scen_clear_setup();
            strcpy( setup.fname, fname );
            strcpy( setup.camp_entry_name, "" );
            setup.player_count = 2;
            setup.ctrl = calloc( setup.player_count, sizeof( int ) );
            setup.names = calloc( setup.player_count, sizeof( char* ) );
            setup.modules = calloc( setup.player_count, sizeof( char* ) );
            /* load the player ctrls */
            if ( !allies_move_first )
            {
                setup.names[0] = strdup(trd(domain, "Axis"));
                setup.names[1] = strdup(trd(domain, "Allies"));
            }
            else
            {
                setup.names[0] = strdup(trd(domain, "Allies"));
                setup.names[1] = strdup(trd(domain, "Axis"));
            }
            setup.ctrl[0] = PLAYER_CTRL_HUMAN;
            setup.ctrl[1] = PLAYER_CTRL_CPU;
            setup.modules[0] = strdup( "default" );
            setup.modules[1] = strdup( "default" );
            if ( ( info = calloc( strlen( name ) + strlen( desc ) + strlen( turns ) + 30, sizeof( char ) ) ) == 0 ) {
                fprintf( stderr, tr("Out of memory\n") );
                free( info );
            }
            sprintf( info, tr("%s##%s##%s Turns#%d Players"), name, desc, turns, 2 );
            fclose(inf);
            return info;
        }
    }
    fclose(inf);
    return 0;
}

int parse_pgcam( const char *fname, const char *full_name, const char *info_entry_name )
{
    List *temp_list, *scenarios = 0;
    Camp_Entry *centry = 0;
    Scen_Entry *sentry = 0;
    FILE *inf;
    char brfnametmp[256];
    char outcomes[10][256];
    char line[1024],tokens[20][256], brf_path[MAX_PATH];
    char str[MAX_LINE_SHORT];
    int j,i,block=0,last_line_length=-1,cursor=0,token=0, current_outcome = 0;
    int lines=0;
    camp_delete();

    inf=fopen(full_name,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open scenario file\n");
        return 0;
    }

    scenarios = list_create( LIST_AUTO_DELETE, scen_entry_delete );

    while (read_utf16_line_convert_to_utf8(inf,line)>=0)
    {
        //count lines so error can be displayed with line number
        lines++;

        //strip comments
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x23) { line[i]=0; break; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        //Block#2 entry points
        if (block == 2 && token > 1)
        {
            if ( strcmp( tokens[0], info_entry_name ) == 0 )
            {
                /* name, desc */
                camp_fname = strdup( fname );
                camp_name = strdup( tokens[0] );
                camp_desc = strdup( tokens[2] );
                temp_list = list_create( LIST_AUTO_DELETE, camp_delete_entry );
                camp_first = strdup( tokens[1] );
            }
        }

        //Block#3 outcomes
        if (block == 3 && token > 1)
        {
            snprintf( outcomes[current_outcome], 256, "%s", tokens[0] );
            current_outcome++;
        }

        //Block#4 scenario entries
        if (block == 4 && token > 1)
        {
            sentry = calloc( 1, sizeof( Scen_Entry ) );
            sentry->id = strdup( tokens[0] );
            sentry->fname = strdup( tokens[1] );
            list_add( scenarios, sentry );
        }

        //Block#5 path
        if (block == 5 && token > 1)
        {
            /* entries */
            centry = calloc( 1, sizeof( Camp_Entry ) );
            centry->id = strdup( tokens[0] );
            centry->scen = strdup( tokens[2] );
            //check initial briefing
            if (strlen(tokens[1])>0)
            {
                strncpy(brfnametmp,tokens[1],256);
                search_file_name_exact( brf_path, brfnametmp, config.mod_name, "Scenario" );
                centry->brief = strdup( parse_pgbrf( brf_path ) );
            }

            centry->nexts = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
            for (j = 0; j < current_outcome; j++)
            {
                if ( strcmp( tokens[current_outcome * ( j + 1 )], "" ) == 0 )
                {
                    strncpy(brfnametmp,tokens[current_outcome * ( j + 1 ) + 2],256);
                    search_file_name_exact( brf_path, brfnametmp, config.mod_name, "Scenario" );
                    char **camp_option = parse_pgbrf_campaign_select( brf_path );
                    // TODO implement prestige costs
                    // selection entry
                    Camp_Entry *select_entry = calloc( 1, sizeof( Camp_Entry ) );
                    sprintf( str, "%s_selection", centry->id );
                    select_entry->id = strdup( str );
                    // select next scenarios
                    select_entry->nexts = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
                    snprintf( str, sizeof(str), "%s>%s", camp_option[1], camp_option[0] );
                    list_add( select_entry->nexts, strdup( str ) );
                    snprintf( str, sizeof(str), "%s>%s", camp_option[4], camp_option[3] );
                    list_add( select_entry->nexts, strdup( str ) );
                    // select descriptions
                    select_entry->descs = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
                    snprintf( str, sizeof(str), "%s>%s", camp_option[1], camp_option[1] );
                    list_add( select_entry->descs, strdup( str ) );
                    snprintf( str, sizeof(str), "%s>%s", camp_option[4], camp_option[4] );
                    list_add( select_entry->descs, strdup( str ) );
                    // checking if selection entry already exists in campaign memory
                    Camp_Entry *search_entry;
                    int found = 0;
                    list_reset( temp_list );
                    while ( ( search_entry = list_next( temp_list ) ) )
                    {
                        if ( strcmp( search_entry->id, select_entry->id ) == 0 )
                            found = 1;
                    }
                    if ( !found )
                    {
                        list_add( temp_list, select_entry );
//                        fprintf(stderr, "%s\n", select_entry->id );
                    }
                    // select entry for current scenario
                    snprintf( str, sizeof(str), "%s>%s", outcomes[j], select_entry->id );
                    list_add( centry->nexts, strdup( str ) );
                }
                else if ( strcmp( tokens[current_outcome * ( j + 1 )], "END" ) == 0 )
                {
                    // ending entry (id is pgbrf file)
                    Camp_Entry *end_entry = calloc( 1, sizeof( Camp_Entry ) );
                    end_entry->id = strdup( tokens[current_outcome * ( j + 1 ) + 2] );
                    strncpy(brfnametmp,tokens[current_outcome * ( j + 1 ) + 2],256);
                    search_file_name_exact( brf_path, brfnametmp, config.mod_name, "Scenario" );
                    end_entry->brief = strdup( parse_pgbrf( brf_path ) );
                    // checking if ending entry already exists in campaign memory
                    Camp_Entry *search_entry;
                    int found = 0;
                    list_reset( temp_list );
                    while ( ( search_entry = list_next( temp_list ) ) )
                    {
                        if ( strcmp( search_entry->id, end_entry->id ) == 0 )
                            found = 1;
                    }
                    if ( !found )
                    {
                        list_add( temp_list, end_entry );
//                        fprintf(stderr, "%s\n", end_entry->brief );
                    }
                    // ending entry for current scenario
                    snprintf( str, sizeof(str), "%s>%s", outcomes[j], tokens[current_outcome * ( j + 1 ) + 2] );
                    list_add( centry->nexts, strdup( str ) );
                }
                else
                {
                    // linear next scenario
                    snprintf( str, sizeof(str), "%s>%s", outcomes[j], tokens[current_outcome * ( j + 1 )] );
                    list_add( centry->nexts, strdup( str ) );
                }
                // debriefings
                centry->descs = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
                strncpy(brfnametmp,tokens[current_outcome * ( j + 1 ) + 2],256);
                search_file_name_exact( brf_path, brfnametmp, config.mod_name, "Scenario" );
                snprintf( str, sizeof(str), "%s>%s", outcomes[j], strdup( parse_pgbrf( brf_path ) ) );
                list_add( centry->descs, strdup( str ) );
//                fprintf(stderr, "%s\n", str );
            }
            list_add( temp_list, centry );
        }
    }

    //end node
    fclose(inf);

    /* build campaign starting at selected entry point */
    camp_entries = list_create( LIST_AUTO_DELETE, camp_delete_entry );
    list_reset( temp_list );
    /* searching for entry point */
    for ( i = 0; i < temp_list->count; i++ )
    {
        centry = list_next( temp_list );
        if ( strcmp( centry->id, camp_first ) == 0 )
        {
            list_transfer( temp_list, camp_entries, centry );
            break;
        }
    }

    Camp_Entry *final_entry;
    char *next = 0, *ptr;
    list_reset( camp_entries );
    /* searching for subsequent entries */
    while ( ( final_entry = list_next( camp_entries ) ) )
    {
        if ( final_entry->nexts && final_entry->nexts->count > 0 )
        {
            list_reset( final_entry->nexts );
            while ( (next = list_next( final_entry->nexts )) )
            {
                ptr = strchr( next, '>' ); 
                if ( ptr ) 
                {
//fprintf(stderr, "%s\n", next );
                    ptr++; 
                    list_reset( temp_list );
                    while ( ( centry = list_next( temp_list ) ) )
                        if ( strcmp( ptr, centry->id ) == 0 )
                        {
                            list_transfer( temp_list, camp_entries, centry );
                        }
                }
            }
        }
        /* searching for scenario filenames */
        list_reset( scenarios );
        while ( ( sentry = list_next( scenarios ) ) )
            if ( ( final_entry->scen ) && strcmp( sentry->id, final_entry->scen ) == 0 )
            {
                free( final_entry->scen );
                final_entry->scen = calloc( 10, sizeof( char ) );
                int length = strcspn( sentry->fname, "." );
                length++;
                snprintf( final_entry->scen, length, "%s", sentry->fname );
                break;
            }
    }

    /* cleanup */
    camp_loaded = FIRST_SCENARIO;
    camp_verify_tree();

    return 1;
}

char *parse_pgcam_info( List *camp_entries, const char *fname, char *path, char *info_entry_name )
{
    FILE *inf;
    char line[1024],tokens[20][256];
    int i,block=0,last_line_length=-1,cursor=0,token=0;
    int lines=0;

    inf=fopen(path,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open scenario file\n");
        return 0;
    }

    while (read_utf16_line_convert_to_utf8(inf,line)>=0)
    {
        //count lines so error can be displayed with line number
        lines++;

        //strip comments
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x23) { line[i]=0; break; }
        if (strlen(line)>0 && last_line_length==0)
        {
            block++;
        }
        last_line_length=strlen(line);
        token=0;
        cursor=0;
        for(i=0;i<strlen(line);i++)
            if (line[i]==0x09)
            {
                tokens[token][cursor]=0;
                token++;
                cursor=0;
            }
            else
            {
                tokens[token][cursor]=line[i];
                cursor++;
            }
        tokens[token][cursor]=0;
        token++;

        //entry points
        if (block == 2 && token > 1)
        {
            if ( camp_entries != 0 )
            {
                Name_Entry_Type *name_entry;
                name_entry = calloc( 1, sizeof( Name_Entry_Type ) );
                name_entry->file_name = strdup( fname );
                name_entry->internal_name = strdup( tokens[0] );
                list_add( camp_entries, name_entry );
            }
            else if ( info_entry_name != 0 )
            {
                if ( strcmp( tokens[0], info_entry_name ) == 0 )
                {
                    fclose(inf);
                    char *info = calloc( strlen( tokens[0] ) + strlen( tokens[2] ) + 3, sizeof( char ) );
                    sprintf( info, "%s##%s", tokens[0], tokens[2] );
                    strcpy( setup.fname, fname );
                    strcpy( setup.camp_entry_name, info_entry_name );
                    setup.scen_state = 0;
                    return info;
                }
            }
        }
        //Block#5  path
        if (block == 5 && token > 1)
        {
            if ( camp_entries != 0 )
            {
                fclose(inf);
                return "1";
            }
        }
    }
    //end node
    fclose(inf);

    return "1";
}

char *parse_pgbrf( const char *path )
{
    FILE *inf;
    char *str;
    int str_idx=0;
    int state=0;
    int buf;

    inf=fopen(path,"rb");
    if (!inf)
    {
        fprintf( stderr, "Couldn't open briefing file\n" );
        return 0;
    }

    str = calloc( 1500, sizeof( char ) );

    buf = fgetc (inf);
    while (buf != EOF)
    {
        switch ( buf )
        {
            case '<': // strip '<p>'; don't change '<a' status
                if ( state < 4 )
                    state = 1;
                break;
            case '>':
                if ( state < 4 )
                    state = 3;
                break;
            case '/':
                if ( state < 4 )
                    state = 2;
                break;
            case 'a': // campaign option; not a part of briefing
                if ( state == 1 )
                {
                    state = 4;
                    break;
                }
            case 'p': // line break on </p>
                if ( state == 2 )
                {
                    str[str_idx]='#';
                    str_idx++;
                    break;
                }
            default:  
                if ( state == 3 )
                {
                    str[str_idx]=(char)buf;
                    str_idx++;
                }
        }
        buf = fgetc (inf);
    }
    fclose(inf);
    return str;
}

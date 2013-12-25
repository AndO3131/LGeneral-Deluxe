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
extern int camp_loaded;

unsigned short UCS2_header=0xfeff;
unsigned short axis_experience=200;
unsigned short allied_experience=200;
unsigned short allies_move_first;

unsigned short axis_experience_table[] =
{

0,1,1,0,1,1,1,1,
1,2,2,3,2,2,2,2,
2,2,1,2,1,1,2,2,
2,2,2,2,2,3,2,2,
2,1,1,2,2,1
};

unsigned short allied_experience_table[] =
{
0,0,0,0,0,1,1,1,
1,1,1,1,1,2,2,2,
2,2,2,2,1,1,0,0,
0,1,1,1,1,2,2,2,
2,2,2,2,0,1
};

char axis_victory_table[][128]={
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t0\t3\t(27:5)",
"AXIS VICTORY\t0\t2\t",
"AXIS VICTORY\t0\t3\t(13:17)",
"AXIS VICTORY\t0\t2\t(11:7)(22:28)(43:27)",
"AXIS VICTORY\t0\t2\t",
"AXIS VICTORY\t0\t7\t(16:16)(26:4)(27:21)(39:21)(48:8)(54:14)(59:18)",
"AXIS VICTORY\t0\t3\t(50:5)(50:22)(44:35)(8:42)",
"AXIS VICTORY\t0\t2\t(37:10)(38:11)",
"AXIS VICTORY\t0\t6\t(36:14)",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t0\t1\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t0\t1\t(3:12)",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t0\t6\t(36:14)",
"AXIS VICTORY\t0\t1\t(35:14)",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t",
"AXIS VICTORY\t1\t0\t"
};

char default_purchasable_classes[]={
 1,1,1,1,1,
 1,1,0,1,1,
 1,0,0,0,0,
 1
};

unsigned short axis_prestige_allotments[PRESTIGE_ALLOTMENTS_NUMBER]={
0,0,0,0,0,0,0,0,
0,0,0,0,142,94,140,96,
0,96,122,140,0,0,0,0,
0,0,0,0,0,0,0,94,
0,140,164,0,0,0
};

unsigned short allied_prestige_allotments[PRESTIGE_ALLOTMENTS_NUMBER]={
40,40,80,98,90,168,90,140, //8
126,170,206,0,0,0,0,0, //16
150,0,0,0,202,90,120,160, //24
230,126,210,190,110,230,244,0, //32
0,0,0,230,270,170 // elements38
};

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
		//normal char
		if (digit>0 && digit<0x80) {
		  //protect from longlines
		  if (gcursor<1024){
			  line[gcursor]=(unsigned char)digit;
			  gcursor++;
		  }
		}
		//big char
		if (digit>=0x80 && digit<0x7ff){
			//we encode to UTF-8
			u1 = (digit & 0x03F) | 0x80;
			u2 = ((digit & 0x007C0) | 0x3000) >> 6;
			  if (gcursor<1024-2){
				  line[gcursor]=(unsigned char)u2;
				  gcursor++;
				  line[gcursor]=(unsigned char)u1;
				  gcursor++;
			  }
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
		if (digit==59) digit=9; //convert ; to tabs
		//normal char
		//if (digit>=0 && digit<=0xFF) {
		  //protect from longlines
		  if (gcursor<1024){
			  line[gcursor]=(unsigned char)digit;
			  gcursor++;
		  }
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


int load_pgf_equipment(char *fname){
	  FILE *inf;

	  char line[1024],tokens[50][256], log_str[256];
	  int i,cursor=0,token=0,lines;
	  //int total_victory,total_left,total_right;
	  int token_len, token_write, bmp_idx;
	  unsigned short temp;
	  unsigned short file_type_probe;
	  char path[MAX_PATH];
	  int utf16 = 0;

    snprintf( path, MAX_PATH, "%s/Scenario/%s", config.mod_name, fname );
	  inf=fopen(path,"rb");
	  if (!inf)
	  {
			strncpy(path,"../../default/scenario/",MAX_PATH);
			strncat(path,fname,MAX_PATH);
			inf=fopen(path,"rb");
			if (!inf)
				return ERROR_PGF_EQUIP_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	  }
	  lines=0;

	  //probe for UTF16 file magic bytes
	  fread(&file_type_probe, 2, 1, inf);
	  if (UCS2_header==file_type_probe) { utf16=1;}
	  fseek(inf,0,SEEK_SET);

    fprintf( stderr, "Loading PGF-style Unit Library '%s'\n", fname );
    sprintf( log_str, tr("Loading PGF-style Unit Library '%s'"), fname );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    unit_lib = list_create( LIST_AUTO_DELETE, unit_lib_delete_entry );

	  while (load_line(inf,line,utf16)>=0){
		  //count lines so error can be displayed with line number
		  lines++;
			  //strip comments
		  //printf("%d\n",0);

		  for(i=0;i<strlen(line);i++)
				  if (line[i]==0x23) { line[i]=0; break; }
			  //tokenize
			 // printf("%d\n",1);
			  token=0;
			  cursor=0;
			for (i = 0; i < strlen(line); i++)
				if (line[i] == 0x09) {
					tokens[token][cursor] = 0;
					token++;
					cursor = 0;
				} else {
					tokens[token][cursor] = line[i];
					cursor++;
				}
			  tokens[token][cursor]=0;
			  token++;
			  //printf("%d\n",2);
			  //printf("%d:",token);
			  //if (lines>110 && lines<120){
			  //for(i=0;i<token;i++)
			  //		printf("%s->", tokens[i]);
			  //printf("\n");
			  //}

			  //0x22
			  //remove quoting
			  if (tokens[1][0]==0x22){
				  //check ending quote
				  token_len=strlen(tokens[1]);

				  if (tokens[1][token_len-1]!=0x22){
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
				  for (i=0;i<token_len+1;i++){
					tokens[1][token_write]=tokens[1][i];
					if (tokens[1][i]==0x22 && tokens[1][i+1]==0x22) i++; //skip next char
					token_write++;
				  }
				  //all done
			  }
			  if (token==33){
			  // write back to normal equipment table
			  i=atoi(tokens[0]);
			  //printf("%d\n",i);

				  //printf("2\n");
/*				strncpy(equip[i], tokens[1], 20);
//				strncpy(equip_name_utf8[i], tokens[1],40);
				//already converted
				//convert_from_cp1250_to_utf8(equip_name_utf8[i], equip[i], 20);

				equip[i][CLASS] = (unsigned char) atoi(tokens[2]);
				//attack
				equip[i][SA] = (unsigned char) atoi(tokens[3]);
				equip[i][HA] = (unsigned char) atoi(tokens[4]);
				equip[i][AA] = (unsigned char) atoi(tokens[5]);
				equip[i][NA] = (unsigned char) atoi(tokens[6]);
				//Defense
				equip[i][GD] = (unsigned char) atoi(tokens[7]);
				equip[i][AD] = (unsigned char) atoi(tokens[8]);
				equip[i][CD] = (unsigned char) atoi(tokens[9]);

				equip[i][MOV_TYPE] = (unsigned char) atoi(tokens[10]);
				if (equip[i][MOV_TYPE] == MOV_TYPE_AIR) //air
					equip[i][GAF] = 1;
				else
					equip[i][GAF] = 0;
				equip[i][INITIATIVE] = (unsigned char) atoi(tokens[11]);
				equip[i][RANGE] = (unsigned char) atoi(tokens[12]);
				equip[i][SPOTTING] = (unsigned char) atoi(tokens[13]);

				equip[i][TARGET_TYPE] = (unsigned char) atoi(tokens[14]);
				equip[i][MOV] = (unsigned char) atoi(tokens[15]);
				equip[i][FUEL] = (unsigned char) atoi(tokens[16]);
				equip[i][AMMO] = (unsigned char) atoi(tokens[17]);
				equip[i][COST] = (unsigned char) ((int) atoi(tokens[18])
						/ COST_DIVISOR);
				bmp_idx = atoi(tokens[19]);
				equip[i][BMP] = bmp_idx & 0xff;
				equip[i][BMP+1] = (bmp_idx >> 8)& 0xff;

				temp = atoi(tokens[20]);
				equip[i][ANI] = temp& 0xff;
				equip[i][ANI+1] = (temp >> 8)& 0xff;

				equip[i][MON] = (unsigned char) atoi(tokens[21]);
				equip[i][YR] = (unsigned char) atoi(tokens[22]);
				equip[i][LAST_YEAR] = (unsigned char) atoi(tokens[23]);
				equip[i][AAF] = (unsigned char) atoi(tokens[24]);
				equip_flags[i] =0;
				equip_flags[i] |= (unsigned char) atoi(tokens[25])?EQUIPMENT_IGNORES_ENTRENCHMENT:0;
				equip_flags[i] |= (unsigned char) atoi(tokens[26])?EQUIPMENT_CAN_HAVE_ORGANIC_TRANSPORT:0;
				equip[i][ALLOWED_TRANSPORT]=0;
				if (atoi(tokens[27])==1) equip[i][ALLOWED_TRANSPORT]=1; //naval transport
				if (atoi(tokens[28])==1) equip[i][ALLOWED_TRANSPORT]=2; //air mobile transport
				if (atoi(tokens[29])==1) equip[i][ALLOWED_TRANSPORT]=3; //paradrop

				equip_flags[i] |= (unsigned char) atoi(tokens[30])?EQUIPMENT_CAN_BRIDGE_RIVERS:0;
				equip_flags[i] |= (unsigned char) atoi(tokens[31])?EQUIPMENT_JET:0;

				equip_country[i] = (char) atoi(tokens[32]);
*/
				//if (total_equip==388)
				//for(i=0;i<token;i++)
				 //printf("%s->", tokens[30]);


//				total_equip++;
		  }
	  }
	  fclose(inf);
	  fprintf( stderr, "All loaded.\n" );
	  return 0;
}

int load_pgf_pgscn(char *fullName){

    FILE *inf;
    char path[MAX_PATH];
    char line[1024],tokens[20][1024], log_str[256], SET_file[MAX_PATH], STM_file[MAX_PATH];
    int i,j,block=0,last_line_length=-1,cursor=0,token=0,x,y,error,lines;
    int air_trsp_player1, air_trsp_player2, sea_trsp_player1, sea_trsp_player2, total_victory,where_add_new;
    unsigned char t1,t2;
    WORD unum;

    scen_delete();
    player = 0;
    SDL_FillRect( sdl.screen, 0, 0x0 );
    log_font->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    log_x = 2; log_y = 2;
    scen_info = calloc( 1, sizeof( Scen_Info ) );
//    fprintf(stderr, "Opening file %s\n", fullName);
    scen_info->fname = strdup( fullName );
    inf=fopen(fullName,"rb");
    if (!inf)
    {
        //printf("Couldn't open scenario file\n");
        return ERROR_PGF_SCN_BASE+ERROR_FPGE_FILE_NOT_FOUND;
    }

    sprintf( log_str, tr("*** Loading scenario '%s' ***"), fullName );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
    //init
    //units

    int total_deploy=0;
    int total_left=0;
    int total_right=0;

    total_victory=0;
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
        //for(i=0;i<token;i++)
        //  printf("%s->",tokens[i]);

        //Block#1  +: General scenario data : 2 col, 14 rows
        if (block==1 && token>1)
        {
            if (token!=2)
                printf("Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,2,token);
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
                /* map and weather */
                strncpy( SET_file, tokens[1], 256 );
//                fprintf( stderr, "SET file:%s\n", SET_file );
/*                strncpy(tokens[2],"",1024);
                j=0;
                for(i=0;i<strlen(tokens[1]);i++)
                    if (tokens[1][i]>='0' && tokens[1][i]<='9')
                    {
                        tokens[2][j]=tokens[1][i];
                        j++;
                    }
                tokens[2][j]=0;
                pgf_map_number=atoi(tokens[2]);*/
                snprintf( log_str, strcspn( SET_file, "." ) + 1, "%s", SET_file );
                snprintf( STM_file, 256, "%s.STM", log_str );
//                fprintf( stderr, "MAP file:%s\n", STM_file );
                sprintf( log_str, tr("Loading Map '%s' and '%s'"), SET_file, STM_file );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
/*                if ( !map_load( tokens[1] ) )
                {
                    terrain_delete();
                    scen_delete();
                    if ( player ) player_delete( player );
                    return ERROR_PGF_EQUIP_BASE+ERROR_FPGE_FILE_NOT_FOUND;
                }*/
            }
            if (strcmp(tokens[0],"turns")==0)
                scen_info->turn_limit=(unsigned char)atoi(tokens[1]);
            if (strcmp(tokens[0],"year")==0)
                scen_info->start_date.year=(unsigned char)atoi(tokens[1]);
            if (strcmp(tokens[0],"month")==0)
                scen_info->start_date.month=(unsigned char)atoi(tokens[1]) - 1;
            if (strcmp(tokens[0],"day")==0)
                scen_info->start_date.day=(unsigned char)atoi(tokens[1]);
            if (strcmp(tokens[0],"days per turn")==0)
                scen_info->days_per_turn=(unsigned char)atoi(tokens[1]);
            if (strcmp(tokens[0],"turns per day")==0)
                scen_info->turns_per_day=(unsigned char)atoi(tokens[1]);
/*            if (strcmp(tokens[0],"weather zone")==0)
                scn_buffer[SCEN_LOCALE]=(unsigned char)atoi(tokens[1]);
            if (strcmp(tokens[0],"current weather")==0)
                if (probe_file_only!=SCAN_FOR_MAP_NUMBER) scn_buffer[STORM_FRONT]=(unsigned char)atoi(tokens[1]);
            if (strcmp(tokens[0],"max unit strength")==0)
                if (probe_file_only!=SCAN_FOR_MAP_NUMBER) strncpy(block1_Max_Unit_Strength,tokens[1],256);
            if (strcmp(tokens[0],"max unit experience")==0)
                if (probe_file_only!=SCAN_FOR_MAP_NUMBER) strncpy(block1_Max_Unit_Experience,tokens[1],256);
*/
            allies_move_first=0;
            if (strcmp(tokens[0],"allies move first")==0)
                allies_move_first=atoi(tokens[1]);
        }
        //Block#2  +: Sides : 11 col, 2 rows
        if (block==2 && token>1)
        {
            if (token!=11)
                printf("Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,11,token);
            if (atoi(tokens[0])==0)
            {
                /* players */
                sprintf( log_str, tr("Loading Players") );
                write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );
                scen_info->player_count = 2;

                /* create player */
                player = calloc( 1, sizeof( Player ) );

                if ( allies_move_first == 0 )
                {
                    player->id = strdup( "axis" );
                    player->name = strdup(trd(domain, "Axis"));
                }
                else
                {
                    player->id = strdup( "allies" );
                    player->name = strdup(trd(domain, "Allies"));
                }
                player->ctrl = PLAYER_CTRL_HUMAN;
                //TODO make this shifts >> below more clear and implement it in right way
                if ((camp_loaded != NO_CAMPAIGN) && config.use_core_units)
                {
                    player->core_limit = (unsigned char)atoi(tokens[2]);
                }
                else
                    player->core_limit = -1;
//                fprintf( stderr, "Core limit:%d\n", player->core_limit );

                player->unit_limit = (unsigned char)atoi(tokens[3]);
                if ( player->core_limit >= 0 )
                    player->unit_limit += player->core_limit;
                else
                    player->unit_limit += (unsigned char)atoi(tokens[2]);
//                fprintf( stderr, "Unit limit:%d\n", player->unit_limit );
                player->strat = (unsigned char)atoi(tokens[4]);
//                fprintf( stderr, "Strategy:%d\n", player->strat );

                player->air_trsp_count = (unsigned char)atoi(tokens[8]);
//                fprintf( stderr, "Air transport count: %d\n", player->air_trsp_count );
                air_trsp_player1 = (unsigned char)atoi(tokens[9]);
//                fprintf( stderr, "Air transport type: %d\n", air_trsp_player1 );
//                s4_buffer[AXIS_AIR_TYPE+1]=(unsigned char)(atoi(tokens[9])>>8);

                player->sea_trsp_count = (unsigned char)atoi(tokens[6]);
//                fprintf( stderr, "Sea transport count: %d\n", player->sea_trsp_count );
                sea_trsp_player1 = (unsigned char)atoi(tokens[7]);
//                fprintf( stderr, "Sea transport type: %d\n", sea_trsp_player1 );
//                s4_buffer[AXIS_SEA_TYPE+1]=(unsigned char)(atoi(tokens[7])>>8);

                player->prestige_per_turn = calloc( scen_info->turn_limit, sizeof(int));
                player->prestige_per_turn[0] = (unsigned char)atoi(tokens[1]);
//                fprintf( stderr, "Prestige: %d\n", player->prestige_per_turn[0] );
//                s4_buffer[AXIS_PRESTIGE+1]=(unsigned char)(atoi(tokens[1])>>8);


//                axis_experience = atoi(tokens[10]);
            }
            if (atoi(tokens[0])==1)
            {
                /* create player */
                player = calloc( 1, sizeof( Player ) );

                if ( allies_move_first == 1 )
                {
                    player->id = strdup( "axis" );
                    player->name = strdup(trd(domain, "Axis"));
//                    fprintf( stderr, "2nd player: %s\n", player->name );
                }
                else
                {
                    player->id = strdup( "allies" );
                    player->name = strdup(trd(domain, "Allies"));
//                    fprintf( stderr, "2nd player: %s\n", player->name );
                }
                player->ctrl = PLAYER_CTRL_CPU;
                player->core_limit = -1;

/*                if (atoi(tokens[5])==1)
                {
                    scn_buffer[ORIENTATION]=1;
                    scn_buffer[ORIENTATION+1]=0;
                }
                else
                {
                    scn_buffer[ORIENTATION]=255;
                    scn_buffer[ORIENTATION+1]=255;
                }
*/
//                fprintf( stderr, "Core limit:%d\n", player->core_limit );

                player->unit_limit = (unsigned char)atoi(tokens[3]);
//                fprintf( stderr, "Unit limit:%d\n", player->unit_limit );
                player->strat = (unsigned char)atoi(tokens[4]);
//                fprintf( stderr, "Strategy:%d\n", player->strat );

                player->air_trsp_count = (unsigned char)atoi(tokens[8]);
//                fprintf( stderr, "Air transport count: %d\n", player->air_trsp_count );
                air_trsp_player1 = (unsigned char)atoi(tokens[9]);
//                fprintf( stderr, "Air transport type: %d\n", air_trsp_player1 );
//                s4_buffer[ALLIED_AIR_TYPE+1]=(unsigned char)(atoi(tokens[9])>>8);

                player->sea_trsp_count = (unsigned char)atoi(tokens[6]);
//                fprintf( stderr, "Sea transport count: %d\n", player->sea_trsp_count );
                sea_trsp_player1 = (unsigned char)atoi(tokens[7]);
//                fprintf( stderr, "Sea transport type: %d\n", sea_trsp_player1 );
//                s4_buffer[ALLIED_SEA_TYPE+1]=(unsigned char)(atoi(tokens[7])>>8);

                player->prestige_per_turn = calloc( scen_info->turn_limit, sizeof(int));
                player->prestige_per_turn[0] = (unsigned char)atoi(tokens[1]);
//                fprintf( stderr, "Prestige: %d\n", player->prestige_per_turn[0] );
//                s4_buffer[ALLIED_PRESTIGE+1]=(unsigned char)(atoi(tokens[1])>>8);

//                allied_experience = atoi(tokens[10]);

                player_add( player ); player = 0;
            }
        }
			  //Block#3  +: Nations: 2 col, 2 or more rows
/*			  if (block==3 && token>1){
				  if (token!=2){
				 	printf("Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,2,token);

				  }else{
				  t1=atoi(tokens[0]);
				  t2=atoi(tokens[1]);

				  if(t2==0) { scn_buffer[total_left*2]=t1;  total_left++;}
				  if(t2==1) { scn_buffer[total_right*2+1]=t1;  total_right++;}

				  if (total_left>6 || total_right>6)
					  printf("Error. Line %d. Too many nations (max=6 per side).\n",lines);
				  }
			  }
			  //Block#4   : Per-turn prestige allotments: 3 col, rows = no of turns
			  if (block==4 && token>1){
				  if (block4_lines>MAX_TURNS)
					  printf("Error. Line %d. Too many lines in 'Per-turn prestige allotments' block.\n",lines);
				  else{
					  strncpy(block4[block4_lines],line,MAX_LINE_SIZE);
					  block4_lines++;
				  }
			  }
			  //Block#5  ?: Supply hexes: 2 col, rows - many
			  if (block == 5 && token > 1) {
				if (block5_lines > MAX_SUPPLY)
					printf(
							"Error. Line %d. Too many lines in 'Supply hexes' block.\n",
							lines);
				else {
					strncpy(block5[block5_lines], line,MAX_LINE_SIZE);
					block5_lines++;
				}
			}
			  //Block#6  +: Victory hexes: 1 col, rows - many, limit 20
			  if (block==6 && strlen(tokens[0])>0){
				  if (token!=1)
				 	printf("Error. Line %d. Victory hexes. Expected no of columns %d while %d columns detected.\n",lines,1,token);

				  error=sscanf(tokens[0],"(%8d:%8d)",&x,&y);
				  //some protection
				  if (x>=0 && y>=0 && x<mapx && y<mapy){
					  victory_hexes[total_victory].x=x;
					  victory_hexes[total_victory].y=y;
					  victory_hexes[total_victory].own=map[x][y].own;

					  map[x][y].vic=1;
					  total_victory++;
				  }
			  }
			  //Block#7   : Victory conditions: 3 col, rows 2
			  if (block == 7 && token > 1) {
				if (block7_lines > MAX_VICTORY_CON)
					printf(
							"Error. Line %d. Too many lines in 'Victory conditions' block.\n",
							lines);
				else {
					strncpy(block7[block7_lines], line,MAX_LINE_SIZE);
					block7_lines++;
				}
			}
			  //Block#8  +: Deploy hexes: 1 col, rows - many, limit 80
			  if (block==8 && strlen(tokens[0])>0){
				  if (token!=1)
				 	printf("Error. Line %d. Expected no of columns %d while %d columns detected.\n",lines,1,token);

				  error=sscanf(tokens[0],"(%8d:%8d)",&x,&y);
				  if (error==2 && x>=0 && y>=0 && x<mapx && y<mapy){
					  map[x][y].deploy=1;
					  total_deploy++;
				  }
			  }
			  //Block#9   : Purchasable classes: 2 col, rows up to 16 ?
			  if (block == 9 && token > 1) {
				if (block9_lines > MAX_CLASSES)
					printf(
							"Error. Line %d. Too many lines in 'Purchasable classes' block.\n",
							lines);
				else {
					strncpy(block9[block9_lines], line,MAX_LINE_SIZE);
					block9_lines++;
				}
			}
			  //Block#10 +: Units: 12 col, rows - many, limit
			  if (block==10 && token>1){
				  //for(i=0;i<token;i++)
					//  printf("%s->",tokens[i]);
				  //printf(":%d:%d\n",block,token);

				  if (token!=12)
					  printf("Error. Line %d. Units. Expected no of columns %d while %d columns detected.\n",lines,12,token);

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

				  where_add_new=add_unit_type(atoi(tokens[4]),atoi(tokens[11]));

				    error=sscanf(tokens[0],"(%8d:%8d)",&x,&y);
				    if (x>=mapx || y>=mapy || x<0 || y<0) //this is an error!
			         {
						 printf("Warning. Ignoring line %d. Check unit x,y !\n",lines);
			         }
				    else
				    {
						all_units[where_add_new].unum=atoi(tokens[1]);
						all_units[where_add_new].orgtnum=atoi(tokens[2]);
						all_units[where_add_new].country=atoi(tokens[5]);
						all_units[where_add_new].auxtnum=atoi(tokens[3]);

						all_units[where_add_new].x=x;
						all_units[where_add_new].y=y;
						all_units[where_add_new].str=atoi(tokens[6]);
						all_units[where_add_new].exp=atoi(tokens[7])/100;
						all_units[where_add_new].entrench=atoi(tokens[8]);
						all_units[where_add_new].fuel=atoi(tokens[9]);
						all_units[where_add_new].ammo=atoi(tokens[10]);

						//place on map
						 unum=all_units[where_add_new].unum;
						 if (equip[unum][GAF]) //1 if air unit
						   map[x][y].auidx=total_units;
						 else
						   map[x][y].guidx=total_units;

						total_units++;
				    }
			  }
			 //printf(":%d:%d\n",block,token);
			  //printf("%d:%s\n",block,line);
			  continue;
*/
    }

    fclose(inf);
/*
		scn_buffer[CORE_UNITS]=total_axis_core;
		scn_buffer[ALLIED_UNITS]=total_allied_core;
		scn_buffer[AUX_UNITS]=total_axis_aux;
		scn_buffer[AUX_ALLIED_UNITS]=total_allied_aux;*/
/*
		printf("total_units=%d\n",total_units);
		printf("total_axis_core=%d\n",total_axis_core);
		printf("total_axis_aux=%d\n",total_axis_aux);
		printf("total_allied_core=%d\n",total_allied_core);
		printf("total_allied_aux=%d\n",total_allied_aux);
*/
	 // printf("block4_lines=%d\nblock5_lines=%d\nblock7_lines=%d\nblock9_lines=%d\n",block4_lines,block5_lines,block7_lines,block9_lines);
    sprintf( log_str, "PGF scenario %s loaded.\n", fullName );
    write_line( sdl.screen, log_font, log_str, log_x, &log_y ); refresh_screen( 0, 0, 0, 0 );

    return 0;
}

char *load_pgf_pgscn_info( const char *fname, const char *path )
{
    FILE *inf;
    char line[1024],tokens[20][1024], temp[MAX_PATH];//, log_str[256];
    int i, block=0,last_line_length=-1,cursor=0,token=0,lines;//,x,y,error,j;
    char name[256], desc[1024], turns[10], *info;//, *str, day[10], month[15], year[10], info[1024];

    scen_delete();
    search_file_name( path, 0, fname, temp, 'o' );
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
            if (strcmp(tokens[0],"description")==0)
                strncpy(desc,tokens[1],1024);
            if (strcmp(tokens[0],"turns")==0)
                strncpy(turns,tokens[1],10);
            allies_move_first=0;
            if (strcmp(tokens[0],"allies move first")==0)
            {
                allies_move_first=atoi(tokens[1]);//block1_Allies_Move_First);
            }
        }
        if (block==2)
        {
            fclose(inf);
        	/* set setup */
        	scen_clear_setup();
        	strcpy( setup.fname, fname );
        	setup.player_count = 2;
        	setup.ctrl = calloc( setup.player_count, sizeof( int ) );
        	setup.names = calloc( setup.player_count, sizeof( char* ) );
        	setup.modules = calloc( setup.player_count, sizeof( char* ) );
        	/* load the player ctrls */
        	if ( allies_move_first == 0 )
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
            info = calloc( 1024, sizeof(char) );
            snprintf( info, 1024, tr("%s##%s##%s Turns#%d Players"), name, desc, turns, 2 );
            return info;
        }
    }
    fclose(inf);
    return 0;
}

/*
int load_bmp_tacticons(){
	FILE *inf;
	char path[MAX_PATH];
	BITMAP *new_map_bitmap;
	int x,y,dx,dy,i,j,c,pink;

	strncpy(path,"..\\graphics\\",MAX_PATH);
	strncat(path,pgf_units_bmp,MAX_PATH);
	canonicalize_filename(path,path,MAX_PATH);
	inf=fopen(path,"rb");
	if (!inf){
		strncpy(path,"..\\..\\default\\graphics\\",MAX_PATH);
		strncat(path,pgf_units_bmp,MAX_PATH);
		canonicalize_filename(path,path,MAX_PATH);
		inf=fopen(path,"rb");
		if (!inf)
			return ERROR_PGF_BMP_TACICONS_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	}
	fclose(inf);
	//all ok, load file
	if(fpge_colors_bits_shift) pink=0xe1e1ff+(fpge_colors_bits_shift==2?0xff000000:0);
	else pink=0xffe1e1;
	new_map_bitmap = load_bmp(path, NULL);
	//printf("%d %d %d\n",new_map_bitmap->w,new_map_bitmap->h, bitmap_color_depth(new_map_bitmap));

	if (new_map_bitmap->w%TILE_FULL_WIDTH) return ERROR_PGF_BMP_TACICONS_BASE+ERROR_FPGE_WRONG_BMP_SIZE_X;
	if (new_map_bitmap->h%50) return ERROR_PGF_BMP_TACICONS_BASE+ERROR_FPGE_WRONG_BMP_SIZE_Y;

	dx=new_map_bitmap->w/TILE_FULL_WIDTH;
	dy=new_map_bitmap->h/TILE_HEIGHT;
	//printf("%d %d\n",dx,dy);

	for(y=0;y<dy;y++)
		for(x=0;x<dx;x++){
			//proxy_bitmap=create_sub_bitmap(new_map_bitmap,x*dx,y*dy,60,50);
			unit_bmp[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			for(i=0;i<TILE_HEIGHT;i++)
				for(j=0;j<TILE_FULL_WIDTH;j++){
					c=getpixel(new_map_bitmap,x*TILE_FULL_WIDTH+j,y*TILE_HEIGHT+i);
					c=make_color_fpge(c&0xff,(c>>8)&0xff,(c>>16)&0xff);
					//if (x+y*dx==146) printf("%d %d %06x\n",j,i,c);
					if (c!=pink)
						putpixel(unit_bmp[x+y*dx],j,i,c);
					else
						putpixel(unit_bmp[x+y*dx],j,i,fpge_mask_color);
				}
		}

	total_uicons=dx*dy;

	destroy_bitmap(new_map_bitmap);
	return 0;
}

int load_bmp_tacmap(){
	FILE *inf;
	char path_dry[MAX_PATH], path_muddy[MAX_PATH], path_frozen[MAX_PATH];
	BITMAP *bitmap_dry, *bitmap_muddy, *bitmap_frozen;
	int x,y,dx,dy,i,j,oc,c,dc,found,pink;

	//dry
	strncpy(path_dry,"..\\graphics\\",MAX_PATH);
	strncat(path_dry,pgf_tacmap_dry_bmp,MAX_PATH);
	canonicalize_filename(path_dry,path_dry,MAX_PATH);
	inf=fopen(path_dry,"rb");
	if (!inf){
		strncpy(path_dry,"..\\..\\default\\graphics\\",MAX_PATH);
		strncat(path_dry,pgf_tacmap_dry_bmp,MAX_PATH);
		canonicalize_filename(path_dry,path_dry,MAX_PATH);
		inf=fopen(path_dry,"rb");
		if (!inf)
			return ERROR_PGF_BMP_TACMAP_DRY_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	}
	fclose(inf);

	//muddy
	strncpy(path_muddy,"..\\graphics\\",MAX_PATH);
	strncat(path_muddy,pgf_tacmap_muddy_bmp,MAX_PATH);
	canonicalize_filename(path_muddy,path_muddy,MAX_PATH);
	inf=fopen(path_muddy,"rb");
	if (!inf){
		strncpy(path_muddy,"..\\..\\default\\graphics\\",MAX_PATH);
		strncat(path_muddy,pgf_tacmap_muddy_bmp,MAX_PATH);
		canonicalize_filename(path_muddy,path_muddy,MAX_PATH);
		inf=fopen(path_muddy,"rb");
		if (!inf)
			return ERROR_PGF_BMP_TACMAP_MUDDY_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	}
	fclose(inf);

	//frozen
	strncpy(path_frozen,"..\\graphics\\",MAX_PATH);
	strncat(path_frozen,pgf_tacmap_frozen_bmp,MAX_PATH);
	canonicalize_filename(path_frozen,path_frozen,MAX_PATH);
	inf=fopen(path_frozen,"rb");
	if (!inf){
		strncpy(path_frozen,"..\\..\\default\\graphics\\",MAX_PATH);
		strncat(path_frozen,pgf_tacmap_frozen_bmp,MAX_PATH);
		canonicalize_filename(path_frozen,path_frozen,MAX_PATH);
		inf=fopen(path_frozen,"rb");
		if (!inf)
			return ERROR_PGF_BMP_TACMAP_FROZEN_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	}
	fclose(inf);

	if(fpge_colors_bits_shift) pink=0xe1e1ff+(fpge_colors_bits_shift==2?0xff000000:0);
	else pink=0xffe1e1;

	//all ok, load all files
	bitmap_dry = load_bmp(path_dry, NULL);
	bitmap_muddy = load_bmp(path_muddy, NULL);
	bitmap_frozen = load_bmp(path_frozen, NULL);

	//printf("%d %d %d\n",new_map_bitmap->w,new_map_bitmap->h, bitmap_color_depth(new_map_bitmap));

	//check if w%60==0 and h%50==0
	if (bitmap_dry->w%TILE_FULL_WIDTH) return ERROR_PGF_BMP_TACMAP_DRY_BASE+ERROR_FPGE_WRONG_BMP_SIZE_X;
	if (bitmap_dry->h%TILE_HEIGHT) return ERROR_PGF_BMP_TACMAP_DRY_BASE+ERROR_FPGE_WRONG_BMP_SIZE_Y;
	if (bitmap_muddy->w%TILE_FULL_WIDTH) return ERROR_PGF_BMP_TACMAP_MUDDY_BASE+ERROR_FPGE_WRONG_BMP_SIZE_X;
	if (bitmap_muddy->h%TILE_HEIGHT) return ERROR_PGF_BMP_TACMAP_MUDDY_BASE+ERROR_FPGE_WRONG_BMP_SIZE_Y;
	if (bitmap_frozen->w%TILE_FULL_WIDTH) return ERROR_PGF_BMP_TACMAP_FROZEN_BASE+ERROR_FPGE_WRONG_BMP_SIZE_X;
	if (bitmap_frozen->h%TILE_HEIGHT) return ERROR_PGF_BMP_TACMAP_FROZEN_BASE+ERROR_FPGE_WRONG_BMP_SIZE_Y;

	//check if they are the same size
	if (bitmap_dry->w!=bitmap_muddy->w) return ERROR_PGF_BMP_TACMAP_DRY_BASE+ERROR_FPGE_DRY_X_DIFF_MUDDY_X;
	if (bitmap_dry->w!=bitmap_frozen->w)return ERROR_PGF_BMP_TACMAP_DRY_BASE+ERROR_FPGE_DRY_X_DIFF_FROZEN_X;

	if (bitmap_dry->h!=bitmap_muddy->h) return ERROR_PGF_BMP_TACMAP_DRY_BASE+ERROR_FPGE_DRY_Y_DIFF_MUDDY_Y;
	if (bitmap_dry->h!=bitmap_frozen->h)return ERROR_PGF_BMP_TACMAP_DRY_BASE+ERROR_FPGE_DRY_Y_DIFF_FROZEN_Y;

	dx=bitmap_dry->w/TILE_FULL_WIDTH;
	dy=bitmap_dry->h/TILE_HEIGHT;
	//printf("%d %d\n",dx,dy);

	for(y=0;y<dy;y++)
		for(x=0;x<dx;x++){
			//proxy_bitmap=create_sub_bitmap(new_map_bitmap,x*dx,y*dy,60,50);
			til_bmp[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			dark_til_bmp[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			til_bmp_mud[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			dark_til_bmp_mud[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			til_bmp_snow[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			dark_til_bmp_snow[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			for(i=0;i<TILE_HEIGHT;i++)
				for(j=0;j<TILE_FULL_WIDTH;j++){
					//dry
					oc=getpixel(bitmap_dry,x*TILE_FULL_WIDTH+j,y*TILE_HEIGHT+i);
					c=make_color_fpge(oc&0xff,(oc>>8)&0xff,(oc>>16)&0xff);
					dc=make_color_fpge( (oc&0xff)*3/4,((oc>>8)&0xff)*3/4,((oc>>16)&0xff)*3/4);

					//if (x+y*dx==146) printf("%d %d %06x\n",j,i,c);
					if (c!=pink){
						if (draw_app6_tiles){
						//int c_idx = find_pal_element92(c);
						if (c==colors_to24bits(92) || c==colors_to24bits(75) || c==colors_to24bits(76)
						 || c==colors_to24bits(91) || c==colors_to24bits(93)|| c==colors_to24bits(90)){
						c=0;
						}
						if (    draw_app6_tiles==1||(
								  c!=colors_to24bits(42)
								&&c!=colors_to24bits(127)
								&&c!=colors_to24bits(89)
								&&c!=colors_to24bits(65)
								&&c!=colors_to24bits(49)
								&&c!=colors_to24bits(48)
								&&c!=colors_to24bits(47)
								&&c!=colors_to24bits(46)
								&&c!=colors_to24bits(45)
								)){ //42=river, 127=road, 89=forest 87-mountain 65-city 45-49-sea
							//green
							if (draw_app6_color==0){
								c &= 0xff00;
								c = c >> 1;
								c &= 0xff00;
							}
							else
							{
								//grey
								int r= (c&0xff0000)>>16;
								int g= (c&0xff00)>>8;
								int b= (c&0xff);
								int m =  ((r+g+b)/3)>>1;
								r= m<<16;
								g= m<<8;
								b=m;
								c=r+g+b;
							}

							}else{
								if (c==colors_to24bits(89)){
									//medium green
									c=make_color_fpge(0,128+32,0);
								}else{
									if (c==colors_to24bits(65)){
										//white, for city/ports
										c=make_color_fpge(255,255,255);
									}else{
										if (c==colors_to24bits(49)){
											c=make_color_fpge(131>>3,147>>3,199>>2);
										}else{
										//make brigther
									c &=0x7f7f7f;
									c =c <<1;
									}
								}
								}
							}
						}//app6

						putpixel(til_bmp[x+y*dx],j,i,c);
						putpixel(dark_til_bmp[x+y*dx],j,i,dc);
						}
					else
					{
						putpixel(til_bmp[x+y*dx],j,i,fpge_mask_color);
						putpixel(dark_til_bmp[x+y*dx],j,i,fpge_mask_color);
					}
					//muddy
					oc=getpixel(bitmap_muddy,x*TILE_FULL_WIDTH+j,y*TILE_HEIGHT+i);
					c=make_color_fpge(oc&0xff,(oc>>8)&0xff,(oc>>16)&0xff);
					dc=make_color_fpge( (oc&0xff)*3/4,((oc>>8)&0xff)*3/4,((oc>>16)&0xff)*3/4);

					//if (x+y*dx==146) printf("%d %d %06x\n",j,i,c);
					if (c!=pink){
						putpixel(til_bmp_mud[x+y*dx],j,i,c);
						putpixel(dark_til_bmp_mud[x+y*dx],j,i,dc);
						}
					else{
						putpixel(til_bmp_mud[x+y*dx],j,i,fpge_mask_color);
						putpixel(dark_til_bmp_mud[x+y*dx],j,i,fpge_mask_color);
					}
					//frozen
					oc=getpixel(bitmap_frozen,x*TILE_FULL_WIDTH+j,y*TILE_HEIGHT+i);
					c=make_color_fpge(oc&0xff,(oc>>8)&0xff,(oc>>16)&0xff);
					dc=make_color_fpge( (oc&0xff)*3/4,((oc>>8)&0xff)*3/4,((oc>>16)&0xff)*3/4);

					//if (x+y*dx==146) printf("%d %d %06x\n",j,i,c);
					if (c!=pink){
						putpixel(til_bmp_snow[x+y*dx],j,i,c);
						putpixel(dark_til_bmp_snow[x+y*dx],j,i,dc);
						}
					else{
						putpixel(til_bmp_snow[x+y*dx],j,i,fpge_mask_color);
						putpixel(dark_til_bmp_snow[x+y*dx],j,i,fpge_mask_color);
					}
				}
		}

	total_tiles=dx*dy;
//kill last empty tiles
	found=0;
	for(x=dx*dy-1;x>=0;x--){
		for(i=0;i<TILE_HEIGHT;i++)
			for(j=0;j<TILE_FULL_WIDTH;j++){
				oc=getpixel(til_bmp[x],j,i);
				if (oc != fpge_mask_color) { found=1; break; }
			}
		if (!found) { destroy_bitmap(til_bmp[x]); total_tiles--; }
		else break; //break i
    }

	destroy_bitmap(bitmap_dry);
	destroy_bitmap(bitmap_muddy);
	destroy_bitmap(bitmap_frozen);
	return 0;
}

int load_bmp_flags(){
	FILE *inf;
	char path[MAX_PATH];
	BITMAP *flags_bitmap,*tmp_bmp;
	int x,y,dx,dy,i,j,c,pink;

	strncpy(path,"..\\graphics\\",MAX_PATH);
	strncat(path,pgf_flags_bmp,MAX_PATH);

	canonicalize_filename(path,path,MAX_PATH);
	inf=fopen(path,"rb");
	if (!inf){
		strncpy(path,"..\\..\\default\\graphics\\",MAX_PATH);
		strncat(path,pgf_flags_bmp,MAX_PATH);
		canonicalize_filename(path,path,MAX_PATH);
		inf=fopen(path,"rb");
		if (!inf)
			return ERROR_PGF_BMP_FLAGS_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	}
	fclose(inf);
	if(fpge_colors_bits_shift) pink=0xe1e1ff+(fpge_colors_bits_shift==2?0xff000000:0);
	else pink=0xffe1e1;

	//all ok, load file
	flags_bitmap = load_bmp(path, NULL);
	//printf("%d %d %d\n",new_map_bitmap->w,new_map_bitmap->h, bitmap_color_depth(new_map_bitmap));

	if (flags_bitmap->w%TILE_FULL_WIDTH) return ERROR_PGF_BMP_FLAGS_BASE+ERROR_FPGE_WRONG_BMP_SIZE_X;
	if (flags_bitmap->h%TILE_HEIGHT) return ERROR_PGF_BMP_FLAGS_BASE+ERROR_FPGE_WRONG_BMP_SIZE_Y;

	dx=flags_bitmap->w/TILE_FULL_WIDTH;
	dy=flags_bitmap->h/TILE_HEIGHT;
	//printf("%d %d\n",dx,dy);

	for(y=0;y<dy;y++)
		for(x=0;x<dx;x++){
			//proxy_bitmap=create_sub_bitmap(new_map_bitmap,x*dx,y*dy,60,50);
			tmp_bmp=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			for(i=0;i<TILE_HEIGHT;i++)
				for(j=0;j<TILE_FULL_WIDTH;j++){
					c=getpixel(flags_bitmap,x*TILE_FULL_WIDTH+j,y*TILE_HEIGHT+i);
					c=make_color_fpge(c&0xff,(c>>8)&0xff,(c>>16)&0xff);
					//if (x+y*dx==146) printf("%d %d %06x\n",j,i,c);
					if (c!=pink)
						putpixel(tmp_bmp,j,i,c);
					else
						putpixel(tmp_bmp,j,i,fpge_mask_color);
				}

			flag_bmp[x+y*dx]=tmp_bmp;
		}

	total_flags=dx*dy;
	//printf("%d\n",total_flags);

	destroy_bitmap(flags_bitmap);
	return 0;
}

int load_bmp_strength(){
	FILE *inf;
	char path[MAX_PATH];
	BITMAP *strength_bitmap;
	int x,y,dx,dy,i,j,c,h=0,pink;

	strncpy(path,"..\\graphics\\",MAX_PATH);
	strncat(path,pgf_strength_bmp,MAX_PATH);
	canonicalize_filename(path,path,MAX_PATH);
	inf=fopen(path,"rb");
	if (!inf){
		strncpy(path,"..\\..\\default\\graphics\\",MAX_PATH);
		strncat(path,pgf_strength_bmp,MAX_PATH);
		canonicalize_filename(path,path,MAX_PATH);
		inf=fopen(path,"rb");
		if (!inf)
			return ERROR_PGF_BMP_STRENGTH_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	}
	fclose(inf);
	if(fpge_colors_bits_shift) pink=0xe1e1ff+(fpge_colors_bits_shift==2?0xff000000:0);
	else pink=0xffe1e1;
	//all ok, load file
	//printf("01\n");
	strength_bitmap = load_bmp(path, NULL);
	//printf("%d %d %d\n",new_map_bitmap->w,new_map_bitmap->h, bitmap_color_depth(new_map_bitmap));

	if (strength_bitmap->w%TILE_FULL_WIDTH) {
		destroy_bitmap(strength_bitmap);
		return ERROR_PGF_BMP_STRENGTH_BASE+ERROR_FPGE_WRONG_BMP_SIZE_X;
	}
	if (strength_bitmap->h%TILE_HEIGHT) {
		destroy_bitmap(strength_bitmap);
		return ERROR_PGF_BMP_STRENGTH_BASE+ERROR_FPGE_WRONG_BMP_SIZE_Y;
	}
	dx=strength_bitmap->w/TILE_FULL_WIDTH;
	//dy=strength_bitmap->h/TILE_HEIGHT;
	//printf("%d %d\n",dx,dy);
	//printf("001\n");

	//we will read only 4 rows
	dy=4;
	for(y=0;y<dy;y++)
		for(x=0;x<dx;x++){
			//proxy_bitmap=create_sub_bitmap(new_map_bitmap,x*dx,y*dy,60,50);
			if (x==0) h++; //skip 0 counter
			//printf("0001\n");
			strength_bmp[h]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			//printf("0001\n");

			for(i=0;i<TILE_HEIGHT;i++)
				for(j=0;j<TILE_FULL_WIDTH;j++){
					c=getpixel(strength_bitmap,x*TILE_FULL_WIDTH+j,y*TILE_HEIGHT+i);
					c=make_color_fpge(c&0xff,(c>>8)&0xff,(c>>16)&0xff);
					//if (x+y*dx==146) printf("%d %d %06x\n",j,i,c);
					if (c!=pink)
						putpixel(strength_bmp[h],j,i,c);
					else
						putpixel(strength_bmp[h],j,i,fpge_mask_color);
				}
			h++;
		}
	//printf("1\n");
	//0 counters are not created
	//make 0 counters out of 10 counter
	if (!str0_bmp_loaded){
		for (h = 0; h < 4; h++){
			strength_bmp[0+MAX_STRENGTH_IN_ROW*h]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);

			for (i = 0; i < TILE_HEIGHT; i++)
						for (j = 0; j < TILE_FULL_WIDTH; j++)
							putpixel(strength_bmp[h * MAX_STRENGTH_IN_ROW], j, i,
									getpixel(strength_bmp[h * MAX_STRENGTH_IN_ROW + 10], j, i));

			c=getpixel(strength_bmp[0+MAX_STRENGTH_IN_ROW*h],23,37);
			rectfill(strength_bmp[0+MAX_STRENGTH_IN_ROW*h],24,38,29,46,c);

			for(i=38;i<45;i++)
			  for(j=28;j<28+6;j++)
				  putpixel(strength_bmp[MAX_STRENGTH_IN_ROW*h],j,i,getpixel(strength_bmp[MAX_STRENGTH_IN_ROW*h],j+2,i));
		}
	}
	destroy_bitmap(strength_bitmap);
	return 0;
}

int load_bmp_stackicn(){
	FILE *inf;
	char path[MAX_PATH];
	BITMAP *stack_bitmap;
	int x,y,dx,dy,i,j,c,found,pink;

	strncpy(path,"..\\graphics\\",MAX_PATH);
	strncat(path,pgf_stackicn_bmp,MAX_PATH);
	canonicalize_filename(path,path,MAX_PATH);
	inf=fopen(path,"rb");
	if (!inf){
		strncpy(path,"..\\..\\default\\graphics\\",MAX_PATH);
		strncat(path,pgf_stackicn_bmp,MAX_PATH);
		canonicalize_filename(path,path,MAX_PATH);
		inf=fopen(path,"rb");
		if (!inf)
			return ERROR_PGF_BMP_STACKICN_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	}
	fclose(inf);
	if(fpge_colors_bits_shift) pink=0xe1e1ff+(fpge_colors_bits_shift==2?0xff000000:0);
	else pink=0xffe1e1;
	//all ok, load file
	stack_bitmap = load_bmp(path, NULL);
	//printf("%d %d %d\n",new_map_bitmap->w,new_map_bitmap->h, bitmap_color_depth(new_map_bitmap));

	if (stack_bitmap->w%TILE_FULL_WIDTH) return ERROR_PGF_BMP_STACKICN_BASE+ERROR_FPGE_WRONG_BMP_SIZE_X;
	if (stack_bitmap->h%TILE_HEIGHT) return ERROR_PGF_BMP_STACKICN_BASE+ERROR_FPGE_WRONG_BMP_SIZE_Y;

	dx=stack_bitmap->w/TILE_FULL_WIDTH;
	dy=stack_bitmap->h/TILE_HEIGHT;
	//printf("%d %d\n",dx,dy);

	for(y=0;y<dy;y++)
		for(x=0;x<dx;x++){
			//proxy_bitmap=create_sub_bitmap(new_map_bitmap,x*dx,y*dy,60,50);
			stack_bmp[x+y*dx]=create_bitmap(TILE_FULL_WIDTH,TILE_HEIGHT);
			for(i=0;i<TILE_HEIGHT;i++)
				for(j=0;j<TILE_FULL_WIDTH;j++){
					c=getpixel(stack_bitmap,x*TILE_FULL_WIDTH+j,y*TILE_HEIGHT+i);
					c=make_color_fpge(c&0xff,(c>>8)&0xff,(c>>16)&0xff);
					//if (x+y*dx==146) printf("%d %d %06x\n",j,i,c);
					if (c!=pink)
						putpixel(stack_bmp[x+y*dx],j,i,c);
					else
						putpixel(stack_bmp[x+y*dx],j,i,fpge_mask_color);
				}
		}

	total_sicons=dx*dy;

	//kill last empty tiles
		found=0;
		for(x=dx*dy-1;x>=0;x--){
			for(i=0;i<TILE_HEIGHT;i++)
				for(j=0;j<TILE_FULL_WIDTH;j++){
					c=getpixel(stack_bmp[x],j,i);
					if (c != fpge_mask_color) { found=1; break; }
				}
			if (!found)  { destroy_bitmap(stack_bmp[x]); total_sicons--; }
			else break; //break i
	    }
	destroy_bitmap(stack_bitmap);
	return 0;
}
*/
/* function to pass to hash_free_table */
//void strfree( void *d )
//{
      /* any additional processing goes here (if you use structures as data) */
      /* free the datapointer */
//      free(d);
//}
/*
int parse_pgcam( int show_info,  int probe_file_only){

	  FILE *inf, *outf;
	  char path[MAX_PATH], brfnametmp[256], color_code[256], br_color_code[256], scenario_node[256];
	  char line[1024],tokens[20][256];
	  int j,i,block=0,last_line_length=-1,cursor=0,token=0, sub_graph_counter=0;
	  int lines=0;

	  hash_table table;

	  snprintf(path,MAX_PATH,pgf_pg_pgcam);
	  canonicalize_filename(path,path,MAX_PATH);
	  if (show_info && probe_file_only==LOAD_FILE)
		  printf("Opening file %s\n",path);
	  inf=fopen(path,"rb");
	  if (!inf)
	  {
	    //printf("Couldn't open scenario file\n");
	    return ERROR_PGCAM_FILE_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	  }
	  if (probe_file_only==PROBE_FILE){
		  fclose(inf);
		  return ERROR_PGCAM_FILE_BASE+ERROR_FPGE_FILE_FOUND;
	  }

	  hash_construct_table(&table,211);

	  snprintf(path,MAX_PATH,fpge_pgcam_gv);
	  canonicalize_filename(path,path,MAX_PATH);
	  outf=fopen(path,"wb");
	  if (!outf)
	  {
	    //printf("Couldn't open scenario file\n");
		  fclose(inf);
		  return ERROR_PGCAM_FILE_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	  }

	  fprintf(outf,"digraph G { \n");

	  while (read_utf16_line_convert_to_utf8(inf,line)>=0){
		  //count lines so error can be displayed with line number
		  lines++;

			  //strip comments
			  for(i=0;i<strlen(line);i++)
				  if (line[i]==0x23) { line[i]=0; break; }
			  if (strlen(line)>0 && last_line_length==0){
				  block++;
			  }
			  last_line_length=strlen(line);
			  token=0;
			  cursor=0;
			  for(i=0;i<strlen(line);i++)
				  if (line[i]==0x09) {tokens[token][cursor]=0;token++;cursor=0;}
				  else {tokens[token][cursor]=line[i]; cursor++;}
			  tokens[token][cursor]=0;
			  token++;

			  //entry points
			  if (block == 2 && token > 1){

				  fprintf(outf,"\"%s\" [shape=diamond, fillcolor=crimson, style=filled, color=black ]\n",tokens[0]);
				  fprintf(outf,"\"%s\" -> \"%s\" %s\n",tokens[0],tokens[1],"[color=crimson]");
			  }
			  //Block#5  path
			  if (block == 5 && token > 1) {
				  //header
				  strncpy(scenario_node,tokens[0],256);
				  //check brefing
				  if (strlen(tokens[1])>0){
					  strncat(scenario_node,"+",256);

					  fprintf(outf,"subgraph cluster_%d { color=white \n",sub_graph_counter);

					  //cheat
					  fprintf(outf,"\"%s\" [shape=ellipse, style=filled, label=\"%s\", color=darkgoldenrod1]\n",tokens[0],tokens[1]); //darkgoldenrod1
					  //link
					  fprintf(outf,"\"%s\" -> \"%s\" [color=darkgoldenrod1]\n",tokens[0],scenario_node);
					  //scenario node
					  fprintf(outf,"\"%s\" [shape=box, fillcolor=darkgoldenrod1, style=filled, color=black, label=\"%s\"]\n",scenario_node,tokens[0]); //+

					  fprintf(outf,"}\n");
					  sub_graph_counter++;
				  }else{
					  fprintf(outf,"\"%s\" [shape=box, fillcolor=darkgoldenrod1, style=filled, color=black, label=\"%s\"]\n",scenario_node,tokens[0]); //chartreuse2
				  }

				  for (j = 0; j < 3; j++) {
				switch (j) {
				case 0:
					strncpy(color_code, "[color=blue]", 256);
					strncpy(br_color_code, "[color=blue2]", 256);
					break;
				case 1:
					strncpy(color_code, "[color=green]", 256);
					strncpy(br_color_code, "[color=green2]", 256);
					break;
				case 2:
					strncpy(color_code, "[color=red]", 256);
					strncpy(br_color_code, "[color=red2]", 256);
					break;
				}
				  if (strlen(tokens[3*(j+1)+2])>0){
					  strncpy(brfnametmp,tokens[3*(j+1)+2],256);
					  if (hash_lookup(brfnametmp,&table)){
						  //printf("%s juz jest\n",brfnametmp);
						  while(hash_lookup(brfnametmp,&table)){
							  strncat(brfnametmp,"+",256);
						  }
						  hash_insert( brfnametmp, strdup(brfnametmp), &table );
						  //printf("%s nie ma\n",brfnametmp);
					  }else{
						  hash_insert( brfnametmp, strdup(brfnametmp), &table );
					  }

					  fprintf(outf,"\"%s\" [shape=ellipse, style=filled, label=\"%s\"] %s\n",brfnametmp,tokens[3*(j+1)+2],br_color_code); //darkgoldenrod1
					  parse_pgbrf(outf,tokens[3*(j+1)+2],brfnametmp,color_code);
					  fprintf(outf,"\"%s\" -> \"%s\" %s\n",scenario_node,brfnametmp,color_code);
					  if (strlen(tokens[3*(j+1)])>0)
						  fprintf(outf,"\"%s\" -> \"%s\" %s\n",brfnametmp,tokens[3*(j+1)],color_code);
				  }else
					  fprintf(outf,"\"%s\" -> \"%s\" %s\n",scenario_node,tokens[3*(j+1)],color_code);
				  }
			}
	  }
	  //end node
	  fprintf(outf,"\"END\" [shape=diamond, fillcolor=crimson, style=filled, color=black ]\n");

	  fprintf(outf,"}\n");
	  fclose(inf);
	  fclose(outf);

      hash_free_table( &table, strfree );

	  if (show_info) {
		  printf("PGF campaign file loaded.\n");
		  printf("'%s' file written. To make pgcam.png use : dot %s -Tpng > pgcam.png\n",fpge_pgcam_gv,fpge_pgcam_gv);
	  }

	  return 0;
}

int parse_pgbrf(FILE *outf, char *path, char *node_name, char *color){

	  FILE *inf;
	  char str[1024];
	  int str_idx=0;
	  int state=0;
	  int buf;

	  inf=fopen(path,"rb");
	  if (!inf)
	  {
	    //printf("Couldn't open scenario file\n");
	    return ERROR_PGBRF_FILE_BASE+ERROR_FPGE_FILE_NOT_FOUND;
	  }
	    do {
	    	buf = fgetc (inf);
	    	if (state==0 && buf=='<')  {state=1; continue;}
	    	if (state==1 && buf=='a')  state=2;
	    	if (state==1 && buf!='a')  state=0;
	    	if (state==2 && buf=='>')  state=0;
	    	if (state==2){
	    		if (buf=='h') { state=3;continue;}
	    	}
	    	if (state==3){
	    		if (buf=='r') { state=4;continue;}
	    		state=2;
	    	}
	    	if (state==4){
	    		if (buf=='e') { state=5;continue;}
	    		state=2;
	    	}
	    	if (state==5){
	    		if (buf=='f') { state=6;continue;}
	    		state=2;
	    	}
	    	if (state==6){
	    		if (buf=='\"') { state=7;continue;}
	    	}
	    	if (state==7){
	    		if (buf=='\"') {
	    			str[str_idx]=0;
	    			fprintf(outf,"\"%s\" -> \"%s\" %s\n",node_name,str,color);
	    			//printf("\n");
	    			state=0;
	    			str_idx=0;
	    			continue;
	    		}
	    	}
	    	if (state==7 && buf != EOF){
	    		str[str_idx]=(char)buf;
	    		str_idx++;
	    		//printf("%c",buf);
	    	}
	    		//printf("%c%d",buf,state);
	    } while (buf != EOF);
	  fclose(inf);
	  return 0;
}*/

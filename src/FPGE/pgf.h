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

#define BRF_FILE_SIZE_WARN 10000

#include <stdio.h>

#define MAX_TURNS  256 //max number of turns in scenario
#define MAX_SUPPLY 400
#define MAX_CLASSES 32
#define MAX_VICTORY_CON 10
#define PRESTIGE_ALLOTMENTS_NUMBER 38

extern unsigned char block1_Name[256];
extern unsigned char block1_Description[1024];
extern unsigned char block1_Max_Unit_Strength[];
extern unsigned char block1_Max_Unit_Experience[];

extern unsigned short axis_experience;
extern unsigned short allied_experience;

int read_utf16_line_convert_to_utf8(FILE *inf, char *line);
int load_pgf_pgscn(char *fname, char *fullName, int scenNumber);
char *load_pgf_pgscn_info( const char *fname, char *path, int name_only );
//int load_pgf_equipment(char *fname);
//int save_pgf_pgscn(int scn_number, int show_info, int save_type, int hide_names, int,int );
//void fake_UTF_write_string_with_eol(FILE *f, char *write_me);
/*int load_bmp_tacticons();
int load_bmp_tacmap();
int load_bmp_flags();
int load_bmp_strength();
int load_bmp_stackicn();*/
/*void draw_units_bmp(BITMAP *, int, int, int, int, int);
void draw_units_per_country_bmp(int country, int month, int year, int flipIcons);
void draw_one_unit_bmp(BITMAP *, int, int, int, int,int);
int save_pgf_units_menu_bmp();
int save_pgf_units_bmp(int);
int save_pgf_dry_tiles_bmp();
int save_pgf_stack_bmp();
int save_pgf_flags_bmp();
int save_pgf_strength_bmp();
int save_pgf_equipment();

int create_small_tiles_out_of_big();
int is_html_quotable(unsigned char c);
int check_pgbrf(int show_info, char *fname);
int check_pgcam(int show_info, int probe_file_only, int fix_brefings);*/
int parse_pgbrf(FILE *, char *, char *, char *);
int parse_pgcam( int show_info, int probe_file_only);
/*int convert_brf_to_pgbrf();
int quote(char *des, char *src, int);*/

/*extern unsigned short UCS2_header;

extern unsigned char block4[MAX_TURNS][MAX_LINE_SIZE]; //block 4 storage
extern int block4_lines;
extern unsigned char block5[MAX_SUPPLY][MAX_LINE_SIZE]; //block 5 storage
extern int block5_lines;
extern unsigned char block1_SET_file[256];

extern unsigned char block7[MAX_VICTORY_CON][MAX_LINE_SIZE]; //block 7 storage
extern int block7_lines;

extern unsigned char block9[MAX_CLASSES][MAX_LINE_SIZE]; //block 9 storage
extern int block9_lines;
*/
#endif /* PGF_H_ */

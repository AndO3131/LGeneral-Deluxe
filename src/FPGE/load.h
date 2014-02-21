/*
 * load.h
 *
 *  Created on: 2010-03-09
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

#ifndef LOAD_H_
#define LOAD_H_

#include <stdio.h>


#define ERROR_FPGE_FILE_NOT_FOUND 0
#define ERROR_FPGE_MAXIMUM_REACHED 1
#define ERROR_FPGE_CANNOT_ALLOCATE_MEM 2
#define ERROR_FPGE_FILE_CANNOT_BE_WRITTEN 3
#define ERROR_FPGE_POSITION_OUT_OF_MAP 4
#define ERROR_FPGE_FILE_FOUND 5
#define ERROR_FPGE_MAP_NUMBER_FOUND 6
#define ERROR_FPGE_MAP_NUMBER_NOT_FOUND 7
#define ERROR_FPGE_AG_FILE_FOUND 8
#define ERROR_FPGE_AG_SCANED 9
#define ERROR_FPGE_NO_EQUIPMENT_FILE 10
#define ERROR_FPGE_UNKNOWN_EXE 11
#define ERROR_FPGE_WRONG_NULP 12
#define ERROR_FPGE_WRONG_BMP_SIZE_X 13
#define ERROR_FPGE_WRONG_BMP_SIZE_Y 14
#define ERROR_FPGE_DRY_X_DIFF_MUDDY_X 15
#define ERROR_FPGE_DRY_X_DIFF_FROZEN_X 16
#define ERROR_FPGE_DRY_Y_DIFF_MUDDY_Y 17
#define ERROR_FPGE_DRY_Y_DIFF_FROZEN_Y 18
#define ERROR_FPGE_WRONG_AG_LONG_DESC 19
#define ERROR_FPGE_BRF_LONG 20
#define ERROR_FPGE_WRONG_BRF_CHAR 21
#define ERROR_FPGE_PG2_FILE_TOO_SMALL 22
#define ERROR_FPGE_FILE_CANNOT_BE_READ 23
#define ERROR_FPGE_PG2_FILE_TOO_BIG 24
#define ERROR_FPGE_WRONG_WEATHER_BINTAB 25
#define ERROR_FPGE_WRONG_MOVE_BINTAB 26

#define ERROR_TILES_BASE 1000
#define ERROR_FLAGS_BASE 1100
#define ERROR_STRENGTH_BASE 1200
#define ERROR_EQUIP_BASE 1300
#define ERROR_SCN_BASE 1400
#define ERROR_UNIT_ICONS_BASE 1500
#define ERROR_STACK_UNIT_ICONS_BASE 1600
#define ERROR_NAMES_BASE 1700
#define ERROR_SET_BASE 1800
#define ERROR_STM_BASE 1900
#define ERROR_DESCRIPTION_BASE 2000
#define ERROR_AG_SCN_BASE 2100

#define ERROR_PGF_SCN_BASE 2200
#define ERROR_PGF_EQUIP_BASE 2300

#define ERROR_TILES_DESC_BASE 2400
#define ERROR_MINITILES_BASE 2500
#define ERROR_MINIUNIT_BASE 2600
#define ERROR_MINIVICTORY_BASE 2700

#define ERROR_PANZER_EXE_BASE 2800
#define ERROR_TEXT_MAP_FRAGMENT_BASE 2900

#define ERROR_PGF_BMP_TACICONS_BASE 3000
#define ERROR_PGF_BMP_TACMAP_DRY_BASE 3100
#define ERROR_PGF_BMP_TACMAP_MUDDY_BASE 3200
#define ERROR_PGF_BMP_TACMAP_FROZEN_BASE 3300
#define ERROR_PGF_BMP_FLAGS_BASE 3400
#define ERROR_PGF_BMP_STRENGTH_BASE 3500
#define ERROR_PGF_BMP_STACKICN_BASE 3600
#define ERROR_AG_TILEART_BASE 3700

#define ERROR_AG_PG_EXE_BASE 3800
#define ERROR_AG_SAVE_SCENARIO_BASE 3900
#define ERROR_P_SLOTS_TXT_BASE 4000
#define ERROR_UNIT_TXT_BASE 4100

#define ERROR_MAP_FRAGMENTS_BASE 4200
#define ERROR_COUNTRY_DESC_BASE 4300
#define ERROR_UNITS_DESC_BASE 4400

#define ERROR_PGCAM_FILE_BASE 4500
#define ERROR_PGBRF_FILE_BASE 4600

#define ERROR_TT2TILES_FILE_BASE 4700
#define ERROR_BMP2CNTY_FILE_BASE 4800

#define ERROR_PZC_EQUIP_BASE 4900
#define ERROR_PGUNIT_TO_PZCUNIT_BASE 5000
#define ERROR_PGCOUNTRY_TO_PZCCOUNTRY_BASE 5100
#define ERROR_PGTT_TO_PZCTT_BASE 5200

#define ERROR_PG2_MAP_LOAD_BASE 5300
#define ERROR_PG2_SCENARIO_LOAD_BASE 5400
#define ERROR_PG2_NAMES_LOAD_BASE 5500
#define ERROR_PG2_EQUIP_BASE 5600
#define ERROR_PG2_EQUIP_NAMES_LOAD_BASE 5700
#define ERROR_PG2UNIT_TO_PGUNIT_BASE 5800

#define ERROR_PG2TT_TO_PGTR_BASE 5900
#define ERROR_PG2TT_TO_PGTT_BASE 6000
#define ERROR_PG2COUNTRY_TO_PGCOUNTRY_BASE 6100
#define ERROR_PG2CLASS_TO_PGCLASS_BASE 6200

#define ERROR_PG2CAM_FILE_BASE 6300
#define ERROR_PG2BRIEF_FILE_BASE 6400
#define ERROR_PZCBRIEF_FILE_BASE 6500

#define ERROR_PZCCAM_FILE_BASE 6600
#define ERROR_PACGEN_CL2PG_CL_FILE_BASE 6700
#define ERROR_PACGEN_MT2PG_MT_FILE_BASE 6800

#define ERROR_PACGEN_SCENARIO_NAMES_LOAD_BASE 6700
#define ERROR_0STR_BASE 6800
#define ERROR_PGFOREVER_EXE_BASE 6900
#define ERROR_PACGEN_EXE_BASE 7000


#define PG_NUPL_SIZE 620
#define AG_NUPL_SIZE 647
#define NUPL_SIZE 650*4

#define NUPL_COUNTRY_IDX 24
#define NUPL_CLASS_IDX 12

#define SHP_SWAP_COLOR 1
#define SHP_NO_SWAP_COLOR 0

/*extern int nupl_cc[NUPL_COUNTRY_IDX][NUPL_CLASS_IDX];
extern WORD nupl[NUPL_SIZE];

extern char load_error_str[];
extern char nupl_countries[];
extern char nupl_class[];

void read_header(FILE *shp,long addr);
void read_shp(BITMAP *bmp, FILE *inf,long addr);
void read_shp_ex(BITMAP *bmp, FILE *inf,long addr, int swap);
int load_tiles();
int load_flags();
int load_strength();
int convert_from_cp1250_to_utf8(unsigned char *save_to,unsigned char *read_from, int limit);

int load_uicons();
int load_sicons();
int load_description(int show_info);

int load_tiles_description();
int parse_nupl();
int load_nupl();
int load_p_slots_txt();
int load_nulp_txt();
int load_unit_txt();
int load_map_fragments();
int load_countries_description();
int load_icons_description();

int load_bmp2country_description();
int load_0str_bmp();
*/
#endif /* LOAd_H_ */

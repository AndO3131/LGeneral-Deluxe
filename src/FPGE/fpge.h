/* This is the main header file for FPGE */
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

#ifndef _FPGE_H_
#define _FPGE_H_

#define MAX_PATH 512
#define MAX_EXT 50

//my helper function
#define Max(a,b) (a>b)?(a):(b)
#define Min(a,b) (a<b)?(a):(b)

#undef min
#undef max

#define print_hex(var)  printf("%s=%04x\n",#var,var)
#define print_dec(var)  printf("%s=%d\n",#var,var)
#define print_ldec(var)  printf("%s=%ld\n",#var,var)
#define print_str(var)  printf("%s=%s\n",#var,var)
#define print_ptr(var)  printf("%s=%p\n",#var,var)

#define BYTE unsigned char
#define WORD short
#define UWORD unsigned short
#define LONG long
#define ULONG unsigned long

#define MAX_TILES_IN_PG 237  //maximum number of map tile icons in PG. Used in various automatic actions
#define STD_MATRIX_MAX_X 20
//#define STD_MATRIX_MAX_Y 12

//#define MAX_TILES 256  //maximum number of map tile icons
#define MAX_TILES 8192  //maximum number of map tile icons

#define MAX_UICONS 6000 //maximum number of unit icons
#define MAX_MUICONS 2  //allways 2 for strategic map
#define MAX_UNITS 5000  //maximum number of units in equipment file
#define MAX_FLAGS 512   //actually PG uses 48, we need 512 to handle 256 flags due to graphics loading code
#define MAX_MINI_VIC_HEX 2   //allways 2 for strategic map
#define MAX_STRENGTH_IN_ROW 21   // from 0 to 20 to suite PGF
#define MAX_STRENGTH MAX_STRENGTH_IN_ROW*4   //actually 63
#define MAX_ICONS MAX_UICONS  //most icons that can be loaded into address 
#define MAX_SICONS 100 // stack icons

#define MAX_VICTORY_HEXES 20
#define MAX_DEPLOY_HEXES  255
#define MAX_AXIS_UNITS    80*2
#define MAX_ALLIED_UNITS 200*2
#define MAX_AXIS_AUX_UNITS    120*2
#define MAX_NAMES        20000

#define MAX_NAME_SIZE 20
#define MAX_PG2_NAME_SIZE 30
#define MAX_NAME_UTF_SIZE 40

#define STD_NAMES_PG 16

//#define MAX_MAP_X 71
//#define MAX_MAP_Y 54
#define MAX_MAP_X 1024
#define MAX_MAP_Y 1024
//#define MAX_DISPLAY_TILES 237  //maximum number of used tiles
#define TILES_HOT_KEY_NUMBER 9

#define CLASS_NUMBER 18
#define CLASS_NAME_LENGTH 16

#define BK_COLOR 0     //transparent shp color
#define HEADER_SIZE 24 //size of shp header
#define TILE_HEIGHT 50
#define TILE_WIDTH 45
#define TILE_FULL_WIDTH 60
#define TILE_HEIGHT_SHIFT 25
#define TILE_WIDTH_SHIFT 16

#define MAP_X_ADDR 101
#define MAP_Y_ADDR 103
#define MAP_STM   0x30
#define MAP_SET   0x51
#define MAP_LAYERS_START   123

#define MAX_PATTERN_SIZE  128

#define NO_FILTER_INDEX -1
#define COAST_FILTER_INDEX 0
#define CITY_FILTER_INDEX 1
#define ROAD_FILTER_INDEX 5
#define RIVER_FILTER_INDEX 6
#define FOREST_FILTER_INDEX 8

#define UNKNOWN_UTR_NAME 39

#define SHOW_LOGS 1
#define DO_NOT_SHOW_LOGS 0

#define LOAD_FILE 0
#define PROBE_FILE 1
#define SCAN_FOR_MAP_NUMBER 2
#define LOAD_CONVERSION_TABLE_ONLY 3
#define NORMAL_SAVE 4
#define SAVE_WITH_UNIT_IDS_CONVERSION 5
#define SAVE_WITH_UNIT_IDS_HARD_CONVERSION 6

#define MAX_COUNTRY 255 //was 25 as standard PG countries number
#define MAX_COUNTRY_NAME_SIZE 21
#define MAX_COUNTRY_SHORT_NAME_SIZE 5
#define MAX_TERRAIN_TYPE 40
#define TERRAIN_TYPE_SIZE 25

#define MAX_MOV_TYPE 25
#define RADIO_PG_MOV_TYPES 8
#define RADIO_PGF_MOV_TYPES 11
#define MAX_TERRAIN_MOV_TYPES 50
#define ROW_PG_TERRAIN_MOV_TYPES 12
#define ROW_PACGEN_TERRAIN_MOV_TYPES 37
#define MOVEMENT_TYPES_NO_COL 3
#define MAX_COLUMNS_DYNDLG 10

#define PGF_MOV_TYPES 11

#define MAX_UNIT_AUX_NAME_SIZE 60

/*  -------------------- Here is the Main Dialog ----------------------- */
/* ------ this section has defines for the main dialog ------------------- */
//#define SCREEN_X 640
//#define SCREEN_Y 480

//#define SCREEN_X 800
//#define SCREEN_Y 600

#define SCREEN_X 1024
#define SCREEN_Y 768

//#define SCREEN_X 1280
//#define SCREEN_Y 1024

#define TOP_LINE_Y 2
#define LINE_1_Y   28
#define LINE_2_Y   54

#define VSLIDE_W 23
#define VSLIDE_H SCREEN_Y-280

#define VSLIDE_X SCREEN_X-80
#define VSLIDE_Y 200

#define HSLIDE_W SCREEN_X - 440
#define HSLIDE_H 23
#define HSLIDE_X 210
#define HSLIDE_Y LINE_1_Y

#define TILES_WIDE (SCREEN_X-80)/TILE_WIDTH
#define TILES_HIGH (SCREEN_Y-TILE_HEIGHT/2-54)/TILE_HEIGHT

#define MAP_H (TILES_HIGH*2+1)*TILE_HEIGHT/2+1
#define MAP_W (TILES_WIDE*3/2+1)*30-15
#define MAIN_BUTTON_X SCREEN_X-50
#define EXIT_Y SCREEN_Y-10

#define BUTTON_SIZE 20
#define BUTTON_H 15
#define BUTTON_W 45

#define COLOR_WHITE 15
#define COLOR_BLACK 255
#define COLOR_BLUE 246
#define COLOR_GREEN 10
#define COLOR_RED 12
#define COLOR_PURPLE 5
#define COLOR_YELLOW 27
#define COLOR_DARK_GREEN 120
#define COLOR_DARK_DARK_GREEN 232
#define COLOR_DARK_GREEN_GRAY 38
#define COLOR_DARK_BLUE_GRAY 162
#define COLOR_LIGHT_BLUE (255-16-2)

#define HEX_COLOR 241
#define LINE_COLOR 22

#define GUI_FG_COLOR 256
#define GUI_BG_COLOR 256+1
#define GUI_EDIT_COLOR 256+2

#define FPGE_FG_COLOR 41
#define FPGE_BG_COLOR 54
#define FPGE_SCREEN_COLOR 8

//main gui colors only
#define FG_COLOR GUI_FG_COLOR
#define BG_COLOR GUI_BG_COLOR
#define SCREEN_COLOR GUI_BG_COLOR

#define MAP_COLOR 149
#define SLIDE_BG_COLOR 38
#define SLIDE_FG_COLOR 33
#define VIC_HEX_COLOR 4

#define NEUTRAL_HEX_COLOR 256-16+3
#define DEPLOY_HEX_COLOR 255
#define BLACK_HEX_COLOR 8
#define PROBLEM_HEX_COLOR 122
#define MAGIC_ROAD_COLOR 127
#define MAGIC_RIVER_COLOR 126

#define BLACK_TILE (MAX_TILES+1)
#define NOT_USED_TILE (MAX_TILES+2)
#define MAGIC_ROAD (MAX_TILES+3)
#define MAGIC_RIVER (MAX_TILES+4)
#define MAGIC_ROAD_AND_RIVER (MAX_TILES+5)

#define BYTE_MAX (256-1)
#define WORD_MAX (256*256-1)

#define PG_MODE 0
#define PGF_MODE 1
#define AG_MODE 2
#define PACGEN_MODE 3

//we won't treat equip as a structure but will use offsets into the records
#define EQUIP_REC_SIZE 50

#define NAME   0
#define CLASS 20 //class
#define SA    21 //soft attack
#define HA    22 //hard attack
#define AA    23 //air attack
#define NA    24 //naval attack
#define GD    25 //ground defence
#define AD    26 //air defence
#define CD    27 //close defence
#define TARGET_TYPE    28 //target type 0-soft,1-hard,2-air,3-naval,[4-submarine-PacGen]
#define AAF   29   //1 for tac bombers who can attack
#define INITIATIVE   31 //initiative
#define RANGE   32 //range
#define SPOTTING   33 //spotting
#define GAF   34    //the ground=0 air=1 flag
#define MOV_TYPE 35 //movement type 0-tracked,1-halftracked,2-wheeled,3-leg,4-towed,5-air,6-naval,7-all terrain
                    //PGF: amphibious tracked (8), amphibious all terrain (9) and mountain (10)
#define MOV   36  //movement
#define FUEL  37  //fuels
#define AMMO  38 //amo
#define COST  41 //cost
#define BMP   42 //icon number - 2 bytes
#define ANI   44 //animation number - 2 bytes (?)
#define MON   46
#define YR    47
#define LAST_YEAR 48
#define ALLOWED_TRANSPORT 49 //1-can have organic, 2-can have sea/air ,3-can paradrop

#define COST_DIVISOR 12

//similarly we will use an array to store the scenario
#define MAX_SCN_SIZE 5000
// These are offsets or starting addresses into the scenario file
#define ORIENTATION 12 //2bytes
#define AXIS_STANCE 14
#define ALLIED_STANCE 15
#define STORM_FRONT 16
#define SCEN_LOCALE 17

#define CORE_UNITS_LIMIT 18
#define AUX_UNITS_LIMIT 19
#define ALLIED_UNITS_LIMIT  20
#define ALLIED_AUX_UNITS_LIMIT  (MAX_SCN_SIZE-1)

#define TURNS 21
#define DAY   22
#define MONTH 23
#define YEAR  24
#define TURNS_PER_DAY 25
#define DAYS_PER_TURN 26
#define AI_AXIS_BUCKET 27 //2 bytes
#define AI_ALLIED_BUCKET 29 //2 bytes
#define AI_AXIS_INTERVAL 31
#define AI_ALLIED_INTERVAL 32
#define CORE_UNITS    33
#define ALLIED_UNITS  34
#define AUX_UNITS     35
#define AUX_ALLIED_UNITS  36
#define VIC_HEXES     37    //up to 20 terminated with -1
#define DEPLOY_TABLE  117
#define DEPLOY_TO_UNITS 0x87

//----
#define SHADE_MASK          0x01
#define PROBLEM_MASK        0x02
#define SELECTED_FRG_MASK   0x04
#define PLACED_FRG_MASK     0x08

#define EQUIPMENT_HARDCODED                  0x0001
#define EQUIPMENT_BOMBER_SPECIAL             0x0002
#define EQUIPMENT_IGNORES_ENTRENCHMENT       0x0004
#define EQUIPMENT_CAN_HAVE_ORGANIC_TRANSPORT 0x0008
#define EQUIPMENT_CAN_HAVE_SEA_TRANSPORT     0x0010
#define EQUIPMENT_CAN_HAVE_AIR_TRANSPORT     0x0020
#define EQUIPMENT_CAN_PARADROP               0x0040
#define EQUIPMENT_CAN_BRIDGE_RIVERS          0x0080
#define EQUIPMENT_JET                        0x0100
#define EQUIPMENT_CANNOT_BE_PURCHASED        0x0200
#define EQUIPMENT_RECON_MOVEMENT             0x0400

#define MAX_LINE_SIZE 256
#define MAX_MAP_FRG 500
#define FRG_X_SIZE 9
#define FRG_Y_SIZE 9

// offsets in s4_buffer
#define AXIS_PRESTIGE   0
#define ALLIED_PRESTIGE 2
#define AXIS_AIR_NUMBER 4
#define ALLIED_AIR_NUMBER 5
#define AXIS_SEA_NUMBER 6
#define ALLIED_SEA_NUMBER 7
#define AXIS_AIR_TYPE 8
#define ALLIED_AIR_TYPE 10
#define AXIS_SEA_TYPE 12
#define ALLIED_SEA_TYPE 14

#define DEFAULT_AXIS_AIR_TYPE 29
#define DEFAULT_AXIS_SEA_TYPE 299
#define DEFAULT_ALLIED_AIR_TYPE 354
#define DEFAULT_ALLIED_SEA_TYPE 291

#define MAX_SCENARIOS 40
#define SHORT_SCN_SIZE 14
#define LONG_SCN_SIZE 160

#endif

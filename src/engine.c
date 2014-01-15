/***************************************************************************
                          engine.c  -  description
                             -------------------
    begin                : Sat Feb 3 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/
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

#ifdef USE_DL
  #include <dlfcn.h>
#endif
#include <math.h>
#include "lgeneral.h"
#include "date.h"
#include "event.h"
#include "windows.h"
#include "nation.h"
#include "unit.h"
#include "purchase_dlg.h"
#include "gui.h"
#include "map.h"
#include "scenario.h"
#include "slot.h"
#include "action.h"
#include "strat_map.h"
#include "campaign.h"
#include "ai.h"
#include "engine.h"
#include "localize.h"
#include "file.h"

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern Config config;
extern int cur_weather; /* used in unit.c to compute weather influence on units; set by set_player() */
extern Nation *nations;
extern int nation_count;
extern int nation_flag_width, nation_flag_height;
extern int hex_w, hex_h;
extern int hex_x_offset, hex_y_offset;
extern Terrain_Icons *terrain_icons;
extern int map_w, map_h;
extern Weather_Type *weather_types;
extern List *players;
extern int turn;
extern List *units;
extern List *reinf;
extern List *avail_units;
extern Scen_Info *scen_info;
extern Map_Tile **map;
extern Mask_Tile **mask;
extern GUI *gui;
extern int camp_loaded;
extern Camp_Entry *camp_cur_scen;
extern Setup setup;
extern int  term_game, sdl_quit;
extern char *camp_first;

/*
====================================================================
Engine
====================================================================
*/
int modify_fog = 0;      /* if this is False the fog initiated by
                            map_set_fog() is kept throughout the turn
                            else it's updated with movement mask etc */   
int cur_ctrl = 0;        /* current control type (equals player->ctrl if set else
                            it's PLAYER_CTRL_NOBODY) */
Player *cur_player = 0;  /* current player pointer */
typedef struct {         /* unit move backup */
    int used;
    Unit unit;          /* shallow copy of unit */
    /* used to reset map flag if unit captured one */
    int flag_saved;     /* these to values used? */
    Nation *dest_nation;
    Player *dest_player;
} Move_Backup;
Move_Backup move_backup = { 0 }; /* backup to undo last move */
int fleeing_unit = 0;    /* if this is true the unit's move is not backuped */
int air_mode = 0;        /* air units are primary */
int end_scen = 0;        /* True if scenario is finished or aborted */
List *left_deploy_units = 0; /* list with unit pointers to avail_units of all
                                units that arent placed yet */
Unit *deploy_unit = 0;   /* current unit selected in deploy list */
Unit *surrender_unit = 0;/* unit that will surrender */
Unit *move_unit = 0;     /* currently moving unit */
Unit *surp_unit = 0;     /* if set cur_unit has surprise_contact with this unit if moving */
Unit *cur_unit = 0;      /* currently selected unit (by human) */
Unit *cur_target = 0;    /* target of cur_unit */
Unit *cur_atk = 0;       /* current attacker - if not defensive fire it's
                            identical with cur_unit */
Unit *cur_def = 0;       /* is the current defender - identical with cur_target
                            if not defensive fire (then it's cur_unit) */
List *df_units = 0;      /* this is a list of defensive fire units giving support to 
                            cur_target. as long as this list isn't empty cur_unit
                            becomes the cur_def and cur_atk is the current defensive
                            unit. if this list is empty in the last step
                            cur_unit and cur_target actually do their fight
                            if attack wasn't broken up */
int  defFire = 0;        /* combat is supportive so switch casualties */
int merge_unit_count = 0;
Unit *merge_units[MAP_MERGE_UNIT_LIMIT];  /* list of merge partners for cur_unit */
int split_unit_count = 0;
Unit *split_units[MAP_SPLIT_UNIT_LIMIT]; /* list of split partners */
int cur_split_str = 0;
/* DISPLAY */
enum { 
    SC_NONE = 0,
    SC_VERT,
    SC_HORI
};    
int sc_type = 0, sc_diff = 0;  /* screen copy type. used to speed up complete map updates */   
SDL_Surface *sc_buffer = 0;    /* screen copy buffer */
int *hex_mask = 0;             /* used to determine hex from pointer pos */
int map_x, map_y;              /* current position in map */
int map_sw, map_sh;            /* number of tiles drawn to screen */
int map_sx, map_sy;            /* position where to draw first tile */
int draw_map = 0;              /* if this flag is true engine_update() calls engine_draw_map() */
enum {
    SCROLL_NONE = 0,
    SCROLL_LEFT,
    SCROLL_RIGHT,
    SCROLL_UP,
    SCROLL_DOWN
};
int scroll_hori = 0, scroll_vert = 0; /* scrolling directions if any */
int blind_cpu_turn = 0;        /* if this is true all movements are hidden */
Button *last_button = 0;       /* last button that was pressed. used to clear the down state when button was released */
/* MISC */
int old_mx = -1, old_my = -1;  /* last map tile the cursor was on */
int old_region = -1;           /* region in map tile */
int scroll_block_keys = 0;     /* block keys fro scrolling */
int scroll_block = 0;          /* block scrolling if set used to have a constant
                                  scrolling speed */
int scroll_time = 100;         /* one scroll every 'scroll_time' milliseconds */
Delay scroll_delay;            /* used to time out the remaining milliseconds */
char *slot_name;               /* slot name to which game is saved */
Delay blink_delay;             /* used to blink dots on strat map */
/* ACTION */
enum {
    STATUS_NONE = 0,           /* actions that are divided into different phases
                                  have this status set */
    STATUS_MOVE,               /* move unit along 'way' */
    STATUS_ATTACK,             /* unit attacks cur_target (inclusive defensive fire) */
    STATUS_MERGE,              /* human may merge with partners */
    STATUS_SPLIT,              /* human wants to split up a unit */
    STATUS_DEPLOY,             /* human may deploy units */
    STATUS_DROP,               /* select drop zone for parachutists */
    STATUS_INFO,               /* show full unit infos */
    STATUS_SCEN_INFO,          /* show scenario info */
    STATUS_CONF,               /* run confirm window */
    STATUS_UNIT_MENU,          /* running the unit buttons */
    STATUS_GAME_MENU,          /* game menu */
    STATUS_DEPLOY_INFO,        /* full unit info while deploying */
    STATUS_STRAT_MAP,          /* showing the strategic map */
    STATUS_RENAME,             /* rename unit */
    STATUS_SAVE_EDIT,          /* running the save edit */
    STATUS_SAVE_MENU,          /* running the save menu */
    STATUS_LOAD_MENU,          /* running the load menu */
    STATUS_MOD_SELECT,         /* running mod select menu */
    STATUS_NEW_FOLDER,         /* creating new folder name */
    STATUS_TITLE,              /* show the background */
    STATUS_TITLE_MENU,         /* run title menu */
    STATUS_RUN_SCEN_DLG,       /* run scenario dialogue */
    STATUS_RUN_CAMP_DLG,       /* run campaign dialogue */
    STATUS_RUN_SCEN_SETUP,     /* run setup of scenario */
    STATUS_RUN_CAMP_SETUP,     /* run setup of campaign */
    STATUS_RUN_MODULE_DLG,     /* select ai module */
    STATUS_CAMP_BRIEFING,      /* run campaign briefing dialogue */
    STATUS_PURCHASE,           /* run unit purchase dialogue */
    STATUS_VMODE_DLG           /* run video mode selection */
};
int status;                    /* statuses defined in engine_tools.h */
enum {
    PHASE_NONE = 0,
    /* COMBAT */
    PHASE_INIT_ATK,             /* initiate attack cross */
    PHASE_SHOW_ATK_CROSS,       /* attacker cross */
    PHASE_SHOW_DEF_CROSS,       /* defender cross */
    PHASE_COMBAT,               /* compute and take damage */
    PHASE_EVASION,              /* sub evades */
    PHASE_RUGGED_DEF,           /* stop the engine for some time and display the rugged defense message */
    PHASE_PREP_EXPLOSIONS,      /* setup the explosions */
    PHASE_SHOW_EXPLOSIONS,      /* animate both explosions */
    PHASE_FIGHT_MSG,		/* show fight status messages */
    PHASE_CHECK_RESULT,         /* clean up this fight and initiate next if any */
    PHASE_BROKEN_UP_MSG,        /* display broken up message if needed */
    PHASE_SURRENDER_MSG,        /* display surrender message */
    PHASE_END_COMBAT,           /* clear status and redraw */
    /* MOVEMENT */
    PHASE_INIT_MOVE,            /* initiate movement */ 
    PHASE_START_SINGLE_MOVE,    /* initiate movement to next way point from current position */
    PHASE_RUN_SINGLE_MOVE,      /* run single movement and call START_SINGLE_MOVEMENT when done */
    PHASE_CHECK_LAST_MOVE,      /* check last single move for suprise contact, flag capture, scenario end */
    PHASE_END_MOVE              /* finalize movement */
};
int phase;
Way_Point *way = 0;             /* way points for movement */
int way_length = 0;
int way_pos = 0;
int dest_x, dest_y;             /* ending point of the way */
Image *move_image = 0;          /* image that contains the moving unit graphic */
float move_vel = 0.3;           /* pixels per millisecond */
Delay move_time;                /* time a single movement takes */
typedef struct {
    float x,y;
} Vector;
Vector unit_vector;             /* floating position of animation */
Vector move_vector;             /* vector the unit moves along */
int surp_contact = 0;           /* true if the current combat is the result of a surprise contact */
int atk_result = 0;             /* result of the attack */
Delay msg_delay;                /* broken up message delay */
int atk_took_damage = 0;
int def_took_damage = 0;        /* if True an explosion is displayed when attacked */
int atk_damage_delta;		/* damage delta for attacking unit */
int atk_suppr_delta;		/* supression delta for attacking unit */
int def_damage_delta;		/* damage delta for defending unit */
int def_suppr_delta;		/* supression delta for defending unit */
static int has_danger_zone;	/* whether there are any danger zones */
int deploy_turn;		/* 1 if this is the deployment-turn */
static Action *top_committed_action;/* topmost action not to be removed */
static struct MessagePane *camp_pane; /* state of campaign message pane */
static char *last_debriefing;   /* text of last debriefing */
int log_x, log_y = 2;
char log_str[MAX_NAME];

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Forwarded
====================================================================
*/          
static void engine_draw_map();
static void engine_update_info( int mx, int my, int region );
static void engine_goto_xy( int x, int y );
static void engine_set_status( int newstat );
static void engine_show_final_message( void );
static void engine_select_player( Player *player, int skip_unit_prep );
static int engine_capture_flag( Unit *unit );
static void engine_show_game_menu( int cx, int cy );
static void engine_handle_button( int id );

/*
====================================================================
End the scenario and display final message.
====================================================================
*/
static void engine_finish_scenario()
{
    /* finalize ai turn if any */
    if ( cur_player && cur_player->ctrl == PLAYER_CTRL_CPU )
        (cur_player->ai_finalize)();
    /* continue campaign if any */
    if (camp_loaded == FIRST_SCENARIO)
        camp_loaded = CONTINUE_CAMPAIGN;
    blind_cpu_turn = 0;
    engine_show_final_message();
    group_set_active( gui->base_menu, ID_MENU, 0 );
    draw_map = 1;
    image_hide( gui->cursors, 0 );
    gui_set_cursor( CURSOR_STD );
    engine_select_player( 0, 0 );
    turn = scen_info->turn_limit;
    engine_set_status( STATUS_NONE ); 
    phase = PHASE_NONE;
}

/*
====================================================================
Return the first human player.
====================================================================
*/
static Player *engine_human_player(int *human_count)
{
    Player *human = 0;
    int count = 0;
    int i;
    for ( i = 0; i < players->count; i++ ) {
        Player *player = list_get( players, i );
        if ( player->ctrl == PLAYER_CTRL_HUMAN ) {
            if ( count == 0 )
                human = player;
            count++;
        }
    }
    if (human_count) *human_count = count;
    return human;
}

/*
====================================================================
Clear danger zone.
====================================================================
*/
static void engine_clear_danger_mask()
{
    if ( has_danger_zone ) {
        map_clear_mask( F_DANGER );
        has_danger_zone = 0;
    }
}

/*
====================================================================
Set wanted status.
====================================================================
*/
static void engine_set_status( int newstat )
{
    if ( newstat == STATUS_NONE && setup.type == SETUP_RUN_TITLE )
    {
        status = STATUS_TITLE;
        /* re-show main menu */
        if (!term_game) engine_show_game_menu(10,40);
    }
    /* return from scenario to title screen */
    else if ( newstat == STATUS_TITLE )
    {
        status = STATUS_TITLE;
        if (!term_game) engine_show_game_menu(10,40);
    }
    else
        status = newstat;
}

/*
====================================================================
Draw wallpaper and background.
====================================================================
*/
static void engine_draw_bkgnd()
{
    int i, j;
    for ( j = 0; j < sdl.screen->h; j += gui->wallpaper->h )
        for ( i = 0; i < sdl.screen->w; i += gui->wallpaper->w ) {
            DEST( sdl.screen, i, j, gui->wallpaper->w, gui->wallpaper->h );
            SOURCE( gui->wallpaper, 0, 0 );
            blit_surf();
        }
    DEST( sdl.screen, 
          ( sdl.screen->w - gui->bkgnd->w ) / 2,
          ( sdl.screen->h - gui->bkgnd->h ) / 2,
          gui->bkgnd->w, gui->bkgnd->h );
    SOURCE( gui->bkgnd, 0, 0 );
    blit_surf();
    log_x = sdl.screen->w - 2;
    log_y = 2;
    gui->font_error->align = ALIGN_X_RIGHT | ALIGN_Y_TOP;
    write_line( sdl.screen, gui->font_error, log_str, log_x, &log_y );
    blit_surf();
}

/*
====================================================================
Returns true when the status screen dismission events took place.
====================================================================
*/
inline static int engine_status_screen_dismissed()
{
    int dummy;
    return event_get_buttonup( &dummy, &dummy, &dummy )
            || event_check_key(SDLK_SPACE)
            || event_check_key(SDLK_RETURN)
            || event_check_key(SDLK_ESCAPE);
}

#if 0
/*
====================================================================
Display a big message box (e.g. briefing) and wait for click.
====================================================================
*/
static void engine_show_message( char *msg )
{
    struct MessagePane *pane = gui_create_message_pane();
    gui_set_message_pane_text(pane, msg);
    engine_draw_bkgnd();
    gui_draw_message_pane(pane);
    /* wait */
    SDL_PumpEvents(); event_clear();
    while ( !engine_status_screen_dismissed() ) { SDL_PumpEvents(); SDL_Delay( 20 ); }
    event_clear();
    gui_delete_message_pane(pane);
}
#endif

/*
====================================================================
Store debriefing of last scenario or an empty string.
====================================================================
*/
static void engine_store_debriefing(const char *result)
{
    const char *str = camp_get_description(result);
    free(last_debriefing); last_debriefing = 0;
    if (str) last_debriefing = strdup(str);
}

/*
====================================================================
Prepare display of next campaign briefing.
====================================================================
*/
static void engine_prep_camp_brief()
{
    gui_delete_message_pane(camp_pane);
    camp_pane = 0;
    engine_draw_bkgnd();
    
    camp_pane = gui_create_message_pane();
    /* make up scenario text */
    {
        char *txt = alloca((camp_cur_scen->title ? strlen(camp_cur_scen->title) + 2 : 0)
                           + (last_debriefing ? strlen(last_debriefing) + 1 : 0)
                           + (camp_cur_scen->brief ? strlen(camp_cur_scen->brief) : 0) + 2 + 1);
        strcpy(txt, "");
        if (camp_cur_scen->title) {
            strcat(txt, camp_cur_scen->title);
            strcat(txt, "##");
        }
        if (last_debriefing) {
            strcat(txt, last_debriefing);
            strcat(txt, " ");
        }
        if (camp_cur_scen->brief) {
            strcat(txt, camp_cur_scen->brief);
            strcat(txt, "##");
        }
        gui_set_message_pane_text(camp_pane, txt);
    }
    
    /* provide options or default id */
    if (camp_cur_scen->scen) {
      gui_set_message_pane_default(camp_pane, "nextscen");
    } else {
      List *ids = camp_get_result_list();
      List *vals = list_create(LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK);
      char *result;
      
      list_reset(ids);
      while ((result = list_next(ids))) {
          const char *desc = camp_get_description(result);
          list_add(vals, desc ? (void *)desc : (void *)result);
      }
      
      /* no ids means finishing state */
      if (ids->count == 0)
          gui_set_message_pane_default(camp_pane, " ");
      /* don't provide explicit selection if there is only one id */
      else if (ids->count == 1)
          gui_set_message_pane_default(camp_pane, list_first(ids));
      else
          gui_set_message_pane_options(camp_pane, ids, vals);
      
      list_delete(ids);
      list_delete(vals);
    }
    
    gui_draw_message_pane(camp_pane);
    engine_set_status(STATUS_CAMP_BRIEFING);
}

/*
====================================================================
Check menu buttons and enable/disable according to engine status.
====================================================================
*/
static void engine_check_menu_buttons()
{
    /* airmode */
    group_set_active( gui->base_menu, ID_AIR_MODE, 1 );
    /* menu */
    if ( cur_ctrl == PLAYER_CTRL_NOBODY )
        group_set_active( gui->base_menu, ID_MENU, 0 );
    else
        group_set_active( gui->base_menu, ID_MENU, 1 );
    /* info */
    if ( status != STATUS_NONE )
        group_set_active( gui->base_menu, ID_SCEN_INFO, 0 );
    else
        group_set_active( gui->base_menu, ID_SCEN_INFO, 1 );
    /* purchase */
    if ( (config.purchase != NO_PURCHASE) && cur_ctrl != PLAYER_CTRL_NOBODY && (!deploy_turn  && status == STATUS_NONE ) )
        group_set_active( gui->base_menu, ID_PURCHASE, 1 );
    else
        group_set_active( gui->base_menu, ID_PURCHASE, 0 );
    /* deploy */
    if ( avail_units )
    {
        if ( ( avail_units->count > 0 || deploy_turn ) && status == STATUS_NONE )
            group_set_active( gui->base_menu, ID_DEPLOY, 1 );
        else
            group_set_active( gui->base_menu, ID_DEPLOY, 0 );
    }
    /* strat map */
    if ( status == STATUS_NONE )
        group_set_active( gui->base_menu, ID_STRAT_MAP, 1 );
    else
        group_set_active( gui->base_menu, ID_STRAT_MAP, 0 );
    /* end turn */
    if ( status != STATUS_NONE )
        group_set_active( gui->base_menu, ID_END_TURN, 0 );
    else
        group_set_active( gui->base_menu, ID_END_TURN, 1 );
    /* victory conditions */
    if ( status != STATUS_NONE )
        group_set_active( gui->base_menu, ID_CONDITIONS, 0 );
    else
        group_set_active( gui->base_menu, ID_CONDITIONS, 1 );
}

/*
====================================================================
Check unit buttons. (use current unit)
====================================================================
*/
static void engine_check_unit_buttons()
{
    char str[MAX_LINE];
    if ( cur_unit == 0 ) return;
    /* rename */
    group_set_active( gui->unit_buttons, ID_RENAME, 1 );
    /* supply */
    if ( unit_check_supply( cur_unit, UNIT_SUPPLY_ANYTHING, 0, 0 ) ) {
        group_set_active( gui->unit_buttons, ID_SUPPLY, 1 );
        /* show supply level */
        sprintf( str, tr("Supply Unit [s] (Rate:%3i%%)"), cur_unit->supply_level );
        strcpy( group_get_button( gui->unit_buttons, ID_SUPPLY )->tooltip_center, str );
    }
    else
        group_set_active( gui->unit_buttons, ID_SUPPLY, 0 );
    /* merge */
    if (config.merge_replacements == OPTION_MERGE)
    {
        if ( merge_unit_count > 0 )
            group_set_active( gui->unit_buttons, ID_MERGE, 1 );
        else
            group_set_active( gui->unit_buttons, ID_MERGE, 0 );
    }
    /* split */
    if (unit_get_split_strength(cur_unit)>0)
        group_set_active( gui->unit_buttons, ID_SPLIT, 1 );
    else
        group_set_active( gui->unit_buttons, ID_SPLIT, 0 );
    /* replacements */
    if (config.merge_replacements == OPTION_REPLACEMENTS)
    {
        if (unit_check_replacements(cur_unit,REPLACEMENTS) > 0)
        {
            group_set_active( gui->unit_buttons, ID_REPLACEMENTS, 1 );
            /* show replacements and supply level */
            unit_check_supply( cur_unit, UNIT_SUPPLY_ANYTHING, 0, 0 );
            sprintf( str, tr("Supply Rate:%3i%%"), cur_unit->supply_level );
            strcpy( group_get_button( gui->unit_buttons, ID_REPLACEMENTS )->tooltip_left, str );
            sprintf( str, tr("Replacements") );
            strcpy( group_get_button( gui->unit_buttons, ID_REPLACEMENTS )->tooltip_center, str );
            sprintf( str, tr("Str:%2i, Exp:%4i, Prst:%4i"),
                     cur_unit->cur_str_repl, -cur_unit->repl_exp_cost, -cur_unit->repl_prestige_cost );
            strcpy( group_get_button( gui->unit_buttons, ID_REPLACEMENTS )->tooltip_right, str );
        }
        else
            group_set_active( gui->unit_buttons, ID_REPLACEMENTS, 0 );
        /* elite replacements */
        if (unit_check_replacements(cur_unit,ELITE_REPLACEMENTS) > 0)
        {
            group_set_active( gui->unit_buttons, ID_ELITE_REPLACEMENTS, 1 );
            /* show elite replacements and supply level */
            sprintf( str, tr("Supply Rate:%3i%%"), cur_unit->supply_level );
            strcpy( group_get_button( gui->unit_buttons, ID_ELITE_REPLACEMENTS )->tooltip_left, str );
            sprintf( str, tr("Elite Replacements") );
            strcpy( group_get_button( gui->unit_buttons, ID_ELITE_REPLACEMENTS )->tooltip_center, str );
            sprintf( str, tr("Str:%2i, Prst:%4i"), cur_unit->cur_str_repl, -cur_unit->repl_prestige_cost );
            strcpy( group_get_button( gui->unit_buttons, ID_ELITE_REPLACEMENTS )->tooltip_right, str );
        }
        else
            group_set_active( gui->unit_buttons, ID_ELITE_REPLACEMENTS, 0 );
    }
    /* undo */
    if ( move_backup.used )
        group_set_active( gui->unit_buttons, ID_UNDO, 1 );
    else
        group_set_active( gui->unit_buttons, ID_UNDO, 0 );
    /* air embark */
    if ( map_check_unit_embark( cur_unit, cur_unit->x, cur_unit->y, EMBARK_AIR, 0 ) || 
         map_check_unit_debark( cur_unit, cur_unit->x, cur_unit->y, EMBARK_AIR, 0 ) )
        group_set_active( gui->unit_buttons, ID_EMBARK_AIR, 1 );
    else
        group_set_active( gui->unit_buttons, ID_EMBARK_AIR, 0 );
    /* disband */
    if (cur_unit->unused)
        group_set_active( gui->unit_buttons, ID_DISBAND, 1 );
    else
        group_set_active( gui->unit_buttons, ID_DISBAND, 0 );
}

/*
====================================================================
Show/Hide full unit info while deploying
====================================================================
*/
static void engine_show_deploy_unit_info( Unit *unit )
{
    status = STATUS_DEPLOY_INFO;
    gui_show_full_info( unit );
    group_set_active( gui->deploy_window, ID_DEPLOY_UP, 0 );
    group_set_active( gui->deploy_window, ID_DEPLOY_DOWN, 0 );
    group_set_active( gui->deploy_window, ID_APPLY_DEPLOY, 0 );
    group_set_active( gui->deploy_window, ID_CANCEL_DEPLOY, 0 );
}
static void engine_hide_deploy_unit_info()
{
    status = STATUS_DEPLOY;
    frame_hide( gui->finfo, 1 );
    old_mx = old_my = -1;
    group_set_active( gui->deploy_window, ID_DEPLOY_UP, 1 );
    group_set_active( gui->deploy_window, ID_DEPLOY_DOWN, 1 );
    group_set_active( gui->deploy_window, ID_APPLY_DEPLOY, 1 );
    group_set_active( gui->deploy_window, ID_CANCEL_DEPLOY, !deploy_turn );
}

/*
====================================================================
Show/Hide game menu.
====================================================================
*/
static void engine_show_game_menu( int cx, int cy )
{
    char str[MAX_NAME];
    if ( setup.type == SETUP_RUN_TITLE ) {
        status = STATUS_TITLE_MENU;
        if ( cy + gui->main_menu->frame->img->img->h >= sdl.screen->h )
            cy = sdl.screen->h - gui->main_menu->frame->img->img->h;
        group_move( gui->main_menu, cx, cy );
        group_hide( gui->main_menu, 0 );
        group_set_active( gui->main_menu, ID_SAVE, 0 );
        group_set_active( gui->main_menu, ID_RESTART, 0 );
        snprintf( str, MAX_NAME, tr("Mod: %s"), config.mod_name );
        label_write( gui->label_left, gui->font_std, str );
        label_write( gui->label_center, gui->font_std, "" );
        label_write( gui->label_right, gui->font_std, "" );
    }
    else {
        engine_check_menu_buttons();
        gui_show_menu( cx, cy );
        status = STATUS_GAME_MENU;
        group_set_active( gui->main_menu, ID_SAVE, 1 );
        group_set_active( gui->main_menu, ID_RESTART, 1 );
    }
    /* lock config buttons */
    group_lock_button( gui->opt_menu, ID_C_SUPPLY, config.supply );
    group_lock_button( gui->opt_menu, ID_C_WEATHER, config.weather );
    group_lock_button( gui->opt_menu, ID_C_GRID, config.grid );
    group_lock_button( gui->opt_menu, ID_C_SHOW_CPU, config.show_cpu_turn );
    group_lock_button( gui->opt_menu, ID_C_SHOW_STRENGTH, config.show_bar );
    group_lock_button( gui->opt_menu, ID_C_SOUND, config.sound_on );
}
static void engine_hide_game_menu()
{
    if ( setup.type == SETUP_RUN_TITLE ) {
        status = STATUS_TITLE;
    }
    else
        engine_set_status( STATUS_NONE );
    group_hide( gui->base_menu, 1 );
    group_hide( gui->main_menu, 1 );
    fdlg_hide( gui->save_menu, 1 );
    fdlg_hide( gui->load_menu, 1 );
    group_hide( gui->opt_menu, 1 );
    old_mx = old_my = -1;
}

/*
====================================================================
Show/Hide unit menu.
====================================================================
*/
static void engine_show_unit_menu( int cx, int cy )
{
    engine_check_unit_buttons();
    gui_show_unit_buttons( cx, cy );
    status = STATUS_UNIT_MENU;
}
static void engine_hide_unit_menu()
{
    engine_set_status( STATUS_NONE );
    group_hide( gui->unit_buttons, 1 );
    group_hide( gui->split_menu, 1 );
    old_mx = old_my = -1;
}

/*
====================================================================
Initiate the confirmation window. (if this returnes ID_CANCEL
the last action stored will be removed)
====================================================================
*/
static void engine_confirm_action( const char *text )
{
    top_committed_action = 0;
    gui_show_confirm( text );
    status = STATUS_CONF;
}

/*
====================================================================
Initiate the confirmation window. If this returnes ID_CANCEL
all actions downto but excluding 'last_persistent_action' are
removed. If 'last_persistent_action' does not exist, all
actions are removed.
====================================================================
*/
static void engine_confirm_actions( Action *last_persistent_action, const char *text )
{
    top_committed_action = last_persistent_action;
    gui_show_confirm( text );
    status = STATUS_CONF;
}

/*
====================================================================
Display final scenario message (called when scen_check_result()
returns True).
====================================================================
*/
static void engine_show_final_message()
{
    event_wait_until_no_input();
    SDL_FillRect( sdl.screen, 0, 0x0 );
    gui->font_turn_info->align = ALIGN_X_CENTER | ALIGN_Y_CENTER;
    write_text( gui->font_turn_info, sdl.screen, sdl.screen->w / 2, sdl.screen->h / 2, scen_get_result_message(), 255 );
    refresh_screen( 0, 0, 0, 0 );
    while ( !engine_status_screen_dismissed() ) { 
        SDL_PumpEvents(); 
        SDL_Delay( 20 ); 
    }
    event_clear(); 
}

/*
====================================================================
Show turn info (done before turn)
====================================================================
*/
static void engine_show_turn_info()
{
    int text_x, text_y;
    int time_factor;
    char text[400];
    FULL_DEST( sdl.screen );
    fill_surf( 0x0 );
#if 0
#  define i time_factor 
    gui->font_std->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    text_x = text_y = 0;
    write_line( sdl.screen, gui->font_std, "Charset Test (latin1): Flöße über Wasser. \241Señálalo!", text_x, &text_y );
    text[32] = 0;
    for (i = 0; i < 256; i++) {
        text[i % 32] = i ? i : 127;
        if ((i + 1) % 32 == 0)
            write_line( sdl.screen, gui->font_std, text, text_x, &text_y );
    }
#  undef i
#endif
    text_x = sdl.screen->w >> 1;
    text_y = ( sdl.screen->h - 4 * gui->font_turn_info->height ) >> 1;
    gui->font_turn_info->align = ALIGN_X_CENTER | ALIGN_Y_TOP;
    scen_get_date( text );
    write_text( gui->font_turn_info, sdl.screen, text_x, text_y, text, OPAQUE );
    text_y += gui->font_turn_info->height;
    sprintf( text, tr("Next Player: %s"), cur_player->name );
    write_text( gui->font_turn_info, sdl.screen, text_x, text_y, text, OPAQUE );
    text_y += gui->font_turn_info->height;
                            
    if ( deploy_turn ) {
        if ( cur_player->ctrl == PLAYER_CTRL_HUMAN ) {
            text_y += gui->font_turn_info->height;
            write_text( gui->font_turn_info, sdl.screen, text_x, text_y, tr("Deploy your troops"), OPAQUE );
            text_y += gui->font_turn_info->height;
            refresh_screen( 0, 0, 0, 0 );
            SDL_PumpEvents(); event_clear();
            while ( !engine_status_screen_dismissed() )
                SDL_PumpEvents(); SDL_Delay( 20 );
            event_clear();
        }
        /* don't show screen for computer-controlled players */
        return;
    }
    
    if ( turn + 1 < scen_info->turn_limit ) {
        sprintf( text, tr("Remaining Turns: %i"), scen_info->turn_limit - turn );
        write_text( gui->font_turn_info, sdl.screen, text_x, text_y, text, OPAQUE );
        text_y += gui->font_turn_info->height;
    }
    sprintf( text, tr("Weather: %s (%s)"), weather_types[scen_get_weather()].name,
                                           weather_types[scen_get_weather()].ground_conditions );
    write_text( gui->font_turn_info, sdl.screen, text_x, text_y, text, OPAQUE );
    text_y += gui->font_turn_info->height;
    if ( turn + 1 < scen_info->turn_limit )
        sprintf( text, tr("Turn: %d"), turn + 1 );
    else
        sprintf( text, tr("Last Turn") );
    write_text( gui->font_turn_info, sdl.screen, text_x, text_y, text, OPAQUE );
    refresh_screen( 0, 0, 0, 0 );
    SDL_PumpEvents(); event_clear();
    time_factor = 3000/20;		/* wait 3 sec */
    while ( !engine_status_screen_dismissed() && time_factor ) {
        SDL_PumpEvents(); SDL_Delay( 20 );
	if (cur_ctrl != PLAYER_CTRL_HUMAN) time_factor--;
    }
    event_clear();
}

/*
====================================================================
Backup data that will be restored when unit move was undone.
(destination flag, spot mask, unit position)
If x != -1 the flag at x,y will be saved.
====================================================================
*/
static void engine_backup_move( Unit *unit, int x, int y )
{
    if ( move_backup.used == 0 ) {
        move_backup.used = 1;
        memcpy( &move_backup.unit, unit, sizeof( Unit ) );
        map_backup_spot_mask();
    }
    if ( x != -1 ) {
        move_backup.dest_nation = map[x][y].nation;
        move_backup.dest_player = map[x][y].player;
        move_backup.flag_saved = 1;
    }
    else
        move_backup.flag_saved = 0;
}
static void engine_undo_move( Unit *unit )
{
    int new_embark;
    if ( !move_backup.used ) return;
    map_remove_unit( unit );
    if ( move_backup.flag_saved ) {
        map[unit->x][unit->y].player = move_backup.dest_player;
        map[unit->x][unit->y].nation = move_backup.dest_nation;
        move_backup.flag_saved = 0;
    }
    /* get stuff before restoring pointer */
    new_embark = unit->embark;
    /* restore */
    memcpy( unit, &move_backup.unit, sizeof( Unit ) );
    /* check debark/embark counters */
    if ( unit->embark == EMBARK_NONE ) {
        if ( new_embark == EMBARK_AIR )
            unit->player->air_trsp_used--;
        if ( new_embark == EMBARK_SEA )
            unit->player->sea_trsp_used--;
    }
    else
        if ( unit->embark == EMBARK_SEA && new_embark == EMBARK_NONE )
            unit->player->sea_trsp_used++;
        else
            if ( unit->embark == EMBARK_AIR && new_embark == EMBARK_NONE )
                unit->player->air_trsp_used++;
    unit_adjust_icon( unit ); /* adjust picture as direction may have changed */
    map_insert_unit( unit );
    map_restore_spot_mask();
    if ( modify_fog ) map_set_fog( F_SPOT );
    move_backup.used = 0;
}
static void engine_clear_backup()
{
    move_backup.used = 0;
    move_backup.flag_saved = 0;
}

/*
====================================================================
Remove unit from map and unit list and clear it's influence.
====================================================================
*/
static void engine_remove_unit( Unit *unit )
{
    if (unit->killed >= 3) return;

    /* check if it's an enemy to the current player; if so the influence must be removed */
    if ( !player_is_ally( cur_player, unit->player ) )
        map_remove_unit_infl( unit );
    map_remove_unit( unit );
    /* from unit list (if not core unit) */
    if (!unit->core)
        unit->killed = 3;
    else
        unit->killed = 2;
}

/*
====================================================================
Select this unit and unselect old selection if nescessary.
Clear the selection if NULL is passed as unit.
====================================================================
*/
static void engine_select_unit( Unit *unit )
{
    /* select unit */
    cur_unit = unit;
    engine_clear_danger_mask();
    if ( cur_unit == 0 ) {
        /* clear view */
        if ( modify_fog ) map_set_fog( F_SPOT );
        engine_clear_backup();
        return;
    }
    /* switch air/ground */
    if ( unit->sel_prop->flags & FLYING )
        air_mode = 1;
    else
        air_mode = 0;
    /* get merge partners and set merge_unit mask */
    map_get_merge_units( cur_unit, merge_units, &merge_unit_count );
    /* moving range */
    map_get_unit_move_mask( unit );
    if ( modify_fog && unit->cur_mov > 0 ) {
        map_set_fog( F_IN_RANGE );
        mask[unit->x][unit->y].fog = 0;
    }
    else
        map_set_fog( F_SPOT );
    /* determine danger zone for air units */
    if ( modify_fog && config.supply && unit->cur_mov
         && (unit->sel_prop->flags & FLYING) && unit->sel_prop->fuel)
        has_danger_zone = map_get_danger_mask( unit );
    return;
}

/*
====================================================================
Return current units in avail_units to reinf list. Get all valid
reinforcements for the current player from reinf and put them to
avail_units. Aircrafts come first.
====================================================================
*/
static void engine_update_avail_reinf_list()
{
    Unit *unit;
    int i;
    /* available reinforcements */
    if ( avail_units )
    {
        list_reset( avail_units );
        while ( avail_units->count > 0 )
            list_transfer( avail_units, reinf, list_first( avail_units ) );
        /* add all units from scen::reinf whose delay <= cur_turn */
        list_reset( reinf );
        for ( i = 0; i < reinf->count; i++ ) {
            unit = list_next( reinf );
            if ( unit->sel_prop->flags & FLYING && unit->player == cur_player && unit->delay <= turn ) {
                list_transfer( reinf, avail_units, unit );
                /* index must be reset if unit was added */
                i--;
            }
        }
    }
    if ( reinf )
    {
        list_reset( reinf );
        for ( i = 0; i < reinf->count; i++ ) {
            unit = list_next( reinf );
            if ( !(unit->sel_prop->flags & FLYING) && unit->player == cur_player && unit->delay <= turn ) {
                list_transfer( reinf, avail_units, unit );
                /* index must be reset if unit was added */
                i--;
            }
        }
    }
}

/*
====================================================================
Initiate player as current player and prepare its turn.
If 'skip_unit_prep' is set scen_prep_unit() is not called.
====================================================================
*/
static void engine_select_player( Player *player, int skip_unit_prep )
{
    Player *human;
    int i, human_count, x, y;
    Unit *unit;
	
    cur_player = player;
    if ( player )
        cur_ctrl = player->ctrl;
    else
        cur_ctrl = PLAYER_CTRL_NOBODY;
    if ( !skip_unit_prep ) {
        /* update available reinforcements */
        engine_update_avail_reinf_list();
        /* prepare units for turn -- fuel, mov-points, entr, weather etc */
        if (units)
        {
            list_reset( units );
            for ( i = 0; i < units->count; i++ ) {
                unit = list_next( units );
                if ( unit->player == cur_player ) {
                    if ( turn == 0 )
                        scen_prep_unit( unit, SCEN_PREP_UNIT_FIRST );
                    else
                        scen_prep_unit( unit, SCEN_PREP_UNIT_NORMAL );
                }
            }
        }
    }
    /* set fog */
    switch ( cur_ctrl ) {
        case PLAYER_CTRL_HUMAN:
            modify_fog = 1;
            map_set_spot_mask(); 
            map_set_fog( F_SPOT );
            break;
        case PLAYER_CTRL_NOBODY:
            for ( x = 0; x < map_w; x++ )
                for ( y = 0; y < map_h; y++ )
                    mask[x][y].spot = 1;
            map_set_fog( 0 );
            break;
        case PLAYER_CTRL_CPU:
            human = engine_human_player( &human_count );
            if ( human_count == 1 ) {
                modify_fog = 0;
                map_set_spot_mask();
                map_set_fog_by_player( human );
            }
            else {
                modify_fog = 1;
                map_set_spot_mask(); 
                map_set_fog( F_SPOT );
            }
            break;
    }
    /* count down deploy center delay's (1==deploy center again) */
    if ( !skip_unit_prep )
	    for (x=0;x<map_w;x++)
		for (y=0;y<map_h;y++)
		    if (map[x][y].deploy_center > 1 && map[x][y].player == 
								cur_player)
			map[x][y].deploy_center--;
    /* set influence mask */
    if ( cur_ctrl != PLAYER_CTRL_NOBODY )
        map_set_infl_mask();
    map_get_vis_units();
    if ( !skip_unit_prep) {

        /* prepare deployment dialog on deployment-turn */
        if ( deploy_turn
             && cur_player && cur_player->ctrl == PLAYER_CTRL_HUMAN )
            engine_handle_button( ID_DEPLOY );
        else
            group_hide( gui->deploy_window, 1 );

        /* supply levels */
        if ( units )
        {
            list_reset( units );
            while ( ( unit = list_next( units ) ) )
                if ( unit->player == cur_player )
                    scen_adjust_unit_supply_level( unit );
        }        
        /* mark unit as re-deployable in deployment-turn when it is
         * located on a deployment-field and is allowed to be re-deployed.
         */
        if (deploy_turn)
        {
            map_get_deploy_mask(cur_player,0,1);
            list_reset( units );
            while ( ( unit = list_next( units ) ) ) {
                if ( unit->player == cur_player )
                if ( mask[unit->x][unit->y].deploy )
                if ( unit_supports_deploy(unit) ) {
                    unit->fresh_deploy = 1;
                    list_transfer( units, avail_units, unit );
                }
            }
        }
    }
    /* clear selections/actions */
    cur_unit = cur_target = cur_atk = cur_def = surp_unit = move_unit = deploy_unit = 0;
    merge_unit_count = 0;
    list_clear( df_units );
    actions_clear();
    scroll_block = 0;
}

/*
====================================================================
Begin turn of next player. Therefore select next player or use
'forced_player' if not NULL (then the next is the one after 
'forced_player').
If 'skip_unit_prep' is set scen_prep_unit() is not called.
====================================================================
*/
static void engine_begin_turn( Player *forced_player, int skip_unit_prep )
{
    char text[400];
    int new_turn = 0;
    Player *player = 0;
    /* clear various stuff that may be still set from last turn */
    group_set_active( gui->confirm, ID_OK, 1 );
    engine_hide_unit_menu();
    engine_hide_game_menu();
    /* clear undo */
    engine_clear_backup();
    /* clear hideous clicks */
    if ( !deploy_turn && cur_ctrl == PLAYER_CTRL_HUMAN ) event_wait_until_no_input();
    /* get player */
    if ( forced_player == 0 ) {
        /* next player and turn */
        player = players_get_next( &new_turn );
        if ( new_turn )  {
            turn++;
        }
        if ( turn == scen_info->turn_limit ) {
            /* use else condition as scenario result */
            /* and take a final look */
            if (camp_loaded == FIRST_SCENARIO)
                camp_loaded = CONTINUE_CAMPAIGN;
            scen_check_result( 1 );
            blind_cpu_turn = 0;
            engine_show_final_message();
            draw_map = 1;
            image_hide( gui->cursors, 0 );
            gui_set_cursor( CURSOR_STD );
            engine_select_player( 0, skip_unit_prep );
            engine_set_status( STATUS_NONE ); 
            phase = PHASE_NONE;
            return;
        }
        else {
            cur_weather = scen_get_weather();
            engine_select_player( player, skip_unit_prep );
        }
    }
    else {
        engine_select_player( forced_player, skip_unit_prep );
        players_set_current( player_get_index( forced_player ) );
    }

    /* Add prestige for regular turn begin (skip_unit_prep means game has been
     * loaded). */
    if (!deploy_turn && !skip_unit_prep)
        cur_player->cur_prestige += cur_player->prestige_per_turn[turn];

#if DEBUG_CAMPAIGN
    if ( scen_check_result(0) ) {
        blind_cpu_turn = 0;
        engine_show_final_message();
        draw_map = 1;
        image_hide( gui->cursors, 0 );
        gui_set_cursor( CURSOR_STD );
        engine_select_player( 0, skip_unit_prep );
        engine_set_status( STATUS_NONE ); 
        phase = PHASE_NONE;
        end_scen = 1;
        return;
    }
#endif
    /* init ai turn if any */
    if ( cur_player && cur_player->ctrl == PLAYER_CTRL_CPU )
        (cur_player->ai_init)();
    /* turn info */
    engine_show_turn_info();
    engine_set_status( deploy_turn ? STATUS_DEPLOY : STATUS_NONE );
    phase = PHASE_NONE;
    /* update screen */
    if ( cur_ctrl != PLAYER_CTRL_CPU || config.show_cpu_turn ) {
        if ( cur_ctrl == PLAYER_CTRL_CPU )
            engine_update_info( 0, 0, 0 );
        else {
            image_hide( gui->cursors, 0 );
        }
        engine_draw_map();
        refresh_screen( 0, 0, 0, 0 );
        blind_cpu_turn = 0;
    }
    else {
        engine_update_info( 0, 0, 0 );
        draw_map = 0;
        FULL_DEST( sdl.screen );
        fill_surf( 0x0 );
        gui->font_turn_info->align = ALIGN_X_CENTER | ALIGN_Y_CENTER;
        sprintf( text, tr("CPU thinks...") );
        write_text( gui->font_turn_info, sdl.screen, sdl.screen->w >> 1, sdl.screen->h >> 1, text, OPAQUE );
        sprintf( text, tr("( Enable option 'Show Cpu Turn' if you want to see what it is doing. )") );
        write_text( gui->font_turn_info, sdl.screen, sdl.screen->w >> 1, ( sdl.screen->h >> 1 )+ 20, text, OPAQUE );
        refresh_screen( 0, 0, 0, 0 );
        blind_cpu_turn = 1;
    }
}

/*
====================================================================
End turn of current player without selecting next player. Here 
autosave happens, aircrafts crash, units get supplied.
====================================================================
*/
static void engine_end_turn()
{
    int i;
    char autosaveName[25];
    Unit *unit;
    /* finalize ai turn if any */
    if ( cur_player && cur_player->ctrl == PLAYER_CTRL_CPU )
        (cur_player->ai_finalize)();
    /* if turn == scen_info->turn_limit this was a final look */
    if ( turn == scen_info->turn_limit ) {
        end_scen = 1;
        return;
    }
    /* autosave game for a human */
    if (!deploy_turn && cur_player && cur_player->ctrl == PLAYER_CTRL_HUMAN)
    {
        snprintf( autosaveName, 25, "Autosave");// (%s), %s, %s",  );
        slot_save( autosaveName, "" );
    }
    /* fuel up and kill crashed aircrafts*/
    list_reset( units );
    while ( (unit=list_next(units)) )
    {
        if ( unit->player != cur_player ) continue;
        /* supply unused ground units just as if it were done manually and
           any aircraft with the updated supply level */
        if (config.supply && unit_check_fuel_usage( unit ))
        {
            if (unit->prop.flags & FLYING)
            {
                /* loose half fuel even if not moved */
                if (unit->cur_mov>0) /* FIXME: this goes wrong if units may move multiple times */
                {
                    unit->cur_fuel -= unit_calc_fuel_usage(unit,0);
                    if (unit->cur_fuel<0) unit->cur_fuel = 0;
                }
                /* supply? */
                scen_adjust_unit_supply_level( unit );        
                if ( unit->supply_level > 0 )
                {
                    unit->unused = 1; /* required to fuel up */
                    unit_supply( unit, UNIT_SUPPLY_ALL ); 
                }
                /* crash if no fuel */
                if (unit->cur_fuel == 0)
                    unit->killed = 1;
            }
            else if ( unit->unused && unit->supply_level > 0 )
                unit_supply( unit, UNIT_SUPPLY_ALL );
        }
    }
    /* remove all units that were killed in the last turn */
    list_reset( units );
    for ( i = 0; i < units->count; i++ ) 
    {
        unit = list_next(units);
        if ( unit->killed ) {
            engine_remove_unit( unit );
            if (unit->killed != 2)
            {
                list_delete_item( units, unit );
                i--; /* adjust index */
            }
        }
    }
}

/*
====================================================================
Get map/screen position from cursor/map position.
====================================================================
*/
static int engine_get_screen_pos( int mx, int my, int *sx, int *sy )
{
    int x = map_sx, y = map_sy;
    /* this is the starting position if x-pos of first tile on screen is not odd */
    /* if it is odd we must add the y_offset to the starting position */
    if ( ODD( map_x ) )
        y += hex_y_offset;
    /* reduce to visible map tiles */
    mx -= map_x;
    my -= map_y;
    /* check range */
    if ( mx < 0 || my < 0) return 0;
    /* compute pos */
    x += mx * hex_x_offset;
    y += my * hex_h;
    /* if x_pos of first tile is even we must add y_offset to the odd tiles in screen */
    if ( EVEN( map_x ) ) {
        if ( ODD( mx ) )
            y += hex_y_offset;
    }
    else {
        /* we must substract y_offset from even tiles */
        if ( ODD( mx ) )
            y -= hex_y_offset;
    }
    /* check range */
    if ( x >= gui->map_frame->w || y >= gui->map_frame->h ) return 0;
    /* assign */
    *sx = x;
    *sy = y + 30;
    return 1;
}
enum {
    REGION_GROUND = 0,
    REGION_AIR
};
static int engine_get_map_pos( int sx, int sy, int *mx, int *my, int *region )
{
    int x = 0, y = 0;
    int screen_x, screen_y;
    int tile_x, tile_y;
    int total_y_offset;
    if ( status == STATUS_STRAT_MAP ) {
        /* strategic map */
        if ( strat_map_get_pos( sx, sy, mx, my ) ) {
            if ( *mx < 0 || *my < 0 || *mx >= map_w || *my >= map_h ) 
                return 0;
            return 1;
        }
        return 0;
    }
    /* get the map offset in screen from mouse position */
    x = ( sx - map_sx ) / hex_x_offset;
    /* y value computes the same like the x value but their may be an offset of engine::y_offset */
    total_y_offset = 0;
    if ( EVEN( map_x ) && ODD( x ) )
        total_y_offset = hex_y_offset;
    /* if engine::map_x is odd there must be an offset of engine::y_offset for the start of first tile */
    /* and all odd tiles receive an offset of -engine::y_offset so the result is:
        odd: offset = 0 even: offset = engine::y_offset it's best to draw this ;-D
    */
    if ( ODD( map_x ) && EVEN( x ) )
        total_y_offset = hex_y_offset;
    y = ( sy - 30 - total_y_offset - map_sy ) / hex_h;
    /* compute screen position */
    if ( !engine_get_screen_pos( x + map_x, y + map_y, &screen_x, &screen_y ) ) return 0;
    /* test mask with  sx - screen_x, sy - screen_y */
    tile_x = sx - screen_x;
    tile_y = sy - screen_y;
    if ( !hex_mask[tile_y * hex_w + tile_x] ) {
        if ( EVEN( map_x ) ) {
            if ( tile_y < hex_y_offset && EVEN( x ) ) y--;
            if ( tile_y >= hex_y_offset && ODD( x ) ) y++;
        }
        else {
            if ( tile_y < hex_y_offset && ODD( x ) ) y--;
            if ( tile_y >= hex_y_offset && EVEN( x ) ) y++;
        }
        x--;
    }
    /* region */
    if ( tile_y < ( hex_h >> 1 ) )
        *region = REGION_AIR;
    else
        *region = REGION_GROUND;
    /* add engine map offset and assign */
    x += map_x;
    y += map_y;
    *mx = x;
    *my = y;
    /* check range */
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) return 0;
    /* ok, tile exists */
    return 1;
}

/*
====================================================================
If x,y is not on screen center this map tile and check if 
screencopy is possible (but only if use_sc is True)
====================================================================
*/
static int engine_focus( int x, int y, int use_sc )
{
    int new_x, new_y;
    if ( x <= map_x + 1 || y <= map_y + 1 || x >= map_x + map_sw - 1 - 2 || y >= map_y + map_sh - 1 - 2 ) {
        new_x = x - ( map_sw >> 1 );
        new_y = y - ( map_sh >> 1 );
        if ( new_x & 1 ) new_x++;
        if ( new_y & 1 ) new_y++;
        engine_goto_xy( new_x, new_y );
        if ( !use_sc ) sc_type = SC_NONE; /* no screencopy */
        return 1;
    }
    return 0;
}
/*
====================================================================
Move to this position and set 'draw_map' if actually moved.
====================================================================
*/
static void engine_goto_xy( int x, int y )
{
    int x_diff, y_diff;
    /* check range */
    if ( x < 0 ) x = 0;
    if ( y < 0 ) y = 0;
    /* if more tiles are displayed then map has ( black space the rest ) no change in position allowed */
    if ( map_sw >= map_w ) x = 0;
    else
        if ( x > map_w - map_sw )
            x = map_w - map_sw;
    if ( map_sh >= map_h ) y = 0;
    else
        if ( y > map_h - map_sh )
            y = map_h - map_sh;
    /* check if screencopy is possible */
    x_diff = x - map_x;
    y_diff = y - map_y;
    /* if one diff is ==0 and one diff !=0 do it! */
    if ( x_diff == 0 && y_diff != 0 ) {
        sc_type = SC_VERT;
        sc_diff = y_diff;
    }
    else
        if ( x_diff != 0 && y_diff == 0 ) {
            sc_type = SC_HORI;
            sc_diff = x_diff;
        }
    /* actually moving? */
    if ( x != map_x || y != map_y ) {
        map_x = x; map_y = y;
        draw_map = 1;
    }
}

/*
====================================================================
Check if mouse position is in scroll region or key is pressed
and scrolling is possible.
If 'by_wheel' is true scroll_hori/vert has been by using
the mouse wheel and checking the keys/mouse must be skipped.
====================================================================
*/
enum {
    SC_NORMAL = 0,
    SC_BY_WHEEL
};
static void engine_check_scroll(int by_wheel)
{
    int region;
    int tol = 3; /* border in which scrolling by mouse */
    int mx, my, cx, cy;
    if ( scroll_block ) return;
    if ( setup.type == SETUP_RUN_TITLE ) return;
    if( !by_wheel ) {
        /* keys */
        scroll_hori = scroll_vert = SCROLL_NONE;
        if ( !scroll_block_keys ) {
            if ( event_check_key( SDLK_UP ) && map_y > 0) 
                scroll_vert = SCROLL_UP;
            else
                if ( event_check_key( SDLK_DOWN ) && map_y < map_h - map_sh )
                    scroll_vert = SCROLL_DOWN;
            if ( event_check_key( SDLK_LEFT ) && map_x > 0 ) 
                scroll_hori = SCROLL_LEFT;
            else
                if ( event_check_key( SDLK_RIGHT ) && map_x < map_w - map_sw )
                    scroll_hori = SCROLL_RIGHT;
        }
        if ( scroll_vert == SCROLL_NONE && scroll_hori == SCROLL_NONE ) {
            /* mouse */
            event_get_cursor_pos( &mx, &my );
            if ( my <= tol && map_y > 0 )
                scroll_vert = SCROLL_UP;
            else
                if ( mx >= sdl.screen->w - tol - 1 && map_x < map_w - map_sw )
                    scroll_hori = SCROLL_RIGHT;
                else
                    if ( my >= sdl.screen->h - tol - 1 && map_y < map_h - map_sh )
                        scroll_vert = SCROLL_DOWN;
                    else
                        if ( mx <= tol && map_x > 0 )
                            scroll_hori = SCROLL_LEFT;
        }
    }
    /* scroll */
    if ( !( scroll_vert == SCROLL_NONE && scroll_hori == SCROLL_NONE ) &&
          ( gui->save_menu->group->frame->img->bkgnd->hide && gui->load_menu->group->frame->img->bkgnd->hide ) )
    {
        if ( scroll_vert == SCROLL_UP )
            engine_goto_xy( map_x, map_y - 2 );
        else
            if ( scroll_hori == SCROLL_RIGHT )
                engine_goto_xy( map_x + 2, map_y );
            else
                if ( scroll_vert == SCROLL_DOWN )
                    engine_goto_xy( map_x, map_y + 2 );
                else
                    if ( scroll_hori == SCROLL_LEFT )
                        engine_goto_xy( map_x - 2, map_y );
        event_get_cursor_pos( &cx, &cy );
        if(engine_get_map_pos( cx, cy, &mx, &my, &region ))
            engine_update_info( mx, my, region );
        if ( !by_wheel )
            scroll_block = 1;
    }
}

/*
====================================================================
Update full map.
====================================================================
*/
static void engine_draw_map()
{
    int x, y, abs_y;
    int i, j;
    int start_map_x, start_map_y, end_map_x, end_map_y;
    int buffer_height, buffer_width, buffer_offset;
    int use_frame = ( cur_ctrl != PLAYER_CTRL_CPU );
    enum Stage { DrawTerrain, DrawUnits, DrawDangerZone } stage = DrawTerrain;
    enum Stage top_stage = has_danger_zone ? DrawDangerZone : DrawUnits;
    
    /* reset_timer(); */
    
    draw_map = 0;
    
    if ( status == STATUS_STRAT_MAP ) {
        sc_type = SC_NONE;
        strat_map_draw();
        return;
    }
    
    if ( status == STATUS_TITLE || status == STATUS_TITLE_MENU ) {
        sc_type = SC_NONE;
        engine_draw_bkgnd();
        return;
    }
    
    /* screen copy? */
    start_map_x = map_x;
    start_map_y = map_y;
    end_map_x = map_x + map_sw;
    end_map_y = map_y + map_sh;
    if ( sc_type == SC_VERT ) {
        /* clear flag */
        sc_type = SC_NONE;
        /* set buffer offset and height */
        buffer_offset = abs( sc_diff ) * hex_h;
        buffer_height = gui->map_frame->h - buffer_offset;
        /* going down */
        if ( sc_diff > 0 ) {
            /* copy screen to buffer */
            DEST( sc_buffer, 0, 0, gui->map_frame->w, buffer_height );
            SOURCE( gui->map_frame, 0, buffer_offset );
            blit_surf();
            /* copy buffer to new pos */
            DEST( gui->map_frame, 0, 0, gui->map_frame->w, buffer_height );
            SOURCE( sc_buffer, 0, 0 );
            blit_surf();
            /* set loop range to redraw lower lines */
            start_map_y += map_sh - sc_diff - 2;
        }
        /* going up */
        else {
            /* copy screen to buffer */
            DEST( sc_buffer, 0, 0, gui->map_frame->w, buffer_height );
            SOURCE( gui->map_frame, 0, 0 );
            blit_surf();
            /* copy buffer to new pos */
            DEST( gui->map_frame, 0, buffer_offset, gui->map_frame->w, buffer_height );
            SOURCE( sc_buffer, 0, 0 );
            blit_surf();
            /* set loop range to redraw upper lines */
            end_map_y = map_y + abs( sc_diff ) + 1;
        }
    }
    else
        if ( sc_type == SC_HORI ) {
            /* clear flag */
            sc_type = SC_NONE;
            /* set buffer offset and width */
            buffer_offset = abs( sc_diff ) * hex_x_offset;
            buffer_width = gui->map_frame->w - buffer_offset;
            buffer_height = gui->map_frame->h;
            /* going right */
            if ( sc_diff > 0 ) {
                /* copy screen to buffer */
                DEST( sc_buffer, 0, 0, buffer_width, buffer_height );
                SOURCE( gui->map_frame, buffer_offset, 0 );
                blit_surf();
                /* copy buffer to new pos */
                DEST( gui->map_frame, 0, 0, buffer_width, buffer_height );
                SOURCE( sc_buffer, 0, 0 );
                blit_surf();
                /* set loop range to redraw right lines */
                start_map_x += map_sw - sc_diff - 2;
            }
            /* going left */
            else {
                /* copy screen to buffer */
                DEST( sc_buffer, 0, 0, buffer_width, buffer_height );
                SOURCE( gui->map_frame, 0, 0 );
                blit_surf();
                /* copy buffer to new pos */
                DEST( gui->map_frame, buffer_offset, 0, buffer_width, buffer_height );
                SOURCE( sc_buffer, 0, 0 );
                blit_surf();
                /* set loop range to redraw right lines */
                end_map_x = map_x + abs( sc_diff ) + 1;
            }
        }
    for (; stage <= top_stage; stage++) {
        /* start position for drawing */
        x = map_sx + ( start_map_x - map_x ) * hex_x_offset;
        y = map_sy + ( start_map_y - map_y ) * hex_h;
        /* end_map_xy must not exceed map's size */
        if ( end_map_x >= map_w ) end_map_x = map_w;
        if ( end_map_y >= map_h ) end_map_y = map_h;
        /* loop to draw map tile */
        for ( j = start_map_y; j < end_map_y; j++ ) {
            for ( i = start_map_x; i < end_map_x; i++ ) {
                /* update each map tile */
                if ( i & 1 )
                    abs_y = y + hex_y_offset;
                else
                    abs_y = y;
                switch (stage) {
                    case DrawTerrain:
                        map_draw_terrain( gui->map_frame, i, j, x, abs_y );
                        break;
                    case DrawUnits:
                        if ( cur_unit && cur_unit->x == i && cur_unit->y == j && status != STATUS_MOVE && mask[i][j].spot )
                            map_draw_units( gui->map_frame, i, j, x, abs_y, !air_mode, use_frame );
                        else
                            map_draw_units( gui->map_frame, i, j, x, abs_y, !air_mode, 0 );
                        break;
                    case DrawDangerZone:
                        if ( mask[i][j].danger )
                            map_apply_danger_to_tile( gui->map_frame, i, j, x, abs_y );
                        break;
                }
                x += hex_x_offset;
            }
            y += hex_h;
            x = map_sx + ( start_map_x - map_x ) * hex_x_offset;
        }
    }
    /* printf( "time needed: %i ms\n", get_time() ); */
}

/*
====================================================================
Get primary unit on tile.
====================================================================
*/
static Unit *engine_get_prim_unit( int x, int y, int region )
{
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) return 0;
    if ( region == REGION_AIR ) {
        if ( map[x][y].a_unit )
            return map[x][y].a_unit;
        else
            return map[x][y].g_unit;
    }
    else {
        if ( map[x][y].g_unit )
            return map[x][y].g_unit;
        else
            return map[x][y].a_unit;
    }
}

/*
====================================================================
Check if there is a target for current unit on x,y.
====================================================================
*/
static Unit* engine_get_target( int x, int y, int region )
{
    Unit *unit;
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) return 0;
    if ( !mask[x][y].spot ) return 0;
    if ( cur_unit == 0 ) return 0;
    if ( ( unit = engine_get_prim_unit( x, y, region ) ) )
        if ( unit_check_attack( cur_unit, unit, UNIT_ACTIVE_ATTACK ) )
            return unit;
    return 0;
/*    if ( region == REGION_AIR ) {
        if ( map[x][y].a_unit && unit_check_attack( cur_unit, map[x][y].a_unit, UNIT_ACTIVE_ATTACK ) )
            return map[x][y].a_unit;
        else
            if ( map[x][y].g_unit && unit_check_attack( cur_unit, map[x][y].g_unit, UNIT_ACTIVE_ATTACK ) )
                return map[x][y].g_unit;
            else
                return 0;
    }
    else {
        if ( map[x][y].g_unit && unit_check_attack( cur_unit, map[x][y].g_unit, UNIT_ACTIVE_ATTACK ) )
            return map[x][y].g_unit;
        else
            if ( map[x][y].a_unit && unit_check_attack( cur_unit, map[x][y].a_unit, UNIT_ACTIVE_ATTACK ) )
                return map[x][y].a_unit;
            else
                return 0;
    }*/
}

/*
====================================================================
Check if there is a selectable unit for current player on x,y
The currently selected unit is not counted as selectable. (though
a primary unit on the same tile may be selected if it's not
the current unit)
====================================================================
*/
static Unit* engine_get_select_unit( int x, int y, int region )
{
    if ( x < 0 || y < 0 || x >= map_w || y >= map_h ) return 0;
    if ( !mask[x][y].spot ) return 0;
    if ( region == REGION_AIR ) {
        if ( map[x][y].a_unit && map[x][y].a_unit->player == cur_player ) {
            if ( cur_unit == map[x][y].a_unit )
                return 0;
            else
                return map[x][y].a_unit;
        }
        else
            if ( map[x][y].g_unit && map[x][y].g_unit->player == cur_player )
                return map[x][y].g_unit;
            else
                return 0;
    }
    else {
        if ( map[x][y].g_unit && map[x][y].g_unit->player == cur_player ) {
            if ( cur_unit == map[x][y].g_unit )
                return 0;
            else
                return map[x][y].g_unit;
        }
        else
            if ( map[x][y].a_unit && map[x][y].a_unit->player == cur_player )
                return map[x][y].a_unit;
            else
                return 0;
    }
}

/*
====================================================================
Update the unit quick info and map tile info if map tile mx,my,
region has the focus. Also update the cursor.
====================================================================
*/
static void engine_update_info( int mx, int my, int region )
{
    Unit *unit1 = 0, *unit2 = 0, *unit;
    char str[MAX_LINE];
    int att_damage, def_damage;
    int moveCost = 0;
    /* no infos when cpu is acting */
    if ( cur_ctrl == PLAYER_CTRL_CPU ) {
        image_hide( gui->cursors, 1 );
        label_write( gui->label_left, gui->font_std, "" );
        label_write( gui->label_center, gui->font_status, "" );
        label_write( gui->label_right, gui->font_std, "" );
        frame_hide( gui->qinfo1, 1 );
        frame_hide( gui->qinfo2, 1 );
        return;
    }
	if ( cur_unit && cur_unit->cur_mov > 0 && mask[mx][my].in_range && 
         !mask[mx][my].blocked ) 
        moveCost = mask[mx][my].moveCost;
    /* entered a new tile so update the terrain info */
    if (status == STATUS_PURCHASE) {
        snprintf( str, MAX_LINE, tr("Prestige: %d"), cur_player->cur_prestige );
        label_write( gui->label_left, gui->font_std, str );
    }
    else if (status==STATUS_DROP)
    {
        label_write( gui->label_left, gui->font_std,tr("Select Drop Zone")  );
    }
    else if (status==STATUS_SPLIT)
    {
        sprintf( str, tr("Split Up %d Strength"), cur_split_str );
        label_write( gui->label_left, gui->font_std, str );
    }
    else if (moveCost>0)
    {
        sprintf( str, GS_DISTANCE "%d", moveCost );
        label_write( gui->label_left, gui->font_status, str );
        sprintf( str, "%s (%i,%i) ", map[mx][my].name, mx, my );
        label_write( gui->label_center, gui->font_status, str );
        snprintf( str, MAX_NAME, tr("Weather: %s"), weather_types[cur_weather].name );
        label_write( gui->label_right, gui->font_std, str );
    }
    else
    {
        snprintf( str, MAX_NAME, tr("Mod: %s"), config.mod_name );
        label_write( gui->label_left, gui->font_std, str );
        sprintf( str, "%s (%i,%i)", map[mx][my].name, mx, my );
        label_write( gui->label_center, gui->font_status, str );
        snprintf( str, MAX_NAME, tr("Weather: %s"), weather_types[cur_weather].name );
        label_write( gui->label_right, gui->font_std, str );
    }
    /* DEBUG: 
    sprintf( str, "(%d,%d), P: %d B: %d I: %d D: %d",mx,my,mask[mx][my].in_range-1,mask[mx][my].blocked,mask[mx][my].vis_infl,mask[mx][my].distance); 
    label_write( gui->label, gui->font_std, str ); */
    /* update the unit info */
    if ( !mask[mx][my].spot ) {
        if ( cur_unit )
            gui_show_quick_info( gui->qinfo1, cur_unit );
        else
            frame_hide( gui->qinfo1, 1 );
        frame_hide( gui->qinfo2, 1 );
    }
    else {
        if ( cur_unit && ( mx != cur_unit->x || my != cur_unit->y ) ) {
            unit1 = cur_unit;
            unit2 = engine_get_prim_unit( mx, my, region );
        }
        else {
            if ( map[mx][my].a_unit && map[mx][my].g_unit ) {
                unit1 = map[mx][my].g_unit;
                unit2 = map[mx][my].a_unit;
            }
            else
                if ( map[mx][my].a_unit )
                    unit1 = map[mx][my].a_unit;
                else
                    if ( map[mx][my].g_unit )
                        unit1 = map[mx][my].g_unit;
        } 
        if ( unit1 )
            gui_show_quick_info( gui->qinfo1, unit1 );
        else
            frame_hide( gui->qinfo1, 1 );
        if ( unit2 && status != STATUS_UNIT_MENU )
            gui_show_quick_info( gui->qinfo2, unit2 );
        else
            frame_hide( gui->qinfo2, 1 );
        /* show expected losses? */
        if ( cur_unit && ( unit = engine_get_target( mx, my, region ) ) ) {
            unit_get_expected_losses( cur_unit, unit, &att_damage, &def_damage );
            gui_show_expected_losses( cur_unit, unit, att_damage, def_damage );
        }
/*        if ( unit1 && unit2 ) {
            if ( engine_get_target( mx, my, region ) )
            if ( unit1 == cur_unit && unit_check_attack( unit1, unit2, UNIT_ACTIVE_ATTACK ) ) {
                unit_get_expected_losses( unit1, unit2, map[unit2->x][unit2->y].terrain, &att_damage, &def_damage );
                gui_show_expected_losses( unit1, unit2, att_damage, def_damage );
            }
            else
            if ( unit2 == cur_unit && unit_check_attack( unit2, unit1, UNIT_ACTIVE_ATTACK ) ) {
                unit_get_expected_losses( unit2, unit1, map[unit1->x][unit1->y].terrain, &att_damage, &def_damage );
                gui_show_expected_losses( unit2, unit1, att_damage, def_damage );
            }
        }*/
    }
    if ( cur_player == 0 )
        gui_set_cursor( CURSOR_STD );
    else
    /* cursor */
    switch ( status ) {
        case STATUS_TITLE:
        case STATUS_TITLE_MENU:
        case STATUS_STRAT_MAP:
        case STATUS_GAME_MENU:
        case STATUS_UNIT_MENU:
            gui_set_cursor( CURSOR_STD );
            break;
        case STATUS_DROP:
            if ( mask[mx][my].deploy )
                gui_set_cursor( CURSOR_DEBARK );
            else
                gui_set_cursor( CURSOR_STD );
            break;
        case STATUS_MERGE:
            if ( mask[mx][my].merge_unit )
                gui_set_cursor( CURSOR_UNDEPLOY );
            else
                gui_set_cursor( CURSOR_STD );
            break;
        case STATUS_SPLIT:
            if (mask[mx][my].split_unit||mask[mx][my].split_okay)
                gui_set_cursor( CURSOR_DEPLOY );
            else
                gui_set_cursor( CURSOR_STD );
            break;
        case STATUS_DEPLOY:
            if (map_get_undeploy_unit(mx,my,region==REGION_AIR))
                gui_set_cursor( CURSOR_UNDEPLOY );
            else
                if (deploy_unit&&mask[mx][my].deploy)
                    gui_set_cursor( CURSOR_DEPLOY );
                else if (map_get_undeploy_unit(mx,my,region!=REGION_AIR))
                    gui_set_cursor( CURSOR_UNDEPLOY );
                else
                    gui_set_cursor( CURSOR_STD );
            break;
        default:
            if ( cur_unit ) {
                if ( cur_unit->x == mx && cur_unit->y == my && engine_get_prim_unit( mx, my, region ) == cur_unit )
                     gui_set_cursor( CURSOR_STD );
                else
                /* unit selected */
                if ( engine_get_target( mx, my, region ) )
                    gui_set_cursor( CURSOR_ATTACK );
                else
                    if ( mask[mx][my].in_range && ( cur_unit->x != mx || cur_unit->y != my ) && !mask[mx][my].blocked ) {
                        if ( mask[mx][my].mount )
                            gui_set_cursor( CURSOR_MOUNT );
                        else
                            gui_set_cursor( CURSOR_MOVE );
                    }
                    else
                        if ( mask[mx][my].sea_embark ) {
                            if ( cur_unit->embark == EMBARK_SEA )
                                gui_set_cursor( CURSOR_DEBARK );
                            else
                                gui_set_cursor( CURSOR_EMBARK );
                        }
                        else
                            if ( engine_get_select_unit( mx, my, region ) )
                                gui_set_cursor( CURSOR_SELECT );
                            else
                                gui_set_cursor( CURSOR_STD );
            }
            else {
                /* no unit selected */
                if ( engine_get_select_unit( mx, my, region ) )
                    gui_set_cursor( CURSOR_SELECT );
                else
                    gui_set_cursor( CURSOR_STD );
            }
            break;
    }
    /* new unit info */
    if ( status == STATUS_INFO || status == STATUS_DEPLOY_INFO ) {
        if ( engine_get_prim_unit( mx, my, region ) )
            if ( mask[mx][my].spot )
                gui_show_full_info( engine_get_prim_unit( mx, my, region ) );
    }
}

/*
====================================================================
Hide all animated toplevel windows.
====================================================================
*/
static void engine_begin_frame()
{
    if ( status == STATUS_ATTACK ) {
        anim_draw_bkgnd( terrain_icons->cross );
        anim_draw_bkgnd( terrain_icons->expl1 );
        anim_draw_bkgnd( terrain_icons->expl2 );
    }
    if ( status == STATUS_MOVE && move_image )
        image_draw_bkgnd( move_image );
    gui_draw_bkgnds();
}
/*
====================================================================
Handle all requested screen updates, draw the windows
and refresh the screen.
====================================================================
*/
static void engine_end_frame()
{
    int full_refresh = 0;
//    if ( blind_cpu_turn ) return;
    if ( draw_map ) {
        engine_draw_map();
        full_refresh = 1;
    }
    if ( status == STATUS_ATTACK ) {
        anim_get_bkgnd( terrain_icons->cross );
        anim_get_bkgnd( terrain_icons->expl1 );
        anim_get_bkgnd( terrain_icons->expl2 );
    }
    if ( status == STATUS_MOVE && move_image ) /* on surprise attack this image ain't created yet */
        image_get_bkgnd( move_image );
    gui_get_bkgnds();
    if ( status == STATUS_ATTACK ) {
        anim_draw( terrain_icons->cross );
        anim_draw( terrain_icons->expl1 );
        anim_draw( terrain_icons->expl2 );
    }
    if ( status == STATUS_MOVE && move_image ) {
        image_draw( move_image );
    }
    gui_draw();
    if ( full_refresh )
        refresh_screen( 0, 0, 0, 0 );
    else
        refresh_rects();
}

/*
====================================================================
Handle a button that was clicked.
====================================================================
*/
static void engine_handle_button( int id )
{
    char path[MAX_PATH];
    char str[MAX_LINE];
    int x, y, i, max;
    Unit *unit;
    Action *last_act;

    switch ( id ) {
        /* loads */
        case ID_LOAD_OK:
            fdlg_hide( gui->load_menu, 1 );
            engine_hide_game_menu();
            action_queue_load( gui->load_menu->current_name );
            sprintf( str, tr("Load Game '%s'"), gui->load_menu->current_name );
            engine_confirm_action( str );
            break;
        case ID_LOAD_CANCEL:
            fdlg_hide( gui->load_menu, 1 );
            engine_set_status( STATUS_NONE );
            break;
        /* save */
        case ID_SAVE_OK:
            fdlg_hide( gui->save_menu, 1 );
            action_queue_overwrite( gui->save_menu->current_name );
            if ( strcmp ( gui->save_menu->current_name, tr( "<empty file>" ) ) != 0 )
                engine_confirm_action( tr("Overwrite saved game?") );
            break;
        case ID_SAVE_CANCEL:
            fdlg_hide( gui->save_menu, 1 );
            engine_set_status( STATUS_NONE );
            break;
        case ID_NEW_FOLDER:
            fdlg_hide( gui->save_menu, 1 );
            edit_show( gui->edit, tr( "new folder" ) );
            engine_set_status( STATUS_NEW_FOLDER );
            scroll_block_keys = 1;
            break;
        /* options */
        case ID_C_SOUND:
#ifdef WITH_SOUND            
            config.sound_on = !config.sound_on;
            audio_enable( config.sound_on );
#endif        
            break;
        case ID_C_SOUND_INC:
#ifdef WITH_SOUND            
            config.sound_volume += 16;
            if ( config.sound_volume > 128 )
                config.sound_volume = 128;
            audio_set_volume( config.sound_volume );
#endif        
            break;
        case ID_C_SOUND_DEC:
#ifdef WITH_SOUND            
            config.sound_volume -= 16;
            if ( config.sound_volume < 0 )
                config.sound_volume = 0;
            audio_set_volume( config.sound_volume );
#endif        
            break;
        case ID_C_SUPPLY:
            config.supply = !config.supply;
            break;
        case ID_C_WEATHER:
            config.weather = !config.weather;
            if ( status == STATUS_GAME_MENU ) {
                cur_weather = scen_get_weather();
                draw_map = 1;
            }
            break;
        case ID_C_GRID:
            config.grid = !config.grid;
            draw_map = 1;
            break;
        case ID_C_SHOW_STRENGTH:
            config.show_bar = !config.show_bar;
            draw_map = 1;
            break;
        case ID_C_SHOW_CPU:
            config.show_cpu_turn = !config.show_cpu_turn;
            break;
        case ID_C_VMODE:
            engine_hide_game_menu();
            gui_vmode_dlg_show();
            status = STATUS_VMODE_DLG;
            break;
        /* main menu */
        case ID_MENU:
            x = gui->base_menu->frame->img->bkgnd->surf_rect.x + 30 - 1;
            y = gui->base_menu->frame->img->bkgnd->surf_rect.y;
            if ( y + gui->main_menu->frame->img->img->h >= sdl.screen->h )
                y = sdl.screen->h - gui->main_menu->frame->img->img->h;
            group_move( gui->main_menu, x, y );
            group_hide( gui->main_menu, 0 );
            break;
        case ID_OPTIONS:
            fdlg_hide( gui->load_menu, 1 );
            fdlg_hide( gui->save_menu, 1 );
            x = gui->main_menu->frame->img->bkgnd->surf_rect.x + 30 - 1;
            y = gui->main_menu->frame->img->bkgnd->surf_rect.y;
            if ( y + gui->opt_menu->frame->img->img->h >= sdl.screen->h )
                y = sdl.screen->h - gui->opt_menu->frame->img->img->h;
            group_move( gui->opt_menu, x, y );
            group_hide( gui->opt_menu, 0 );
            break;
        case ID_RESTART:
            engine_hide_game_menu();
            action_queue_restart();
            engine_confirm_action( tr("Do you really want to restart this scenario?") );
            break;
        case ID_SCEN:
            camp_loaded = NO_CAMPAIGN;
            config.use_core_units = 0;
            engine_hide_game_menu();
            sprintf( path, "%s/%s/Scenario", get_gamedir(), config.mod_name );
            fdlg_open( gui->scen_dlg, path, "" );
            group_set_active( gui->scen_dlg->group, ID_SCEN_SETUP, 0 );
            group_set_active( gui->scen_dlg->group, ID_SCEN_OK, 0 );
            group_set_active( gui->scen_dlg->group, ID_SCEN_CANCEL, 1 );
            status = STATUS_RUN_SCEN_DLG;
            break;
        case ID_CAMP:
            camp_loaded = FIRST_SCENARIO;
            engine_hide_game_menu();
            sprintf( path, "%s/%s/Scenario", get_gamedir(), config.mod_name );
            fdlg_open( gui->camp_dlg, path, "" );
            group_set_active( gui->camp_dlg->group, ID_CAMP_SETUP, 0 );
            group_set_active( gui->camp_dlg->group, ID_CAMP_OK, 0 );
            group_set_active( gui->camp_dlg->group, ID_CAMP_CANCEL, 1 );
            status = STATUS_RUN_CAMP_DLG;
            break;
        case ID_SAVE:
            engine_hide_game_menu();
            sprintf( path, "%s/%s/Save", get_gamedir(), config.mod_name );
            fdlg_open( gui->save_menu, path, "" );
            group_set_active( gui->save_menu->group, ID_SAVE_OK, 0 );
            group_set_active( gui->save_menu->group, ID_SAVE_CANCEL, 1 );
            engine_set_status( STATUS_SAVE_MENU );
            break;
        case ID_LOAD:
            engine_hide_game_menu();
            sprintf( path, "%s/%s/Save", get_gamedir(), config.mod_name );
            fdlg_open( gui->load_menu, path, "" );
            group_set_active( gui->load_menu->group, ID_LOAD_OK, 0 );
            group_set_active( gui->load_menu->group, ID_LOAD_CANCEL, 1 );
            engine_set_status( STATUS_LOAD_MENU );
            break;
        case ID_QUIT:
            engine_hide_game_menu();
            action_queue_quit();
            engine_confirm_action( tr("Do you really want to quit?") );
            break;
        case ID_AIR_MODE:
            engine_hide_game_menu();
            air_mode = !air_mode;
            draw_map = 1;
            break;
        case ID_END_TURN:
            engine_hide_game_menu();
            action_queue_end_turn();
            group_set_active( gui->base_menu, ID_END_TURN, 0 );
            engine_confirm_action( tr("Do you really want to end your turn?") );
            break;
        case ID_SCEN_INFO:
            engine_hide_game_menu();
            gui_show_scen_info();
            status = STATUS_SCEN_INFO;
            break;
        case ID_CONDITIONS:
            engine_hide_game_menu();
            gui_show_conds();
            status = STATUS_SCEN_INFO; /* is okay for the engine ;) */
            break;
        case ID_CANCEL:
            /* a confirmation window is run before an action so if cancel
               is hit all actions more recent than top_committed_action
               will be removed. If not given, only the topmost action is
               removed */
            while ( actions_top() != top_committed_action ) {
                action_remove_last();
                if ( !top_committed_action ) break;
            }
            /* fall through */
        case ID_OK:
            engine_set_status( deploy_turn ? STATUS_DEPLOY : STATUS_NONE );
            group_hide( gui->confirm, 1 );
            old_mx = old_my = -1;
            draw_map = 1;
            break;
        case ID_SUPPLY:
            if (cur_unit==0) break;
            action_queue_supply( cur_unit );
            engine_select_unit( cur_unit );
            draw_map = 1;
            engine_hide_unit_menu();
            break;
        case ID_EMBARK_AIR:
            if ( cur_unit->embark == EMBARK_NONE ) {
                action_queue_embark_air( cur_unit, cur_unit->x, cur_unit->y );
                if ( cur_unit->trsp_prop.id )
                    engine_confirm_action( tr("Abandon the ground transporter?") );
                engine_backup_move( cur_unit, -1, -1 );
                engine_hide_unit_menu();
            }
            else
            {
                int drop = 0;
                if (cur_unit->prop.flags&PARACHUTE) 
                {    
                    int x = cur_unit->x, y = cur_unit->y;
                    drop = 1;
                    if (map[x][y].terrain->flags[cur_weather] & SUPPLY_AIR)
                    if (player_is_ally(map[x][y].player,cur_unit->player))
                    if (map[x][y].g_unit==0)
                        drop = 0;
                }
                if (!drop)
                {
                    action_queue_debark_air( cur_unit, cur_unit->x, cur_unit->y, 1 );
                    engine_backup_move( cur_unit, -1, -1 );
                    engine_hide_unit_menu();
                }
                else
                {
                    map_get_dropzone_mask(cur_unit);
                    map_set_fog( F_DEPLOY );
                    engine_hide_unit_menu();
                    status = STATUS_DROP;
                }
            }
            draw_map = 1;
            break;
        case ID_SPLIT:
            if (cur_unit==0) break;
            max = unit_get_split_strength(cur_unit);
            for (i=0;i<10;i++)
                group_set_active(gui->split_menu,ID_SPLIT_1+i,i<max);
            x = gui->unit_buttons->frame->img->bkgnd->surf_rect.x + 30 - 1;
            y = gui->unit_buttons->frame->img->bkgnd->surf_rect.y;
            if ( y + gui->split_menu->frame->img->img->h >= sdl.screen->h )
                y = sdl.screen->h - gui->split_menu->frame->img->img->h;
            group_move( gui->split_menu, x, y );
            group_hide( gui->split_menu, 0 );
            break;
        case ID_SPLIT_1:
        case ID_SPLIT_2:
        case ID_SPLIT_3:
        case ID_SPLIT_4:
        case ID_SPLIT_5:
        case ID_SPLIT_6:
        case ID_SPLIT_7:
        case ID_SPLIT_8:
        case ID_SPLIT_9:
        case ID_SPLIT_10:
            if (cur_unit==0) break;
            cur_split_str = id-ID_SPLIT_1+1;
            map_get_split_units_and_hexes(cur_unit,cur_split_str,split_units,&split_unit_count);
            map_set_fog( F_SPLIT_UNIT );
            engine_hide_unit_menu();
            status = STATUS_SPLIT;
            draw_map = 1;
            break;
        case ID_MERGE:
            if (cur_unit&&merge_unit_count>0)
            {
                map_set_fog( F_MERGE_UNIT );
                engine_hide_unit_menu();
                status = STATUS_MERGE;
                draw_map = 1;
            }
            break;
        case ID_REPLACEMENTS:
            if (cur_unit==0) break;
            engine_hide_unit_menu();
            action_queue_replace(cur_unit);
            draw_map = 1;
            break;
        case ID_ELITE_REPLACEMENTS:
            if (cur_unit==0) break;
            engine_hide_unit_menu();
            action_queue_elite_replace(cur_unit);
            draw_map = 1;
            break;
        case ID_DISBAND:
            if (cur_unit==0) break;
            engine_hide_unit_menu();
            action_queue_disband(cur_unit);
            engine_confirm_action( tr("Do you really want to disband this unit?") );
            break;
        case ID_UNDO:
            if (cur_unit==0) break;
            engine_undo_move( cur_unit );
            engine_select_unit( cur_unit );
            engine_focus( cur_unit->x, cur_unit->y, 0 );
            draw_map = 1;
            engine_hide_unit_menu();
            break;
        case ID_RENAME:
            if (cur_unit==0) break;
            engine_hide_unit_menu();
            status = STATUS_RENAME;
            edit_show( gui->edit, cur_unit->name );
            scroll_block_keys = 1;
            break;
        case ID_PURCHASE:
            engine_hide_game_menu();
            engine_select_unit( 0 );
            gui_show_purchase_window();
            status = STATUS_PURCHASE;
            draw_map = 1;
            break;
        case ID_PURCHASE_EXIT:
            purchase_dlg_hide( gui->purchase_dlg, 1 );
            engine_set_status( STATUS_NONE );
            break;
        case ID_DEPLOY:
            if ( avail_units->count > 0 || deploy_turn )
            {
                engine_hide_game_menu();
                engine_select_unit( 0 );
                gui_show_deploy_window();
                map_get_deploy_mask(cur_player,deploy_unit,deploy_turn);
                map_set_fog( F_DEPLOY );
                if ( deploy_turn && (camp_loaded != NO_CAMPAIGN) && STRCMP(camp_cur_scen->id, camp_first) &&
                    (config.use_core_units) )
                {
                    Unit *entry;
                    list_reset( units );
                    /* searching for core units */
                    while ( ( entry = list_next( units ) ) ) {
                        if (mask[entry->x][entry->y].deploy)
                            entry->core = CORE;
                    }
                }
                status = STATUS_DEPLOY;
                draw_map = 1;
            }
            break;
        case ID_STRAT_MAP:
            engine_hide_game_menu();
            status = STATUS_STRAT_MAP;
            strat_map_update_terrain_layer();
            strat_map_update_unit_layer();
            set_delay( &blink_delay, 500 );
            draw_map = 1;
            break; 
        case ID_SCEN_CANCEL:
            fdlg_hide( gui->scen_dlg, 1 );
            engine_set_status( STATUS_NONE );
            break;
        case ID_SCEN_OK:
            fdlg_hide( gui->scen_dlg, 1 );
            group_set_hidden( gui->unit_buttons, ID_REPLACEMENTS, !config.merge_replacements );
            group_set_hidden( gui->unit_buttons, ID_ELITE_REPLACEMENTS, !config.merge_replacements );
            group_set_hidden( gui->unit_buttons, ID_MERGE, config.merge_replacements );
            group_set_hidden( gui->unit_buttons, ID_SPLIT, config.merge_replacements );
            engine_set_status( STATUS_NONE );
            action_queue_start_scen();
            break;
        case ID_CAMP_CANCEL:
            fdlg_hide( gui->camp_dlg, 1 );
            engine_set_status( STATUS_NONE );
            break;
        case ID_CAMP_OK:
            fdlg_hide( gui->camp_dlg, 1 );
            setup.type = SETUP_CAMP_BRIEFING;
            group_set_hidden( gui->unit_buttons, ID_REPLACEMENTS, !config.merge_replacements );
            group_set_hidden( gui->unit_buttons, ID_ELITE_REPLACEMENTS, !config.merge_replacements );
            group_set_hidden( gui->unit_buttons, ID_MERGE, config.merge_replacements );
            group_set_hidden( gui->unit_buttons, ID_SPLIT, config.merge_replacements );
            engine_set_status( STATUS_NONE );
            action_queue_start_camp();
            break;
        case ID_SCEN_SETUP:
            fdlg_hide( gui->scen_dlg, 1 );
            gui_open_scen_setup();
            status = STATUS_RUN_SCEN_SETUP;
            break;
        case ID_SCEN_SETUP_OK:
            fdlg_hide( gui->scen_dlg, 0 );
            sdlg_hide( gui->scen_setup, 1 );
            status = STATUS_RUN_SCEN_DLG;
            break;
        case ID_SCEN_SETUP_SUPPLY:
            config.supply = !config.supply;
            break;
        case ID_SCEN_SETUP_WEATHER:
            config.weather = !config.weather;
            break;
        case ID_SCEN_SETUP_FOG:
            config.fog_of_war = !config.fog_of_war;
            break;
        case ID_SCEN_SETUP_DEPLOYTURN:
            config.deploy_turn = !config.deploy_turn;
            break;
        case ID_SCEN_SETUP_PURCHASE:
            if (config.purchase < 2)
                config.purchase = config.purchase + 1;
            else
                config.purchase = 0;
            break;
        case ID_SCEN_SETUP_MERGE_REPLACEMENTS:
            config.merge_replacements = !config.merge_replacements;
            break;
        case ID_SCEN_SETUP_CTRL:
            setup.ctrl[gui->scen_setup->sel_id] = !(setup.ctrl[gui->scen_setup->sel_id] - 1) + 1;
            gui_handle_player_select( gui->scen_setup->list->cur_item );
            break;
        case ID_SCEN_SETUP_MODULE:
            sdlg_hide( gui->scen_setup, 1 );
            group_set_active( gui->module_dlg->group, ID_MODULE_OK, 0 );
            group_set_active( gui->module_dlg->group, ID_MODULE_CANCEL, 1 );
            sprintf( path, "%s/Default/AI_modules", get_gamedir() );
            fdlg_open( gui->module_dlg, path, "" );
            status = STATUS_RUN_MODULE_DLG;
            break;
        case ID_CAMP_SETUP:
            fdlg_hide( gui->camp_dlg, 1 );
            gui_open_camp_setup();
            status = STATUS_RUN_CAMP_SETUP;
            break;
        case ID_CAMP_SETUP_OK:
            fdlg_hide( gui->camp_dlg, 0 );
            sdlg_hide( gui->camp_setup, 1 );
            config.purchase = config.campaign_purchase + 1;
            if (config.merge_replacements == OPTION_MERGE)
                group_set_active( gui->unit_buttons, ID_REPLACEMENTS, 0 );
            status = STATUS_RUN_CAMP_DLG;
            break;
        case ID_CAMP_SETUP_MERGE_REPLACEMENTS:
            config.merge_replacements = !config.merge_replacements;
            break;
        case ID_CAMP_SETUP_CORE:
            config.use_core_units = !config.use_core_units;
            break;
        case ID_CAMP_SETUP_PURCHASE:
            config.campaign_purchase = !config.campaign_purchase;
            break;
        case ID_MODULE_OK:
            if ( gui->module_dlg->lbox->cur_item ) {
                if ( gui->module_dlg->subdir[0] != 0 )
                    sprintf( path, "%s/%s", gui->module_dlg->subdir, (char*)gui->module_dlg->lbox->cur_item );
                else
                    sprintf( path, "%s", (char*)gui->module_dlg->lbox->cur_item );
                free( setup.modules[gui->scen_setup->sel_id] );
                setup.modules[gui->scen_setup->sel_id] = strdup( path );
                gui_handle_player_select( gui->scen_setup->list->cur_item );
            }
        case ID_MODULE_CANCEL:
            fdlg_hide( gui->module_dlg, 1 );
            sdlg_hide( gui->scen_setup, 0 );
            status = STATUS_RUN_SCEN_SETUP;
            break;
        case ID_DEPLOY_UP:
            gui_scroll_deploy_up();
            break;
        case ID_DEPLOY_DOWN:
            gui_scroll_deploy_down();
            break;
        case ID_APPLY_DEPLOY:
            /* transfer all units with x != -1 */
            list_reset( avail_units );
            last_act = actions_top();
            while ( ( unit = list_next( avail_units ) ) )
                if ( unit->x != -1 )
                    action_queue_deploy( unit, unit->x, unit->y );
            if ( cur_ctrl == PLAYER_CTRL_HUMAN ) {
                if ( deploy_turn ) {
                    action_queue_end_turn();
                    engine_confirm_actions( last_act, tr("End deployment?") );
                } else {
                    action_queue_set_spot_mask();
                    action_queue_draw_map();
                }
            }
            if ( !deploy_turn ) {
                engine_set_status( STATUS_NONE );
                group_hide( gui->deploy_window, 1 );
            }
            break;
        case ID_CANCEL_DEPLOY:
            list_reset( avail_units );
            while ( ( unit = list_next( avail_units ) ) )
                if ( unit->x != -1 )
                    map_remove_unit( unit );
            draw_map = 1;
            engine_set_status( STATUS_NONE );
            group_hide( gui->deploy_window, 1 );
            map_set_fog( F_SPOT );
            break;
	case ID_VMODE_OK:
		i = select_dlg_get_selected_item_index( gui->vmode_dlg );
		if (i == -1)
			break;
		action_queue_set_vmode( sdl.vmodes[i].width, 
					sdl.vmodes[i].height, 
					sdl.vmodes[i].fullscreen);
		select_dlg_hide( gui->vmode_dlg, 1 );
		engine_set_status( STATUS_NONE );
		break;
	case ID_VMODE_CANCEL:
		select_dlg_hide( gui->vmode_dlg, 1 );
		engine_set_status( STATUS_NONE );
		break;
        case ID_MOD_SELECT:
            engine_hide_game_menu();
            sprintf( path, "%s", get_gamedir() );
            fdlg_open( gui->mod_select_dlg, path, "" );
            fdlg_hide( gui->mod_select_dlg, 0 );
            group_set_active( gui->mod_select_dlg->group, ID_MOD_SELECT_OK, 0 );
            group_set_active( gui->mod_select_dlg->group, ID_MOD_SELECT_CANCEL, 1 );
            engine_set_status( STATUS_MOD_SELECT );
            break;
        case ID_MOD_SELECT_OK:
            fdlg_hide( gui->mod_select_dlg, 1 );
            action_queue_change_mod();
            engine_confirm_action( tr("Loading new game folder will erase current game. Are you sure?") );
            break;
        case ID_MOD_SELECT_CANCEL:
            fdlg_hide( gui->mod_select_dlg, 1 );
            engine_set_status( STATUS_NONE );
            break;
    }
}

/*
====================================================================
Undeploys the given unit. Does not remove it from the map.
====================================================================
*/
static void engine_undeploy_unit( Unit *unit )
{
    int mx = unit->x, my = unit->y;
    unit->x = -1; unit->y = -1;

    /* disembark units if they are embarked */
    if ( map_check_unit_debark( unit, mx, my, EMBARK_AIR, 1 ) )
        map_debark_unit( unit, -1, -1, EMBARK_AIR, 0 );
    if ( map_check_unit_debark( unit, mx, my, EMBARK_SEA, 1 ) )
        map_debark_unit( unit, -1, -1, EMBARK_SEA, 0 );

    gui_add_deploy_unit( unit );
}

/*
====================================================================
Get actions from input events or CPU and queue them.
====================================================================
*/
static void engine_check_events(int *reinit)
{
    int region;
    int hide_edit = 0;
    SDL_Event event;
    Unit *unit;
    int cx, cy, button = 0;
    int mx, my, keypressed = 0;
    SDL_PumpEvents(); /* gather events in the queue */
    if ( sdl_quit ) term_game = 1; /* end game by window manager */
    if ( status == STATUS_MOVE || status == STATUS_ATTACK )
        return;
    if ( cur_ctrl == PLAYER_CTRL_CPU ) {
        if ( actions_count() == 0 )
            (cur_player->ai_run)();
    }
    else {
        if ( event_get_motion( &cx, &cy ) ) {
            if ( setup.type == SETUP_RUN_TITLE )
                ;
            else if (status == STATUS_CAMP_BRIEFING) {
                gui_handle_message_pane_event(camp_pane, cx, cy, BUTTON_NONE, 0);
            } else {
                if ( status == STATUS_DEPLOY || status == STATUS_DEPLOY_INFO ) {
                    gui_handle_deploy_motion( cx, cy, &unit );
                    if ( unit ) {
                        if ( status == STATUS_DEPLOY_INFO )
                            gui_show_full_info( unit );
                        /* display info of this unit */
                        gui_show_quick_info( gui->qinfo1, unit );
                        frame_hide( gui->qinfo2, 1 );
                    }
                }
                if ( engine_get_map_pos( cx, cy, &mx, &my, &region ) ) {
                    /* mouse motion */
                    if ( mx != old_mx || my != old_my || region != old_region ) {
                        old_mx = mx; old_my = my, old_region = region;
                        engine_update_info( mx, my, region );
                    }
                }
            }
            gui_handle_motion( cx, cy );
        }
        if ( event_get_buttondown( &button, &cx, &cy ) ) {
            /* click */
            if ( gui_handle_button( button, cx, cy, &last_button ) ) {
                engine_handle_button( last_button->id );
#ifdef WITH_SOUND
                wav_play( gui->wav_click );
#endif                
            }
            else {
                switch ( status ) {
                    case STATUS_TITLE:
                        /* title menu */
                        if ( button == BUTTON_RIGHT )
                            engine_show_game_menu( cx, cy );
                        break;
                    case STATUS_TITLE_MENU:
                        if ( button == BUTTON_RIGHT )
                            engine_hide_game_menu();
                        break;
                    case STATUS_SAVE_MENU:
                        if ( button == BUTTON_RIGHT )
                            engine_show_game_menu( cx, cy );
                        break;
                    case STATUS_CAMP_BRIEFING:
                        gui_handle_message_pane_event(camp_pane, cx, cy, button, 1);
                        break;
                    default:
                        if ( setup.type == SETUP_RUN_TITLE ) break;
                        /* checking mouse wheel */
                        if ( status == STATUS_NONE ) {
                            if( button == WHEEL_UP ) {
                                scroll_vert = SCROLL_UP;
                                engine_check_scroll( SC_BY_WHEEL );
                            }
                            else if( button == WHEEL_DOWN ) {
                                scroll_vert = SCROLL_DOWN;
                                engine_check_scroll( SC_BY_WHEEL );
                            }
                        }
                        /* select unit from deploy window */
                        if ( status == STATUS_DEPLOY) {
                            if ( gui_handle_deploy_click(button, cx, cy) ) {
                                if ( button == BUTTON_RIGHT ) {
                                    engine_show_deploy_unit_info( deploy_unit );
                                }
                                map_get_deploy_mask(cur_player,deploy_unit,deploy_turn);
                                map_set_fog( F_DEPLOY );
                                draw_map = 1;
                                break;
                            }
                        }
                        /* selection only from map */
                        if ( !engine_get_map_pos( cx, cy, &mx, &my, &region ) ) break;
                        switch ( status ) {
                            case STATUS_STRAT_MAP:
                                /* switch from strat map to tactical map */
                                if ( button == BUTTON_LEFT )
                                    engine_focus( mx, my, 0 );
                                engine_set_status( STATUS_NONE );
                                old_mx = old_my = -1;
                                /* before updating the map, clear the screen */
                                SDL_FillRect( sdl.screen, 0, 0x0 );
                                draw_map = 1;
                                break;
                            case STATUS_GAME_MENU:
                                if ( button == BUTTON_RIGHT )
                                    engine_hide_game_menu();
                                break;
                            case STATUS_DEPLOY_INFO:    
                                engine_hide_deploy_unit_info();
                                break;
                            case STATUS_DEPLOY:
                                /* deploy/undeploy */
                                if ( button == BUTTON_LEFT ) {
                                    /* deploy, but check whether we are explicitly on a un-deployable unit */
                                    if ((unit=map_get_undeploy_unit(mx,my,region==REGION_AIR)))
                                    {}
                                    else if ( deploy_unit && mask[mx][my].deploy ) {
                                        Unit *old_unit;
                                        /* embark to air if embarkable and explicitly on air region in deploy turn */
                                        if ( deploy_turn && 
                                             map_check_unit_embark( deploy_unit, mx, my, EMBARK_AIR, 1 ) && 
                                             (region==REGION_AIR||map[mx][my].g_unit) )
                                            map_embark_unit( deploy_unit, -1, -1, EMBARK_AIR, 0 );
                                        /* embark into sea transport if placed onto ocean */
                                        else if ( map_check_unit_embark( deploy_unit, mx, my, EMBARK_SEA, 1 ) )
                                            map_embark_unit( deploy_unit, -1, -1, EMBARK_SEA, 0 );
                                        deploy_unit->x = mx; deploy_unit->y = my;
                                        deploy_unit->fresh_deploy = 1;
                                        /* swap units if already there */
                                        old_unit = map_swap_unit( deploy_unit );
                                        gui_remove_deploy_unit( deploy_unit );
                                        if ( old_unit ) engine_undeploy_unit( old_unit );
                                        if ( deploy_unit || deploy_turn ) {
                                            map_get_deploy_mask(cur_player,deploy_unit,deploy_turn);
                                            map_set_fog( F_DEPLOY );
                                        }
                                        else {
                                            map_clear_mask( F_DEPLOY );
                                            map_set_fog( F_DEPLOY );
                                        }
                                        engine_update_info(mx,my,region);
                                        draw_map = 1;
                                    }
                                    /* undeploy unit of other region? */
                                    else 
                                        unit=map_get_undeploy_unit(mx,my,region!=REGION_AIR);
                                    if (unit) { /* undeploy? */
                                        map_remove_unit( unit );
                                        engine_undeploy_unit( unit );
                                        map_get_deploy_mask(cur_player,deploy_unit,deploy_turn);
                                        map_set_fog( F_DEPLOY );
                                        engine_update_info(mx,my,region);
                                        draw_map = 1;
                                    }
                                    /* check end_deploy button */
//                                    if (deploy_turn)
//                                        group_set_active(gui->deploy_window,ID_APPLY_DEPLOY,(left_deploy_units->count>0)?0:1);
                                }
                                else if ( button == BUTTON_RIGHT )
                                {
                                    if ( mask[mx][my].spot && ( unit = engine_get_prim_unit( mx, my, region ) ) ) {
                                       /* info */
                                       engine_show_deploy_unit_info( unit );
                                    }
                                }
                                break;
                            case STATUS_DROP:
                                if ( button == BUTTON_RIGHT ) {
                                    /* clear status */
                                    engine_set_status( STATUS_NONE );
                                    gui_set_cursor(CURSOR_STD);
                                    map_set_fog( F_IN_RANGE );
                                    mask[cur_unit->x][cur_unit->y].fog = 0;
                                    draw_map = 1;
                                }
                                else 
                                    if ( button == BUTTON_LEFT )
                                        if ( mask[mx][my].deploy ) {
                                            action_queue_debark_air( cur_unit, mx, my, 0 );
                                            map_set_fog( F_SPOT );
                                            engine_set_status( STATUS_NONE );
                                            gui_set_cursor(CURSOR_STD);
                                            draw_map = 1;
                                        }
                                break;
                            case STATUS_MERGE:
                                if ( button == BUTTON_RIGHT ) {
                                    /* clear status */
                                    engine_set_status( STATUS_NONE );
                                    gui_set_cursor(CURSOR_STD);
                                    map_set_fog( F_IN_RANGE );
                                    mask[cur_unit->x][cur_unit->y].fog = 0;
                                    draw_map = 1;
                                }
                                else 
                                    if ( button == BUTTON_LEFT )
                                        if ( mask[mx][my].merge_unit ) {
                                            action_queue_merge( cur_unit, mask[mx][my].merge_unit );
                                            map_set_fog( F_SPOT );
                                            engine_set_status( STATUS_NONE );
                                            gui_set_cursor(CURSOR_STD);
                                            draw_map = 1;
                                        }
                                break;
                            case STATUS_SPLIT:
                                if ( button == BUTTON_RIGHT ) {
                                    /* clear status */
                                    engine_set_status( STATUS_NONE );
                                    gui_set_cursor(CURSOR_STD);
                                    map_set_fog( F_IN_RANGE );
                                    mask[cur_unit->x][cur_unit->y].fog = 0;
                                    draw_map = 1;
                                }
                                else 
                                    if ( button == BUTTON_LEFT )
                                    {
                                        if ( mask[mx][my].split_unit||mask[mx][my].split_okay ) {
                                            action_queue_split( cur_unit, cur_split_str, mx, my, mask[mx][my].split_unit );
                                            cur_split_str = 0;
                                            map_set_fog( F_SPOT );
                                            engine_set_status( STATUS_NONE );
                                            gui_set_cursor(CURSOR_STD);
                                            draw_map = 1;
                                        } 
                                    }
                                break;
                            case STATUS_UNIT_MENU:
                                if ( button == BUTTON_RIGHT )
                                    engine_hide_unit_menu();
                                break;
                            case STATUS_SCEN_INFO:
                                engine_set_status( STATUS_NONE );
                                frame_hide( gui->sinfo, 1 );
                                old_mx = old_my = -1;
                                break;
                            case STATUS_INFO:
                                engine_set_status( STATUS_NONE );
                                frame_hide( gui->finfo, 1 );
                                old_mx = old_my = -1;
                                break;
                            case STATUS_NONE:
                                switch ( button ) {
                                    case BUTTON_LEFT:
                                        if ( cur_unit ) {
                                            /* handle current unit */
                                            if ( cur_unit->x == mx && cur_unit->y == my && engine_get_prim_unit( mx, my, region ) == cur_unit )
                                                engine_show_unit_menu( cx, cy );
                                            else
                                            if ( ( unit = engine_get_target( mx, my, region ) ) ) {
                                                action_queue_attack( cur_unit, unit );
                                                frame_hide( gui->qinfo1, 1 );
                                                frame_hide( gui->qinfo2, 1 );
                                            }
                                            else
                                                if ( mask[mx][my].in_range && !mask[mx][my].blocked ) {
                                                    action_queue_move( cur_unit, mx, my );
                                                    frame_hide( gui->qinfo1, 1 );
                                                    frame_hide( gui->qinfo2, 1 );
                                                }
                                                else
                                                    if ( mask[mx][my].sea_embark ) {
                                                        if ( cur_unit->embark == EMBARK_NONE )
                                                            action_queue_embark_sea( cur_unit, mx, my );
                                                        else
                                                            action_queue_debark_sea( cur_unit, mx, my );
                                                        engine_backup_move( cur_unit, mx, my );
                                                        draw_map = 1;
                                                    }
                                                    else
                                                        if ( ( unit = engine_get_select_unit( mx, my, region ) ) && cur_unit != unit ) {
                                                            /* first capture the flag for the human unit */
                                                            if ( cur_ctrl == PLAYER_CTRL_HUMAN ) {
                                                                if ( engine_capture_flag( cur_unit ) ) {
                                                                    /* CHECK IF SCENARIO IS FINISHED */
                                                                    if ( scen_check_result( 0 ) )  {
                                                                        engine_finish_scenario();
                                                                        break;
                                                                    }
                                                                }
                                                            }
                                                            engine_select_unit( unit );
                                                            engine_clear_backup();
                                                            engine_update_info( mx, my, region );
                                                            draw_map = 1;
                                                        }
                                        }
                                        else
                                            if ( ( unit = engine_get_select_unit( mx, my, region ) ) && cur_unit != unit ) {
                                                /* select unit */
                                                engine_select_unit( unit );
                                                engine_update_info( mx, my, region );
                                                draw_map = 1;
#ifdef WITH_SOUND
                                                wav_play( terrain_icons->wav_select );
#endif
                                            }
                                        break;
                                    case BUTTON_RIGHT:
                                        if ( cur_unit == 0 ) {
                                            if ( mask[mx][my].spot && ( unit = engine_get_prim_unit( mx, my, region ) ) ) {
                                                /* show unit info */
                                                gui_show_full_info( unit );
                                                status = STATUS_INFO;
                                                gui_set_cursor( CURSOR_STD );
                                            }
                                            else {
                                                /* show menu */
                                                engine_show_game_menu( cx, cy );
                                            }
                                        }
                                        else
                                            if ( cur_unit ) {
                                                /* handle current unit */
                                                if ( cur_unit->x == mx && cur_unit->y == my && engine_get_prim_unit( mx, my, region ) == cur_unit )
                                                    engine_show_unit_menu( cx, cy );
                                                else {
                                                    /* first capture the flag for the human unit */
                                                    if ( cur_ctrl == PLAYER_CTRL_HUMAN ) {
                                                        if ( engine_capture_flag( cur_unit ) ) {
                                                            /* CHECK IF SCENARIO IS FINISHED */
                                                            if ( scen_check_result( 0 ) )  {
                                                                engine_finish_scenario();
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    /* release unit */
                                                    engine_select_unit( 0 );
                                                    engine_update_info( mx, my, region );
                                                    draw_map = 1;
                                                }
                                            }
                                        break;
                                }
                                break;
                        }
                    }
            }
        }
        else
            if ( event_get_buttonup( &button, &cx, &cy ) ) {
                if (status == STATUS_CAMP_BRIEFING) {
                     const char *result;
                     gui_handle_message_pane_event(camp_pane, cx, cy, button, 0);
                     
                     /* check for selection/dismission */
                     result = gui_get_message_pane_selection(camp_pane);
                     if (result && strcmp(result, "nextscen") == 0) {
                         /* start scenario */
                         sprintf( setup.fname, "%s", camp_cur_scen->scen );
                         setup.type = SETUP_DEFAULT_SCEN;
                         end_scen = 1;
                         *reinit = 1;
                     } else if (result) {
                         /* load next campaign entry */
                         setup.type = SETUP_CAMP_BRIEFING;
                         engine_store_debriefing(0);
                         if ( !camp_set_next( result ) )
                             setup.type = SETUP_RUN_TITLE;
                         end_scen = 1;
                         *reinit = 1;
                     }
                     return;
                }
                /* if there was a button pressed released it */
                if ( last_button ) {
                    if ( !last_button->lock ) {
                        last_button->down = 0;
                        if ( last_button->active )
                            last_button->button_rect.x = 0;
                    }
                    if ( last_button->button_rect.x == 0 )
                        if ( button_focus( last_button, cx, cy ) )
                            last_button->button_rect.x = last_button->surf_rect.w;
                    last_button = 0;
                }
            }
        if ( SDL_PollEvent( &event ) )
        {
            if ( event.type == SDL_KEYDOWN ) 
            {
                /* allow mirroring anywhere */
                if (event.key.keysym.sym==SDLK_TAB)
                {
                    gui_mirror_asymm_windows();
                    keypressed = 1;
                }
                if ( status == STATUS_NONE || status == STATUS_INFO || status == STATUS_UNIT_MENU ) {
                    int shiftPressed = event.key.keysym.mod&KMOD_LSHIFT||event.key.keysym.mod&KMOD_RSHIFT;
                    if ( (status != STATUS_UNIT_MENU) && (event.key.keysym.sym == SDLK_n || 
                         (event.key.keysym.sym == SDLK_f&&!shiftPressed) || 
                         (event.key.keysym.sym == SDLK_m&&!shiftPressed) ) ) {
                        int stype = (event.key.keysym.sym==SDLK_n)?0:(event.key.keysym.sym==SDLK_f)?1:2;
                        /* select next unit that has either movement
                           or attack left */
                        list_reset( units );
                        if ( cur_unit != 0 )
                            while ( ( unit = list_next( units ) ) )
                                if ( cur_unit == unit ) 
                                    break;
                        /* get next unit */
                        while ( ( unit = list_next( units ) ) ) {
                            if ( unit->killed ) continue;
                            if ( unit->is_guarding ) continue;
                            if ( unit->player == cur_player )
                                if ( ((stype==0||stype==2)&&unit->cur_mov > 0) || 
                                     ((stype==0||stype==1)&&unit->cur_atk_count > 0) )
                                    break;
                        }
                        if ( unit == 0 ) {
                            /* search again from beginning of list */
                            list_reset( units );
                            while ( ( unit = list_next( units ) ) ) {
                                if ( unit->killed ) continue;
                                if ( unit->is_guarding ) continue;
                                if ( unit->player == cur_player )
                                    if ( ((stype==0||stype==2)&&unit->cur_mov > 0) || 
                                         ((stype==0||stype==1)&&unit->cur_atk_count > 0) )
                                        break;
                            }
                        }
                        if ( unit ) {
                            engine_select_unit( unit );
                            engine_focus( cur_unit->x, cur_unit->y, 0 );
                            draw_map = 1;
                            if ( status == STATUS_INFO )
                                gui_show_full_info( unit );
                            gui_show_quick_info( gui->qinfo1, cur_unit );
                            frame_hide( gui->qinfo2, 1 );
                        }
                        keypressed = 1;
                    } else
                    if ( (status != STATUS_UNIT_MENU) && (event.key.keysym.sym == SDLK_p ||
                         (event.key.keysym.sym == SDLK_f&&shiftPressed) || 
                         (event.key.keysym.sym == SDLK_m&&shiftPressed) ) ) {
                        int stype = (event.key.keysym.sym==SDLK_p)?0:(event.key.keysym.sym==SDLK_f)?1:2;
                        /* select previous unit that has either movement
                           or attack left */
                        list_reset( units );
                        if ( cur_unit != 0 )
                            while ( ( unit = list_next( units ) ) )
                                if ( cur_unit == unit ) 
                                    break;
                        /* get previous unit */
                        while ( ( unit = list_prev( units ) ) ) {
                            if ( unit->killed ) continue;
                            if ( unit->is_guarding ) continue;
                            if ( unit->player == cur_player )
                                if ( ((stype==0||stype==2)&&unit->cur_mov > 0) || 
                                     ((stype==0||stype==1)&&unit->cur_atk_count > 0) )
                                    break;
                        }
                        if ( unit == 0 ) {
                            /* search again from end of list */
                            units->cur_entry = &units->tail;
                            while ( ( unit = list_prev( units ) ) ) {
                                if ( unit->killed ) continue;
                                if ( unit->is_guarding ) continue;
                                if ( unit->player == cur_player )
                                    if ( ((stype==0||stype==2)&&unit->cur_mov > 0) || 
                                         ((stype==0||stype==1)&&unit->cur_atk_count > 0) )
                                        break;
                            }
                        }
                        if ( unit ) {
                            engine_select_unit( unit );
                            engine_focus( cur_unit->x, cur_unit->y, 0 );
                            draw_map = 1;
                            if ( status == STATUS_INFO )
                                gui_show_full_info( unit );
                            gui_show_quick_info( gui->qinfo1, cur_unit );
                            frame_hide( gui->qinfo2, 1 );
                        }
                        keypressed = 1;
                    } else {
                        Uint8 *keystate; int numkeys;
                        keypressed = 1;
                        switch (event.key.keysym.sym)
                        {
                            case SDLK_t: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_AIR_MODE); break;
                            case SDLK_o: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_STRAT_MAP); break;
                            case SDLK_d: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_DEPLOY); break;
                            case SDLK_e: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_END_TURN); break;
                            case SDLK_v: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_C_VMODE); break;
                            case SDLK_g: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_C_GRID); break;
                            case SDLK_PERIOD: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_C_SHOW_STRENGTH); break;
                            case SDLK_i: if (status!=STATUS_UNIT_MENU) engine_handle_button(ID_SCEN_INFO); break;
                                
                            case SDLK_u: engine_handle_button(ID_UNDO); break;
                            case SDLK_s: engine_handle_button(ID_SUPPLY); break;
                            case SDLK_j: engine_handle_button(ID_MERGE); break;
                            case SDLK_1:
                            case SDLK_2:
                            case SDLK_3:
                            case SDLK_4:
                            case SDLK_5:
                            case SDLK_6:
                            case SDLK_7:
                            case SDLK_8:
                            case SDLK_9:
                                keystate = SDL_GetKeyState(&numkeys);
                                if (keystate[SDLK_x])
                                        engine_handle_button(ID_SPLIT_1+event.key.keysym.sym-SDLK_1); 
                                break;
                            case SDLK_MINUS:
                                if (cur_unit) cur_unit->is_guarding = !cur_unit->is_guarding;
                                draw_map = 1;
                                break;
                            default: keypressed = 0; break;
                        }
                    }
                } else if ( status == STATUS_CONF ) {
                    if ( event.key.keysym.sym == SDLK_RETURN )
                        engine_handle_button(ID_OK);
                    else if ( event.key.keysym.sym == SDLK_ESCAPE )
                        engine_handle_button(ID_CANCEL);
                } else if ( status == STATUS_RENAME || status == STATUS_SAVE_EDIT || status == STATUS_NEW_FOLDER ) {
                    if ( event.key.keysym.sym == SDLK_RETURN ) {
                        /* apply */
                        switch ( status ) {
                            case STATUS_RENAME:
                                strcpy_lt( cur_unit->name, gui->edit->text, 20 );
                                hide_edit = 1;
                                break;
                            case STATUS_SAVE_EDIT:
                                if ( dir_check( gui->save_menu->lbox->items, gui->edit->text ) != -1 )
                                    engine_confirm_action( tr("Overwrite saved game?") );
                                slot_save( gui->edit->text, gui->save_menu->subdir );
                                hide_edit = 1;
                                break;
                            case STATUS_NEW_FOLDER:
                                dir_create( gui->edit->text, gui->save_menu->subdir );
                                char path[MAX_PATH];
                                sprintf( path, "%s/%s/Save", config.dir_name, config.mod_name );
                                fdlg_open( gui->save_menu, path, gui->save_menu->subdir );
                                group_set_active( gui->save_menu->group, ID_SAVE_OK, 0 );
                                group_set_active( gui->save_menu->group, ID_SAVE_CANCEL, 1 );
                                group_set_active( gui->save_menu->group, ID_NEW_FOLDER, 1 );
                                hide_edit = 1;
                                break;
                        }
                        keypressed = 1;
                    }
                    else
                        if ( event.key.keysym.sym == SDLK_ESCAPE )
                        {
                            hide_edit = 1;
                            if ( status == STATUS_NEW_FOLDER )
                            {
                                char path[MAX_PATH];
                                sprintf( path, "%s/%s/Save", config.dir_name, config.mod_name );
                                fdlg_open( gui->save_menu, path, gui->save_menu->subdir );
                                group_set_active( gui->save_menu->group, ID_SAVE_OK, 0 );
                                group_set_active( gui->save_menu->group, ID_SAVE_CANCEL, 1 );
                                group_set_active( gui->save_menu->group, ID_NEW_FOLDER, 1 );
                            }
                        }
                    if ( hide_edit ) {    
                        engine_set_status( STATUS_NONE );
                        edit_hide( gui->edit, 1 );
                        old_mx = old_my = -1;
                        scroll_block_keys = 0;
                    }
                    else {
                        edit_handle_key( gui->edit, event.key.keysym.sym, event.key.keysym.mod, event.key.keysym.unicode );
#ifdef WITH_SOUND
                        wav_play( gui->wav_edit );
#endif
                    }
                }
#ifdef WITH_SOUND
                if (keypressed)
                    wav_play( gui->wav_click );
#endif
            }
        }
        /* scrolling */
        engine_check_scroll( SC_NORMAL );
    }
}

/*
====================================================================
Get next combatants assuming that cur_unit attacks cur_target.
Set cur_atk and cur_def and return True if there are any more
combatants.
====================================================================
*/
static int engine_get_next_combatants()
{
    int fight = 0;
    char str[MAX_LINE];
    /* check if there are supporting units; if so initate fight 
       between attacker and these units */
    if ( df_units->count > 0 ) {
        cur_atk = list_first( df_units );
        cur_def = cur_unit;
        fight = 1;
        defFire = 1;
        /* set message if seen */
        if ( !blind_cpu_turn ) {
            if ( cur_atk->sel_prop->flags & ARTILLERY )
                sprintf( str, tr("Defensive Fire") );
            else
                if ( cur_atk->sel_prop->flags & AIR_DEFENSE )
                    sprintf( str, tr("Air-Defense") );
                else
                    sprintf( str, tr("Interceptors") );
            label_write( gui->label_center, gui->font_error, str );
        }
    }
    else {
        /* clear info */
        if ( blind_cpu_turn )
        {
            label_hide( gui->label_left, 1 );
            label_hide( gui->label_center, 1 );
            label_hide( gui->label_right, 1 );
        }
        /* normal attack */
        cur_atk = cur_unit;
        cur_def = cur_target;
        fight = 1;
        defFire = 0;
    }
    return fight;
}

/*
====================================================================
Unit is completely suppressed so check if it
  does nothing
  tries to move to a neighbored tile
  surrenders because it can't move away
====================================================================
*/
enum {
    UNIT_STAYS = 0,
    UNIT_FLEES,
    UNIT_SURRENDERS
};
static void engine_handle_suppr( Unit *unit, int *type, int *x, int *y )
{
    int i, nx, ny;
    *type = UNIT_STAYS;
    if ( unit->sel_prop->mov == 0 ) return;
    /* 80% chance that unit wants to flee */
    if ( DICE(10) <= 8 ) {
        unit->cur_mov = 1;
        map_get_unit_move_mask( unit );
        /* get best close hex. if none: surrender */
        for ( i = 0; i < 6; i++ )
            if ( get_close_hex_pos( unit->x, unit->y, i, &nx, &ny ) )
                if ( mask[nx][ny].in_range && !mask[nx][ny].blocked ) {
                    *type = UNIT_FLEES;
                    *x = nx; *y = ny;
                    return;
                }
        /* surrender! */
        *type = UNIT_SURRENDERS;
    }
}

/*
====================================================================
Check if unit stays on top of an enemy flag and capture
it. Return True if the flag was captured.
====================================================================
*/
static int engine_capture_flag( Unit *unit ) {
    if ( !( unit->sel_prop->flags & FLYING ) )
        if ( !( unit->sel_prop->flags & SWIMMING ) )
            if ( map[unit->x][unit->y].nation != 0 )
                if ( !player_is_ally( map[unit->x][unit->y].player, unit->player ) ) {
                    /* capture */
                    map[unit->x][unit->y].nation = unit->nation;
                    map[unit->x][unit->y].player = unit->player;
                    /* a conquered flag gets deploy ability again after some turns */
                    map[unit->x][unit->y].deploy_center = 3;
                    /* a regular flag gives 40, a victory objective 80, 
                     * no prestige loss for other player */
                    unit->player->cur_prestige += 40;
                    if (map[unit->x][unit->y].obj)
                            unit->player->cur_prestige += 40;
                    return 1;
                }
    return 0;
}
                                
/*
====================================================================
Deqeue the next action and perform it.
====================================================================
*/
static void engine_handle_next_action( int *reinit )
{
    Action *action = 0;
    int enemy_spotted = 0;
    int depth, flags, i, j;
    char name[MAX_NAME];
    /* lock action queue? */
    if ( status == STATUS_CONF || status == STATUS_ATTACK || status == STATUS_MOVE )
        return;
    /* get action */
    if ( ( action = actions_dequeue() ) == 0 ) 
        return;
    /* handle it */
    switch ( action->type ) {
        case ACTION_START_SCEN:
            camp_delete();
            setup.type = SETUP_INIT_SCEN;
            *reinit = 1;
            end_scen = 1;
            break;
        case ACTION_START_CAMP:
            setup.type = SETUP_INIT_CAMP;
            *reinit = 1;
            end_scen = 1;
            break;
        case ACTION_OVERWRITE:
            status = STATUS_SAVE_EDIT;
            if (gui->save_menu->current_name)
            {
                char time[MAX_NAME];
                currentDateTime( time );
                snprintf( name, MAX_NAME, "(%s) %s, turn %i", time, scen_info->name, turn + 1 );
            }
            else
            {
                snprintf( name, MAX_NAME, "%s", gui->save_menu->current_name );
            }
            edit_show( gui->edit, name );
            scroll_block_keys = 1;
            break;
        case ACTION_LOAD:
            setup.type = SETUP_LOAD_GAME;
            strcpy( setup.fname, gui->load_menu->current_name );
            *reinit = 1;
            end_scen = 1;
            break;
        case ACTION_RESTART:
            strcpy( setup.fname, scen_info->fname );
            setup.type = SETUP_INIT_SCEN;
            *reinit = 1;
            end_scen = 1;
            if (config.use_core_units && (camp_loaded != NO_CAMPAIGN) && !STRCMP(camp_cur_scen->id, camp_first))
                camp_loaded = RESTART_CAMPAIGN;
            break;
        case ACTION_QUIT:
            engine_set_status( STATUS_NONE );
            end_scen = 1;
            break;
        case ACTION_CHANGE_MOD:
            snprintf( config.mod_name, MAX_NAME, "%s", gui->mod_select_dlg->current_name );
            frame_hide( gui->qinfo1, 1 );
            frame_hide( gui->qinfo2, 1 );
            setup.type = SETUP_RUN_TITLE;
            engine_set_status( STATUS_TITLE );
            break;
        case ACTION_SET_VMODE:
            flags = SDL_SWSURFACE;
            if ( action->full ) flags |= SDL_FULLSCREEN;
            depth = SDL_VideoModeOK( action->w, action->h, 32, flags );
            if ( depth == 0 ) {
                fprintf( stderr, tr("Video Mode: %ix%i, Fullscreen: %i not available\n"), 
                         action->w, action->h, action->full );
            }
            else {
                /* videomode */
                SDL_SetVideoMode( action->w, action->h, depth, flags );
                printf(tr("Applied video mode %dx%dx%d %s\n"),
                                        action->w, action->h, depth,
                                        (flags&SDL_FULLSCREEN)?tr("Fullscreen"):
                                            tr("Window"));
                /* adjust windows */
                gui_adjust();
                if ( setup.type != SETUP_RUN_TITLE ) {
                    /* reset engine's map size (number of tiles on screen) */
                    for ( i = map_sx, map_sw = 0; i < gui->map_frame->w; i += hex_x_offset )
                        map_sw++;
                    for ( j = map_sy, map_sh = 0; j < gui->map_frame->h; j += hex_h )
                        map_sh++;
                    /* reset map pos if nescessary */
                    if ( map_x + map_sw >= map_w )
                        map_x = map_w - map_sw;
                    if ( map_y + map_sh >= map_h )
                        map_y = map_h - map_sh;
                    if ( map_x < 0 ) map_x = 0;
                    if ( map_y < 0 ) map_y = 0;
                    /* recreate strategic map */
                    strat_map_delete();
                    strat_map_create();
                    /* recreate screen buffer */
                    free_surf( &sc_buffer );
                    sc_buffer = create_surf( sdl.screen->w, sdl.screen->h, SDL_SWSURFACE );
                }
                /* redraw map */
                draw_map = 1;
                /* set config */
                config.width = action->w;
                config.height = action->h;
                config.fullscreen = flags & SDL_FULLSCREEN;
            }
            break;
        case ACTION_SET_SPOT_MASK:
            map_set_spot_mask();
            map_set_fog( F_SPOT );
            break;
        case ACTION_DRAW_MAP:
            draw_map = 1;
            break;
        case ACTION_DEPLOY:
            action->unit->delay = 0;
            action->unit->fresh_deploy = 0;
            action->unit->x = action->x;
            action->unit->y = action->y;
            list_transfer( avail_units, units, action->unit );
            if ( cur_ctrl == PLAYER_CTRL_CPU ) /* for human player it is already inserted */
                map_insert_unit( action->unit );
            scen_prep_unit( action->unit, SCEN_PREP_UNIT_DEPLOY );
            break;
        case ACTION_EMBARK_SEA:
            if ( map_check_unit_embark( action->unit, action->x, action->y, EMBARK_SEA, 0 ) ) {
                map_embark_unit( action->unit, action->x, action->y, EMBARK_SEA, &enemy_spotted );
                if ( enemy_spotted ) engine_clear_backup();
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    engine_select_unit( action->unit );
            }
            break;
        case ACTION_DEBARK_SEA:
            if ( map_check_unit_debark( action->unit, action->x, action->y, EMBARK_SEA, 0 ) ) {
                map_debark_unit( action->unit, action->x, action->y, EMBARK_SEA, &enemy_spotted );
                if ( enemy_spotted ) engine_clear_backup();
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    engine_select_unit( action->unit );
                /* CHECK IF SCENARIO IS FINISHED (by capturing last flag) */
                if ( scen_check_result( 0 ) )  {
                    engine_finish_scenario();
                    break;
                }
            }
            break;
        case ACTION_MERGE:
            if ( unit_check_merge( action->unit, action->target ) ) {
                unit_merge( action->unit, action->target );
                engine_remove_unit( action->target );
                map_get_vis_units();
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    engine_select_unit( action->unit );
            }
            break;
        case ACTION_SPLIT:
            if ( map_check_unit_split(action->unit,action->str,action->x,action->y,action->target) )
            {
                if (action->target)
                {
                    /* do reverse merge */
                    int old_str = action->unit->str;
                    action->unit->str = action->str;
                    unit_merge(action->target,action->unit);
                    action->unit->str = old_str-action->str;
                    unit_set_as_used(action->unit);
                    unit_update_bar(action->unit);
                }
                else
                {
                    /* add new unit */
                    int dummy;
                    Unit *newUnit = unit_duplicate( action->unit, 1 );
                    newUnit->str = action->str;
                    newUnit->x = action->x; newUnit->y = action->y;
                    newUnit->terrain = map[newUnit->x][newUnit->y].terrain;
                    action->unit->str -= action->str;
                    list_add(units,newUnit); map_insert_unit(newUnit);
                    unit_set_as_used(action->unit); unit_set_as_used(newUnit);
                    unit_update_bar(action->unit); unit_update_bar(newUnit);
                    map_update_spot_mask(newUnit,&dummy); map_set_fog(F_SPOT);
                }
            }
            break;
        case ACTION_REPLACE:
            if ( unit_check_replacements( action->unit,REPLACEMENTS ) ) {
                unit_replace( action->unit,REPLACEMENTS );
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    engine_select_unit( action->unit );
            }
            break;
        case ACTION_ELITE_REPLACE:
            if ( unit_check_replacements( action->unit,ELITE_REPLACEMENTS ) ) {
                unit_replace( action->unit,ELITE_REPLACEMENTS );
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    engine_select_unit( action->unit );
            }
            break;
        case ACTION_DISBAND:
            engine_remove_unit( action->unit );
            map_get_vis_units();
            if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                engine_select_unit(0);
            break;
        case ACTION_EMBARK_AIR:
            if ( map_check_unit_embark( action->unit, action->x, action->y, EMBARK_AIR, 0 ) ) {
                map_embark_unit( action->unit, action->x, action->y, EMBARK_AIR, &enemy_spotted );
                if ( enemy_spotted ) engine_clear_backup();
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    engine_select_unit( action->unit );
            }
            break;
        case ACTION_DEBARK_AIR:
            if ( map_check_unit_debark( action->unit, action->x, action->y, EMBARK_AIR, 0 ) ) {
                if (action->id==1) /* normal landing */
                    map_debark_unit( action->unit, action->x, action->y, EMBARK_AIR, &enemy_spotted );
                else
                    map_debark_unit( action->unit, action->x, action->y, DROP_BY_PARACHUTE, &enemy_spotted );
                if ( enemy_spotted ) engine_clear_backup();
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    engine_select_unit( action->unit );
                /* CHECK IF SCENARIO IS FINISHED (by capturing last flag) */
                if ( scen_check_result( 0 ) )  {
                    engine_finish_scenario();
                    break;
                }
            }
            break;
        case ACTION_SUPPLY:
            if ( unit_check_supply( action->unit, UNIT_SUPPLY_ANYTHING, 0, 0 ) )
                unit_supply( action->unit, UNIT_SUPPLY_ALL );
            break;
        case ACTION_END_TURN:
            engine_end_turn();
            if (!end_scen)
                engine_begin_turn( 0, 0 );
            break;
        case ACTION_MOVE:
            cur_unit = action->unit;
            move_unit = action->unit;
            if ( move_unit->cur_mov == 0 ) {
                fprintf( stderr, tr("%s has no move points remaining\n"), move_unit->name );
                break;
            }
            dest_x = action->x;
            dest_y = action->y;
            status = STATUS_MOVE;
            phase = PHASE_INIT_MOVE;
            engine_clear_danger_mask();
            if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                image_hide( gui->cursors, 1 );
            break;
        case ACTION_ATTACK:
            if ( !unit_check_attack( action->unit, action->target, UNIT_ACTIVE_ATTACK ) ) {
                fprintf( stderr, tr("'%s' (%i,%i) can not attack '%s' (%i,%i)\n"), 
                         action->unit->name, action->unit->x, action->unit->y,
                         action->target->name, action->target->x, action->target->y );
                break;
            }
            if ( !mask[action->target->x][action->target->y].spot ) {
                fprintf( stderr, tr("'%s' may not attack unit '%s' (not visible)\n"), action->unit->name, action->target->name );
                break;
            }
            cur_unit = action->unit;
            cur_target = action->target;
            unit_get_df_units( cur_unit, cur_target, units, df_units );
            if ( engine_get_next_combatants() ) {
                status = STATUS_ATTACK;
                phase = PHASE_INIT_ATK;
                engine_clear_danger_mask();
                if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                    image_hide( gui->cursors, 1 );
            }
            break;
    }
    free( action );
}

/*
====================================================================
Update the engine if an action is currently handled.
====================================================================
*/
static void engine_update( int ms )
{
    int cx, cy;
    int old_atk_str, old_def_str;
    int old_atk_suppr, old_def_suppr;
    int old_atk_turn_suppr, old_def_turn_suppr;
    int reset = 0;
    int broken_up = 0;
    int was_final_fight = 0;
    int type = UNIT_STAYS, dx, dy;
    int start_x, start_y, end_x, end_y;
    float len;
    int i;
    int enemy_spotted = 0;
    int surrender = 0;
    /* check status and phase */
    switch ( status ) {
        case STATUS_MOVE:
            switch ( phase ) {
                case PHASE_INIT_MOVE:
                    /* get move mask */
                    map_get_unit_move_mask( move_unit );
                    /* check if tile is in reach */
                    if ( mask[dest_x][dest_y].in_range == 0 ) {
                        fprintf( stderr, tr("%i,%i out of reach for '%s'\n"), dest_x, dest_y, move_unit->name );
                        phase = PHASE_END_MOVE;
                        break;
                    }
                    if ( mask[dest_x][dest_y].blocked ) {
                        fprintf( stderr, tr("%i,%i is blocked ('%s' wants to move there)\n"), dest_x, dest_y, move_unit->name );
                        phase = PHASE_END_MOVE;
                        break;
                    }
                    way = map_get_unit_way_points( move_unit, dest_x, dest_y, &way_length, &surp_unit );
                    if ( way == 0 ) {
                        fprintf( stderr, tr("There is no way for unit '%s' to move to %i,%i\n"), 
                                 move_unit->name, dest_x, dest_y );
                        phase = PHASE_END_MOVE;
                        break;
                    }
                    /* remove unit influence */
                    if ( !player_is_ally( move_unit->player, cur_player ) ) 
                        map_remove_unit_infl( move_unit );
                    /* backup the unit but only if this is not a fleeing unit! */
                    if ( fleeing_unit )
                        fleeing_unit = 0;
                    else
                        engine_backup_move( move_unit, dest_x, dest_y );
                    /* if ground transporter needed mount unit */
                    if ( mask[dest_x][dest_y].mount ) unit_mount( move_unit );
                    /* start at first way point */
                    way_pos = 0;
                    /* unit's used */
                    move_unit->unused = 0;
                    /* artillery looses ability to attack */
                    if ( move_unit->sel_prop->flags & ATTACK_FIRST )
                        move_unit->cur_atk_count = 0;
                    /* decrease moving points */
/*                    if ( ( move_unit->sel_prop->flags & RECON ) && surp_unit == 0 )
                        move_unit->cur_mov = mask[dest_x][dest_y].in_range - 1;
                    else*/
                        move_unit->cur_mov = 0;
                    if ( move_unit->cur_mov < 0 ) 
                        move_unit->cur_mov = 0;
                    /* no entrenchment */
                    move_unit->entr = 0;
                    /* build up the image */
                    if ( !blind_cpu_turn ) {
                        move_image = image_create( create_surf( move_unit->sel_prop->icon->w, 
                                                                move_unit->sel_prop->icon->h, SDL_SWSURFACE ),
                                                   move_unit->sel_prop->icon_w, move_unit->sel_prop->icon_h,
                                                   sdl.screen, 0, 0 ); 
                        image_set_region( move_image, move_unit->icon_offset, 0, 
                                          move_unit->sel_prop->icon_w, move_unit->sel_prop->icon_h );
                        SDL_FillRect( move_image->img, 0, move_unit->sel_prop->icon->format->colorkey );
                        SDL_SetColorKey( move_image->img, SDL_SRCCOLORKEY, move_unit->sel_prop->icon->format->colorkey );
                        FULL_DEST( move_image->img );
                        FULL_SOURCE( move_unit->sel_prop->icon );
                        blit_surf();
                        if ( mask[move_unit->x][move_unit->y].fog )
                            image_hide( move_image, 1 );
                    }
                    /* remove unit from map */
                    map_remove_unit( move_unit );
                    if ( !blind_cpu_turn ) {
                        engine_get_screen_pos( move_unit->x, move_unit->y, &start_x, &start_y );
                        start_x += ( ( hex_w - move_unit->sel_prop->icon_w ) >> 1 );
                        start_y += ( ( hex_h - move_unit->sel_prop->icon_h ) >> 1 );
                        image_move( move_image, start_x, start_y );
                        draw_map = 1;
                    }
                    /* animate */
                    phase = PHASE_START_SINGLE_MOVE;
                    /* play sound */
#ifdef WITH_SOUND   
                    if ( !mask[move_unit->x][move_unit->y].fog )
                        wav_play( move_unit->sel_prop->wav_move );
#endif
                    /* since it moves it is no longer assumed to be a guard */
                    move_unit->is_guarding = 0;
                    break;
                case PHASE_START_SINGLE_MOVE:
                    /* get next start way point */
                    if ( blind_cpu_turn ) {
                        way_pos = way_length - 1;
                        /* quick move unit */
                        for ( i = 1; i < way_length; i++ ) {
                            move_unit->x = way[i].x; move_unit->y = way[i].y;
                            map_update_spot_mask( move_unit, &enemy_spotted );
                        }
                    }
                    else
                        if ( !modify_fog ) {
                            i = way_pos;
                            while ( i + 1 < way_length && mask[way[i].x][way[i].y].fog && mask[way[i + 1].x][way[i + 1].y].fog ) {
                                i++;
                                /* quick move unit */
                                move_unit->x = way[i].x; move_unit->y = way[i].y;
                                map_update_spot_mask( move_unit, &enemy_spotted );
                            }
                            way_pos = i;
                        }
                    /* focus current way point */
                    if ( way_pos < way_length - 1 )
                        if ( !blind_cpu_turn && ( MAP_CHECK_VIS( way[way_pos].x, way[way_pos].y ) || MAP_CHECK_VIS( way[way_pos + 1].x, way[way_pos + 1].y ) ) ) {
                            if ( engine_focus( way[way_pos].x, way[way_pos].y, 1 ) ) {
                                engine_get_screen_pos( way[way_pos].x, way[way_pos].y, &start_x, &start_y );
                                start_x += ( ( hex_w - move_unit->sel_prop->icon_w ) >> 1 );
                                start_y += ( ( hex_h - move_unit->sel_prop->icon_h ) >> 1 );
                                image_move( move_image, start_x, start_y );
                            }
                        }
                    /* units looking direction */
                    if ( way_pos + 1 < way_length )
                        unit_adjust_orient( move_unit, way[way_pos + 1].x, way[way_pos + 1].y );
                    if ( !blind_cpu_turn )
                        image_set_region( move_image, move_unit->icon_offset, 0, 
                                          move_unit->sel_prop->icon_w, move_unit->sel_prop->icon_h );
                    /* units position */
                    move_unit->x = way[way_pos].x; move_unit->y = way[way_pos].y;
                    /* update spotting */
                    map_update_spot_mask( move_unit, &enemy_spotted );
                    if ( modify_fog ) map_set_fog( F_SPOT );
                    if ( enemy_spotted ) {
                        /* if you spotted an enemy it's not allowed to undo the turn */
                        engine_clear_backup();
                    }
                    /* determine next step */
                    if ( way_pos == way_length - 1 )
                        phase = PHASE_CHECK_LAST_MOVE;
                    else {
                        /* animate? */
                        if ( MAP_CHECK_VIS( way[way_pos].x, way[way_pos].y ) || MAP_CHECK_VIS( way[way_pos + 1].x, way[way_pos + 1].y ) ) {
                            engine_get_screen_pos( way[way_pos].x, way[way_pos].y, &start_x, &start_y );
                            start_x += ( ( hex_w - move_unit->sel_prop->icon_w ) >> 1 );
                            start_y += ( ( hex_h - move_unit->sel_prop->icon_h ) >> 1 );
                            engine_get_screen_pos( way[way_pos + 1].x, way[way_pos + 1].y, &end_x, &end_y );
                            end_x += ( ( hex_w - move_unit->sel_prop->icon_w ) >> 1 );
                            end_y += ( ( hex_h - move_unit->sel_prop->icon_h ) >> 1 );
                            unit_vector.x = start_x; unit_vector.y = start_y;
                            move_vector.x = end_x - start_x; move_vector.y = end_y - start_y;
                            len = sqrt( move_vector.x * move_vector.x + move_vector.y * move_vector.y );
                            move_vector.x /= len; move_vector.y /= len;
                            image_move( move_image, (int)unit_vector.x, (int)unit_vector.y );
                            set_delay( &move_time, ((int)( len / move_vel ))/config.anim_speed );
                        }
                        else
                            set_delay( &move_time, 0 );
                        phase = PHASE_RUN_SINGLE_MOVE;
                        image_hide( move_image, 0 );
                    }
                    break;
                case PHASE_RUN_SINGLE_MOVE:
                    if ( timed_out( &move_time, ms ) ) {
                        /* next way point */
                        way_pos++;
                        /* next movement */
                        phase = PHASE_START_SINGLE_MOVE;
                    }
                    else {
                        unit_vector.x += move_vector.x * move_vel * ms;
                        unit_vector.y += move_vector.y * move_vel * ms;
                        image_move( move_image, (int)unit_vector.x, (int)unit_vector.y );
                    }
                    break;
                case PHASE_CHECK_LAST_MOVE:
                    /* insert unit */
                    map_insert_unit( move_unit );
                    /* capture flag if there is one */
                    /* NOTE: only do it for AI. For the human player, it will
                     * be done on deselecting the current unit to resemble
                     * original Panzer General behaviour
                     */
                    if ( cur_ctrl == PLAYER_CTRL_CPU ) {
                        if ( engine_capture_flag( move_unit ) ) {
                            /* CHECK IF SCENARIO IS FINISHED */
                            if ( scen_check_result( 0 ) )  {
                                engine_finish_scenario();
                                break;
                            }
                        }
                    }
                    /* add influence */
                    if ( !player_is_ally( move_unit->player, cur_player ) )
                        map_add_unit_infl( move_unit );
                    /* update the visible units list */
                    map_get_vis_units();
                    map_set_vis_infl_mask();
                    /* next phase */
                    phase = PHASE_END_MOVE;
                    break;
                case PHASE_END_MOVE:
                    /* fade out sound */
#ifdef WITH_SOUND         
                    audio_fade_out( 0, 500 ); /* move sound channel */
#endif
                    /* decrease fuel for way_pos hex tiles of movement */
                    if ( unit_check_fuel_usage( move_unit ) && config.supply ) {
                        move_unit->cur_fuel -= unit_calc_fuel_usage( move_unit, way_pos );
                        if ( move_unit->cur_fuel < 0 )
                            move_unit->cur_fuel = 0;
                    }
                    /* clear move buffer image */
                    if ( !blind_cpu_turn )
                        image_delete( &move_image );
                    /* run surprise contact */
                    if ( surp_unit ) {
                        cur_unit = move_unit;
                        cur_target = surp_unit;
                        surp_contact = 1;
                        surp_unit = 0;
                        if ( engine_get_next_combatants() ) {
                            status = STATUS_ATTACK;
                            phase = PHASE_INIT_ATK;
                            if ( !blind_cpu_turn ) {
                                image_hide( gui->cursors, 1 );
                                draw_map = 1;
                            }
                        }
                        break;
                    }
                    /* reselect unit -- cur_unit may differ from move_unit! */
                    if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                        engine_select_unit( cur_unit );
                    /* status */
                    engine_set_status( STATUS_NONE );
                    phase = PHASE_NONE;
                    /* allow new human/cpu input */
                    if ( !blind_cpu_turn ) {
                        if ( cur_ctrl == PLAYER_CTRL_HUMAN ) {
                            gui_set_cursor( CURSOR_STD );
                            image_hide( gui->cursors, 0 );
                            old_mx = old_my = -1;
                        }
                        draw_map = 1;
                    }
                    break;
            }
            break;
        case STATUS_ATTACK:
            switch ( phase ) {
                case PHASE_INIT_ATK:
#ifdef DEBUG_ATTACK
                    printf( "\n" );
#endif
                    label_write( gui->label_left, gui->font_std, "" );
                    label_write( gui->label_center, gui->font_std, "" );
                    label_write( gui->label_right, gui->font_std, "" );
                    if ( !blind_cpu_turn ) {
                        if ( MAP_CHECK_VIS( cur_atk->x, cur_atk->y ) ) {
                            /* show attacker cross */
                            engine_focus( cur_atk->x, cur_atk->y, 1 );
                            engine_get_screen_pos( cur_atk->x, cur_atk->y, &cx, &cy );
                            anim_move( terrain_icons->cross, cx, cy );
                            anim_play( terrain_icons->cross, 0 );
                        }
                        phase = PHASE_SHOW_ATK_CROSS;
                    }
                    else
                        phase = PHASE_COMBAT;
                    /* both units in a fight are no longer just guarding */
                    cur_atk->is_guarding = 0;
                    cur_def->is_guarding = 0;
                    break;
                case PHASE_SHOW_ATK_CROSS:
                    if ( !terrain_icons->cross->playing ) {
                        if ( MAP_CHECK_VIS( cur_def->x, cur_def->y ) ) {
                            /* show defender cross */
                            engine_focus( cur_def->x, cur_def->y, 1 );
                            engine_get_screen_pos( cur_def->x, cur_def->y, &cx, &cy );
                            anim_move( terrain_icons->cross, cx, cy );
                            anim_play( terrain_icons->cross, 0 );
                        }
                        phase = PHASE_SHOW_DEF_CROSS;
                    }
                    break;
                case PHASE_SHOW_DEF_CROSS:
                    if ( !terrain_icons->cross->playing )
                        phase = PHASE_COMBAT;
                    break;
                case PHASE_COMBAT:
                    /* backup old strength to see who needs and explosion */
                    old_atk_str = cur_atk->str; old_def_str = cur_def->str;
                    /* backup old suppression to calculate delta */
                    old_atk_suppr = cur_atk->suppr; old_def_suppr = cur_def->suppr;
                    old_atk_turn_suppr = cur_atk->turn_suppr;
                    old_def_turn_suppr = cur_def->turn_suppr;
                    /* take damage */
                    if ( surp_contact )
                        atk_result = unit_surprise_attack( cur_atk, cur_def );
                    else {
                        if ( df_units->count > 0 )
                            atk_result = unit_normal_attack( cur_atk, cur_def, UNIT_DEFENSIVE_ATTACK );
                        else
                            atk_result = unit_normal_attack( cur_atk, cur_def, UNIT_ACTIVE_ATTACK );
                    }
                    /* calculate deltas */
                    atk_damage_delta = old_atk_str - cur_atk->str;
		            def_damage_delta = old_def_str - cur_def->str;
                    atk_suppr_delta = cur_atk->suppr - old_atk_suppr;
		            def_suppr_delta = cur_def->suppr - old_def_suppr;
                    atk_suppr_delta += cur_atk->turn_suppr - old_atk_turn_suppr;
                    def_suppr_delta += cur_def->turn_suppr - old_def_turn_suppr;
                    if ( blind_cpu_turn )
                        phase = PHASE_CHECK_RESULT;
                    else {
                        /* if rugged defense add a pause */
                        if ( atk_result & AR_RUGGED_DEFENSE ) {
                            phase = PHASE_RUGGED_DEF;
                            if ( cur_def->sel_prop->flags & FLYING )
                                label_write( gui->label_center, gui->font_error, tr("Out Of The Sun!") );
                            else
                                if ( cur_def->sel_prop->flags & SWIMMING )
                                    label_write( gui->label_center, gui->font_error, tr("Surprise Contact!") );
                                else
                                    label_write( gui->label_center, gui->font_error, tr("Rugged Defense!") );
                            reset_delay( &msg_delay );
                        }
                        else if (atk_result & AR_EVADED) {
                            /* if sub evaded give info */
                            label_write( gui->label_center, gui->font_error, tr("Submarine evades!") );
                            reset_delay( &msg_delay );
                            phase = PHASE_EVASION;
                        }
                        else 
                            phase = PHASE_PREP_EXPLOSIONS;
                    }
                    break;
                case PHASE_EVASION:
                    if ( timed_out( &msg_delay, ms ) ) {
                        phase = PHASE_PREP_EXPLOSIONS;
                    }
                    break;
                case PHASE_RUGGED_DEF:
                    if ( timed_out( &msg_delay, ms ) ) {
                        phase = PHASE_PREP_EXPLOSIONS;
                    }
                    break;
                case PHASE_PREP_EXPLOSIONS:
                    engine_focus( cur_def->x, cur_def->y, 1 );
                    if (defFire)
                        gui_show_actual_losses( cur_def, cur_atk, def_suppr_delta, def_damage_delta,
                                                atk_suppr_delta, atk_damage_delta );
                    else
                    {
                        if (cur_atk->sel_prop->flags & TURN_SUPPR)
                            gui_show_actual_losses( cur_atk, cur_def, atk_suppr_delta, atk_damage_delta,
                                                    def_suppr_delta, def_damage_delta );
                        else
                            gui_show_actual_losses( cur_atk, cur_def, 0, atk_damage_delta,
                                                    0, def_damage_delta );
                    }
                    /* attacker epxlosion */
                    if ( atk_damage_delta ) {
                        engine_get_screen_pos( cur_atk->x, cur_atk->y, &cx, &cy );
			            if (!cur_atk->str) map_remove_unit( cur_atk );
                        map_draw_tile( gui->map_frame, cur_atk->x, cur_atk->y, cx, cy, !air_mode, 0 );
                        anim_move( terrain_icons->expl1, cx, cy );
                        anim_play( terrain_icons->expl1, 0 );
                    }
                    /* defender explosion */
                    if ( def_damage_delta ) {
                        engine_get_screen_pos( cur_def->x, cur_def->y, &cx, &cy );
                        if (!cur_def->str) map_remove_unit( cur_def );
			            map_draw_tile( gui->map_frame, cur_def->x, cur_def->y, cx, cy, !air_mode, 0 );
                        anim_move( terrain_icons->expl2, cx, cy );
                        anim_play( terrain_icons->expl2, 0 );
                    }
                    phase = PHASE_SHOW_EXPLOSIONS;
                    /* play sound */
#ifdef WITH_SOUND                    
                    if ( def_damage_delta || atk_damage_delta )
                        wav_play( terrain_icons->wav_expl );
#endif
                    break;
                case PHASE_SHOW_EXPLOSIONS:
		            if ( !terrain_icons->expl1->playing && !terrain_icons->expl2->playing ) {
                        phase = PHASE_FIGHT_MSG;
                        reset_delay( &msg_delay );
			            anim_hide( terrain_icons->expl1, 1 );
			            anim_hide( terrain_icons->expl2, 1 );
		            }
                    break;
		        case PHASE_FIGHT_MSG:
                    if ( timed_out( &msg_delay, ms ) ) {
                        phase = PHASE_CHECK_RESULT;
                    }
                    break;
                case PHASE_CHECK_RESULT:
                    surp_contact = 0;
                    /* check attack result */
                    if ( atk_result & AR_UNIT_KILLED ) {
                        scen_inc_casualties_for_unit( cur_atk );
                        engine_remove_unit( cur_atk );
                        cur_atk = 0;
                    }
                    if ( atk_result & AR_TARGET_KILLED ) {
                        scen_inc_casualties_for_unit( cur_def );
                        engine_remove_unit( cur_def );
                        cur_def = 0;
                    }
                    /* CHECK IF SCENARIO IS FINISHED DUE TO UNITS_KILLED OR UNITS_SAVED */
                    if ( scen_check_result( 0 ) )  {
                        engine_finish_scenario();
                        break;
                    }
                    reset = 1;
                    if ( df_units->count > 0 ) {
                        if ( atk_result & AR_TARGET_SUPPRESSED || atk_result & AR_TARGET_KILLED ) {
                            list_clear( df_units );
                            if ( atk_result & AR_TARGET_KILLED )
                                cur_unit = 0;
                            else {
                                /* supressed unit looses its actions */
                                cur_unit->cur_mov = 0;
                                cur_unit->cur_atk_count = 0;
                                cur_unit->unused = 0;
                                broken_up = 1;
                            }
                        }
                        else {
                            reset = 0;
                            list_delete_pos( df_units, 0 );
                        }
                    }
                    else
                        was_final_fight = 1;
                    if ( !reset ) {
                        /* continue fights */
                        if ( engine_get_next_combatants() ) {
                            status = STATUS_ATTACK;
                            phase = PHASE_INIT_ATK;
                        }
                        else
                            fprintf( stderr, tr("Deadlock! No remaining combatants but supposed to continue fighting? How is this supposed to work????\n") );
                    }
                    else {
                        /* clear suppression from defensive fire */
                        if ( cur_atk ) {
                            cur_atk->suppr = 0;
                            cur_atk->unused = 0;
                        }
                        if ( cur_def )
                            cur_def->suppr = 0;
                        /* if this was the final fight between selected unit and selected target
                           check if one of these units was completely suppressed and surrenders
                           or flees */
                        if ( was_final_fight ) {
                            engine_clear_backup(); /* no undo allowed after attack */
                            if ( cur_atk != 0 && cur_def != 0 ) {
                                if ( atk_result & AR_UNIT_ATTACK_BROKEN_UP ) {
                                    /* unit broke up the attack */
                                    broken_up = 1;
                                }
                                else
                                /* total suppression may only cause fleeing or 
                                   surrender if: both units are ground/naval units in
                                   close combat: the unit that causes suppr must
                                   have range 0 (melee)
                                   inf -> fort (fort may surrender)
                                   fort -> adjacent inf (inf will not flee) */
                                if (!(cur_atk->sel_prop->flags&FLYING)&&
                                    !(cur_def->sel_prop->flags&FLYING))
                                {
                                    if ( atk_result & AR_UNIT_SUPPRESSED && 
                                         !(atk_result & AR_TARGET_SUPPRESSED) &&
                                         cur_def->sel_prop->rng==0 ) {
                                        /* cur_unit is suppressed */
                                        engine_handle_suppr( cur_atk, &type, &dx, &dy );
                                        if ( type == UNIT_FLEES ) {
                                            status = STATUS_MOVE;
                                            phase = PHASE_INIT_MOVE;
                                            move_unit = cur_atk;
                                            fleeing_unit = 1;
                                            dest_x = dx; dest_y = dy;
                                            break;
                                        }
                                        else
                                            if ( type == UNIT_SURRENDERS ) {
                                                surrender = 1;
                                                surrender_unit = cur_atk;
                                            }
                                    }
                                    else
                                        if ( atk_result & AR_TARGET_SUPPRESSED && 
                                             !(atk_result & AR_UNIT_SUPPRESSED) &&
                                             cur_atk->sel_prop->rng==0 ) {
                                            /* cur_target is suppressed */
                                            engine_handle_suppr( cur_def, &type, &dx, &dy );
                                            if ( type == UNIT_FLEES ) {
                                                status = STATUS_MOVE;
                                                phase = PHASE_INIT_MOVE;
                                                move_unit = cur_def;
                                                fleeing_unit = 1;
                                                dest_x = dx; dest_y = dy;
                                                break;
                                            }
                                            else
                                                if ( type == UNIT_SURRENDERS ) {
                                                    surrender = 1;
                                                    surrender_unit = cur_def;
                                                }
                                        }
                                }
                            }
                            /* clear pointers */
                            if ( cur_atk == 0 ) cur_unit = 0;
                            if ( cur_def == 0 ) cur_target = 0;
                        }
                        if ( broken_up ) {
                            phase = PHASE_BROKEN_UP_MSG;
                            label_write( gui->label_center, gui->font_error, tr("Attack Broken Up!") );
                            reset_delay( &msg_delay );
                            break;
                        }
                        if ( surrender ) {
                            const char *msg = surrender_unit->sel_prop->flags & SWIMMING ? tr("Ship is scuttled!") : tr("Surrenders!");
                            phase = PHASE_SURRENDER_MSG;
                            label_write( gui->label_center, gui->font_error, msg );
                            reset_delay( &msg_delay );
                            break;
                        }
                        phase = PHASE_END_COMBAT;
                    }
                    break;
                case PHASE_SURRENDER_MSG:
                    if ( timed_out( &msg_delay, ms ) ) {
                        if ( surrender_unit == cur_atk ) { 
                            cur_unit = 0;
                            cur_atk = 0;
                        }
                        if ( surrender_unit == cur_def ) {
                            cur_def = 0;
                            cur_target = 0;
                        }
                        engine_remove_unit( surrender_unit );
                        phase = PHASE_END_COMBAT;
                    }
                    break;
                case PHASE_BROKEN_UP_MSG:
                    if ( timed_out( &msg_delay, ms ) ) {
                        phase = PHASE_END_COMBAT;
                    }
                    break;
                case PHASE_END_COMBAT:    
#ifdef WITH_SOUND                
                    audio_fade_out( 2, 1500 ); /* explosion sound channel */
#endif
                    /* costs one fuel point for attacker */
                    if ( cur_unit && unit_check_fuel_usage( cur_unit ) && cur_unit->cur_fuel > 0 )
                        cur_unit->cur_fuel--;
                    /* update the visible units list */
                    map_get_vis_units();
                    map_set_vis_infl_mask();
                    /* reselect unit */
                    if ( cur_ctrl == PLAYER_CTRL_HUMAN )
                        engine_select_unit( cur_unit );
                    /* status */
                    engine_set_status( STATUS_NONE );
                    phase = PHASE_NONE;
                    /* allow new human/cpu input */
                    if ( !blind_cpu_turn ) {
                        if ( cur_ctrl == PLAYER_CTRL_HUMAN ) {
                            image_hide( gui->cursors, 0 );
                            gui_set_cursor( CURSOR_STD );
                        }
                        draw_map = 1;
                    }
                    break;
            }
            break;
    }
    /* update anims */
    if ( status == STATUS_ATTACK ) {
        anim_update( terrain_icons->cross, ms );
        anim_update( terrain_icons->expl1, ms );
        anim_update( terrain_icons->expl2, ms );
    }
    if ( status == STATUS_STRAT_MAP ) {
        if ( timed_out( &blink_delay, ms ) )
            strat_map_blink();
    }
    if ( status == STATUS_CAMP_BRIEFING ) {
        gui_draw_message_pane(camp_pane);
    }
    /* update gui */
    gui_update( ms );
    if ( edit_update( gui->edit, ms ) ) {
#ifdef WITH_SOUND
        wav_play( gui->wav_edit );
#endif
    }
}

/*
====================================================================
Main game loop.
If a restart is nescessary 'setup' is modified and 'reinit'
is set True.
====================================================================
*/
static void engine_main_loop( int *reinit )
{
    int ms;
    if ( status == STATUS_TITLE && !term_game ) {
        engine_draw_bkgnd();
        engine_show_game_menu(10,40);
        refresh_screen( 0, 0, 0, 0 );
    }
    else if ( status == STATUS_CAMP_BRIEFING )
        ;
    else
        engine_begin_turn( cur_player, setup.type == SETUP_LOAD_GAME /* skip unit preps then */ );
    gui_get_bkgnds();
    *reinit = 0;
    reset_timer();
    while( !end_scen && !term_game ) {
        engine_begin_frame();
        /* check input/cpu events and put to action queue */
        engine_check_events(reinit);
        /* handle queued actions */
        engine_handle_next_action( reinit );
        /* get time */
        ms = get_time();
        /* delay next scroll step */
        if ( scroll_vert || scroll_hori ) {
            if ( scroll_time > ms ) {
                set_delay( &scroll_delay, scroll_time );
                scroll_block = 1;
                scroll_vert = scroll_hori = SCROLL_NONE;
            }
            else
                set_delay( &scroll_delay, 0 );
        }
        if ( timed_out( &scroll_delay, ms ) ) 
            scroll_block = 0;
        /* update */
        engine_update( ms );
        engine_end_frame();
        /* short delay */
        SDL_Delay( 5 );
    }
    /* hide these windows, so the initial screen looks as original */
    frame_hide(gui->qinfo1, 1);
    frame_hide(gui->qinfo2, 1);
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Create engine (load resources that are not modified by scenario)
====================================================================
*/
int engine_create()
{
    gui_load( "Default" );
    return 1;
}
void engine_delete()
{
    engine_shutdown();
    scen_clear_setup();
    gui_delete();
}

/*
====================================================================
Initiate engine by loading scenario either as saved game or
new scenario by the global 'setup'.
====================================================================
*/
int engine_init()
{
    int i, j;
    Player *player;
#ifdef USE_DL
    char path[256];
#endif
    end_scen = 0;
    /* build action queue */
    actions_create();
    /* scenario&campaign or title*/
    if ( setup.type == SETUP_RUN_TITLE ) {
        status = STATUS_TITLE;
        return 1;
    }
    if ( setup.type == SETUP_CAMP_BRIEFING ) {
        status = STATUS_CAMP_BRIEFING;
        return 1;
    }
    if ( setup.type == SETUP_LOAD_GAME ) {
        if ( !slot_load( gui->load_menu->current_name ) ) return 0;
        group_set_hidden( gui->unit_buttons, ID_REPLACEMENTS, !config.merge_replacements );
        group_set_hidden( gui->unit_buttons, ID_ELITE_REPLACEMENTS, !config.merge_replacements );
        group_set_hidden( gui->unit_buttons, ID_MERGE, config.merge_replacements );
        group_set_hidden( gui->unit_buttons, ID_SPLIT, config.merge_replacements );
    }
    else 
        if ( setup.type == SETUP_INIT_CAMP ) {
            if ( !camp_load( setup.fname, setup.camp_entry_name ) ) return 0;
            camp_set_cur( setup.scen_state );
            if ( !camp_cur_scen ) return 0;
            setup.type = SETUP_CAMP_BRIEFING;
            return 1;
        }
        else
        {
            if ( !scen_load( setup.fname ) ) return 0;
            if ( setup.type == SETUP_INIT_SCEN ) {
                /* player control */
                list_reset( players );
                for ( i = 0; i < setup.player_count; i++ ) {
                    player = list_next( players );
                    player->ctrl = setup.ctrl[i];
                    free( player->ai_fname );
                    player->ai_fname = strdup( setup.modules[i] );
                }
            }
            /* select first player */
            cur_player = players_get_first();
        }
    /* store current settings to setup */
    scen_set_setup();
    /* load the ai modules */
    list_reset( players );
    for ( i = 0; i < players->count; i++ ) {
        player = list_next( players );
        /* clear callbacks */
        player->ai_init = 0;
        player->ai_run = 0;
        player->ai_finalize = 0;
        if ( player->ctrl == PLAYER_CTRL_CPU ) {
#ifdef USE_DL
            if ( strcmp( "default", player->ai_fname ) ) {
                sprintf( path, "%s/ai_modules/%s", get_gamedir(), player->ai_fname );
                if ( ( player->ai_mod_handle = dlopen( path, RTLD_GLOBAL | RTLD_NOW ) ) == 0 )
                    fprintf( stderr, "%s\n", dlerror() );
                else {
                    if ( ( player->ai_init = dlsym( player->ai_mod_handle, "ai_init" ) ) == 0 )
                        fprintf( stderr, "%s\n", dlerror() );
                    if ( ( player->ai_run = dlsym( player->ai_mod_handle, "ai_run" ) ) == 0 )
                        fprintf( stderr, "%s\n", dlerror() );
                    if ( ( player->ai_finalize = dlsym( player->ai_mod_handle, "ai_finalize" ) ) == 0 )
                        fprintf( stderr, "%s\n", dlerror() );
                }
                if ( player->ai_init == 0 || player->ai_run == 0 || player->ai_finalize == 0 ) {
                    fprintf( stderr, tr("%s: AI module '%s' invalid. Use built-in AI.\n"), player->name, player->ai_fname );
                    /* use the internal AI */
                    player->ai_init = ai_init;
                    player->ai_run = ai_run;
                    player->ai_finalize = ai_finalize;
                    if ( player->ai_mod_handle ) {
                        dlclose( player->ai_mod_handle ); 
                        player->ai_mod_handle = 0;
                    }
                }
            }
            else {
                player->ai_init = ai_init;
                player->ai_run = ai_run;
                player->ai_finalize = ai_finalize;
            }
#else
            player->ai_init = ai_init;
            player->ai_run = ai_run;
            player->ai_finalize = ai_finalize;
#endif
        }
    }
    /* no unit selected */
    cur_unit = cur_target = cur_atk = cur_def = move_unit = surp_unit = deploy_unit = surrender_unit = 0;
    df_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    /* engine */
    /* tile mask */
    /*  1 = map tile directly hit
        0 = neighbor */
    hex_mask = calloc( hex_w * hex_h, sizeof ( int ) );
    for ( j = 0; j < hex_h; j++ )
        for ( i = 0; i < hex_w; i++ )
            if ( get_pixel( terrain_icons->fog, i, j ) )
                hex_mask[j * hex_w + i] = 1;
    /* screen copy buffer */
    sc_buffer = create_surf( sdl.screen->w, sdl.screen->h, SDL_SWSURFACE );
    sc_type = 0;
    /* map geometry */
    map_x = map_y = 0;
    map_sx = -hex_x_offset;
    map_sy = -hex_h;
    for ( i = map_sx, map_sw = 0; i < gui->map_frame->w; i += hex_x_offset )
        map_sw++;
    for ( j = map_sy, map_sh = 0; j < gui->map_frame->h; j += hex_h )
        map_sh++;
    /* reset scroll delay */
    set_delay( &scroll_delay, 0 );
    scroll_block = 0;
    /* message delay */
    set_delay( &msg_delay, 1500/config.anim_speed );
    /* hide animations */
    anim_hide( terrain_icons->cross, 1 );
    anim_hide( terrain_icons->expl1, 1 );
    anim_hide( terrain_icons->expl2, 1 );
    /* remaining deploy units list */
    left_deploy_units = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
    /* build strategic map */
    strat_map_create();
    /* clear status */
    status = STATUS_NONE;
    /* weather */
    cur_weather = scen_get_weather();
    return 1;
}
void engine_shutdown()
{
    engine_set_status( STATUS_NONE );
    modify_fog = 0;
    cur_player = 0; cur_ctrl = 0;
    cur_unit = cur_target = cur_atk = cur_def = move_unit = surp_unit = deploy_unit = surrender_unit = 0;
    memset( merge_units, 0, sizeof( int ) * MAP_MERGE_UNIT_LIMIT );
    merge_unit_count = 0;
    engine_clear_backup();
    scroll_hori = scroll_vert = 0;
    last_button = 0;
    scen_delete();
    if ( df_units ) {
        list_delete( df_units );
        df_units = 0;
    }
    if ( hex_mask ) {
        free( hex_mask );
        hex_mask = 0;
    }
    if ( sc_buffer ) {
        SDL_FreeSurface( sc_buffer );
        sc_buffer = 0;
    }
    sc_type = SC_NONE;
    actions_delete();
    if ( way ) {
        free( way );
        way = 0;
        way_length = 0; way_pos = 0;
    }
    if ( left_deploy_units ) {
        list_delete( left_deploy_units );
        left_deploy_units = 0;
    }
    strat_map_delete();
}

/*
====================================================================
Run the engine (starts with the title screen)
====================================================================
*/
void engine_run()
{
    int reinit = 1;
    if (setup.type == SETUP_UNKNOWN) setup.type = SETUP_RUN_TITLE;
    while ( 1 ) {
        while ( reinit ) {
            reinit = 0;
            if(engine_init() == 0) {
              /* if engine initialisation is unsuccesful */
              /* stay with the title screen */
              status = STATUS_TITLE;
              setup.type = SETUP_RUN_TITLE;
            }
            if ( turn == 0 && (camp_loaded != NO_CAMPAIGN) && setup.type == SETUP_CAMP_BRIEFING )
                engine_prep_camp_brief();
            engine_main_loop( &reinit );
            if (term_game) break;
            engine_shutdown();
        }
        if ( scen_done() ) {
            if ( camp_loaded != NO_CAMPAIGN ) {
                engine_store_debriefing(scen_get_result());
                /* determine next scenario in campaign */
                if ( !camp_set_next( scen_get_result() ) )
                    break;
                if ( camp_cur_scen->nexts == 0 ) {
                    /* final message */
                    setup.type = SETUP_CAMP_BRIEFING;
                    reinit = 1;
                }
                else if ( !camp_cur_scen->scen ) { /* options */
                    setup.type = SETUP_CAMP_BRIEFING;
                    reinit = 1;
                }
                else {
                    /* next scenario */
                    sprintf( setup.fname, "%s", camp_cur_scen->scen );
                    setup.type = SETUP_CAMP_BRIEFING;
                    reinit = 1;
                }
            }
            else {
                setup.type = SETUP_RUN_TITLE;
                reinit = 1;
            }
        }
        else
            break;
        /* clear result before next loop (if any) */
        scen_clear_result();
    }
}

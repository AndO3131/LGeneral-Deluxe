/***************************************************************************
                          gui.h  -  description
                             -------------------
    begin                : Sun Mar 31 2002
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/
/***************************************************************************
                     Modifications by LGD team 2012+.
 ***************************************************************************/
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __GUI_H
#define __GUI_H

#include "unit.h"
#include "windows.h"

struct _List;

/*
====================================================================
Button ids
====================================================================
*/
enum {
    ID_INTERN_UP = -1000,
    ID_INTERN_DOWN,
    ID_MENU = 0,
    ID_SCEN_INFO,
    ID_AIR_MODE,
    ID_DEPLOY,
    ID_STRAT_MAP,
    ID_END_TURN,
    ID_CONDITIONS,
    ID_PURCHASE,
    ID_OK,
    ID_CANCEL,
    ID_SUPPLY,
    ID_EMBARK_AIR,
    ID_MERGE,
    ID_UNDO,
    ID_RENAME,
    ID_DISBAND,
    ID_SPLIT,
    ID_REPLACEMENTS,
    ID_ELITE_REPLACEMENTS,
    ID_SPLIT_1,
    ID_SPLIT_2,
    ID_SPLIT_3,
    ID_SPLIT_4,
    ID_SPLIT_5,
    ID_SPLIT_6,
    ID_SPLIT_7,
    ID_SPLIT_8,
    ID_SPLIT_9,
    ID_SPLIT_10,
    ID_APPLY_DEPLOY,
    ID_CANCEL_DEPLOY,
    ID_DEPLOY_UP,
    ID_DEPLOY_DOWN,
    ID_SAVE,
    ID_LOAD,
    ID_RESTART,
    ID_CAMP,
    ID_SCEN,
    ID_OPTIONS,
    ID_MOD_SELECT,
    ID_QUIT,
    ID_LOAD_OK,
    ID_LOAD_CANCEL,
    ID_SAVE_OK,
    ID_SAVE_CANCEL,
    ID_NEW_FOLDER,
    ID_C_SUPPLY,
    ID_C_WEATHER,
    ID_C_SHOW_CPU,
    ID_C_SHOW_STRENGTH,
    ID_C_SOUND,
    ID_C_SOUND_INC,
    ID_C_SOUND_DEC,
    ID_C_MUSIC,
    ID_C_GRID,
    ID_C_VMODE,
    ID_SCEN_OK,
    ID_SCEN_CANCEL,
    ID_SCEN_SETUP,
    ID_CAMP_OK,
    ID_CAMP_CANCEL,
    ID_CAMP_SETUP,
    ID_SCEN_SETUP_OK,
    ID_SCEN_SETUP_FOG,
    ID_SCEN_SETUP_SUPPLY,
    ID_SCEN_SETUP_WEATHER,
    ID_SCEN_SETUP_DEPLOYTURN,
    ID_SCEN_SETUP_PURCHASE,
    ID_SCEN_SETUP_MERGE_REPLACEMENTS,
    ID_SCEN_SETUP_CTRL,
    ID_SCEN_SETUP_MODULE,
    ID_CAMP_SETUP_OK,
    ID_CAMP_SETUP_MERGE_REPLACEMENTS,
    ID_CAMP_SETUP_CORE,
    ID_CAMP_SETUP_PURCHASE,
    ID_MODULE_OK,
    ID_MODULE_CANCEL,
    ID_PURCHASE_OK,
    ID_PURCHASE_EXIT,
    ID_PURCHASE_REFUND,
    ID_VMODE_OK,
    ID_VMODE_CANCEL,
    ID_MOD_SELECT_OK,
    ID_MOD_SELECT_CANCEL
};

/*
====================================================================
GUI graphics, fonts and windows.
====================================================================
*/
typedef struct {
    char *name;         /* directory name of gui */
    Font *font_std;
    Font *font_status;
    Font *font_error;
    Font *font_turn_info;
    Font *font_brief;
    Image *cursors;
    Label *label, *label2;
    Frame *qinfo1, *qinfo2; /* unit quick infos */
    Frame *finfo; /* full unit info */
    Frame *sinfo; /* scenario information */
    Group *unit_buttons; /* unit action buttons */
    Group *split_menu; /* menu of number of units to split up */
    Group *confirm; /* confirmation window */
    PurchaseDlg *purchase_dlg;
    Group *deploy_window;
    Edit *edit;
    Group *base_menu; /* basic menu (switch airmode, deploy, end turn, menu ...) */
    Group *main_menu; /* main game menu */
    Group *opt_menu;  /* options */
    FDlg *load_menu;
    FDlg *save_menu;
    SelectDlg *vmode_dlg;
    FDlg *scen_dlg;    /* scenario selection */
    FDlg *camp_dlg;    /* campaign selection */
    SDlg *scen_setup;  /* scenario setup (controls and ai modules) */
    SDlg *camp_setup;  /* campaign setup (controls) */
    FDlg *module_dlg;  /* ai module selection */
    FDlg *mod_select_dlg; /* mod selection */
    /* frame tiles */
    SDL_Surface *fr_luc, *fr_llc, *fr_ruc, *fr_rlc, *fr_hori, *fr_vert;
    SDL_Surface *brief_frame;   /* briefing window */
    SDL_Surface *bkgnd;         /* background for briefing window and main menu */
    SDL_Surface *wallpaper;     /* wallpaper used if background doesn't fill the whole screen */
    SDL_Surface *folder_icon;   /* folder icon for file list */
    /* sounds */
#ifdef WITH_SOUND
    Wav *wav_click;
    Wav *wav_edit;
#endif
    /* misc */
    int mirror_asymm; /* mirror asymmetric windows to look behind */
} GUI;

struct MessagePane;

/*
====================================================================
Create the gui and use the graphics in gfx/themes/path
====================================================================
*/
int gui_load( char *path );
void gui_delete();

/*
====================================================================
Move all windows to there proper position according to screen's
measurements.
====================================================================
*/
void gui_adjust();

/*
====================================================================
Change all GUI graphics to the one found in gfx/theme/path.
====================================================================
*/
int gui_change( const char *path );

/*
====================================================================
Hide/draw from/to screen or get backgrounds
====================================================================
*/
void gui_get_bkgnds();
void gui_draw_bkgnds();
void gui_draw();

/*
====================================================================
Set cursor.
====================================================================
*/
enum {
    CURSOR_STD = 0,
    CURSOR_SELECT,
    CURSOR_ATTACK,
    CURSOR_MOVE,
    CURSOR_MOUNT,
    CURSOR_DEBARK,
    CURSOR_EMBARK,
    CURSOR_MERGE,
    CURSOR_DEPLOY,
    CURSOR_UNDEPLOY,
    CURSOR_UP,
    CURSOR_DOWN,
    CURSOR_LEFT,
    CURSOR_RIGHT,
    CURSOR_COUNT
};
void gui_set_cursor( int type );

/*
====================================================================
Handle events.
====================================================================
*/
int gui_handle_motion( int cx, int cy );
int  gui_handle_button( int button_id, int cx, int cy, Button **button );
void gui_update( int ms );

/*
====================================================================
Set quick info frame with information on this unit and set hide = 0.
====================================================================
*/
void gui_show_quick_info( Frame *qinfo, Unit *unit );

/*
====================================================================
Draw the expected losses to the label.
====================================================================
*/
void gui_show_expected_losses( Unit *att, Unit *def, int att_dam, int def_dam );

/*
====================================================================
Draw the actual losses to the label.
====================================================================
*/
void gui_show_actual_losses( Unit *att, Unit *def, 
    int att_suppr, int att_dam, int def_suppr, int def_dam );
    
/*
====================================================================
Draw the given unit's strength or suppression to the given label
====================================================================
*/
void gui_show_unit_results( Unit *unit, Label *label, int is_str, int dam );

/*
====================================================================
Show full info window.
====================================================================
*/
void gui_show_full_info( Unit *unit );

/*
====================================================================
Show scenario info window.
====================================================================
*/
void gui_show_scen_info();

/*
====================================================================
Show explicit victory conditions and use scenario info window for
this.
====================================================================
*/
void gui_show_conds();

/*
====================================================================
Show confirmation window.
====================================================================
*/
void gui_show_confirm( const char *text );

/*
====================================================================
Show unit buttons at screen x,y (does not include the button check)
====================================================================
*/
void gui_show_unit_buttons( int x, int y );

/** Show unit purchase dialogue. */
void gui_show_purchase_window();

/*
====================================================================
Show deploy window and select first unit as 'deploy_unit'.
====================================================================
*/
void gui_show_deploy_window();

/*
====================================================================
Handle deploy window.
  gui_handle_deploy_motion: 'unit' is the unit the cursor is 
      currently above
  gui_handle_deploy_click: 'new_unit' is set True if a new unit was
      selected (which is 'deploy_unit' ) else False
      return True if something happended
====================================================================
*/
int gui_handle_deploy_click( int button, int cx, int cy );
void gui_handle_deploy_motion( int cx, int cy, Unit **unit );

/*
====================================================================
Scroll deploy list up/down.
====================================================================
*/
void gui_scroll_deploy_up();
void gui_scroll_deploy_down();

/*
====================================================================
Update deploy list. Unit is either removed or added to 
left_deploy_units and the deploy window is updated.
====================================================================
*/
void gui_remove_deploy_unit( Unit *unit );
void gui_add_deploy_unit( Unit *unit );

/*
====================================================================
Show base menu at screen x,y (does not include the button check)
====================================================================
*/
void gui_show_menu( int x, int y );

/*
====================================================================
Render the file name to surface. (directories start with an
asteriks)
====================================================================
*/
void gui_render_file_name( void *item, SDL_Surface *buffer );

/*
====================================================================
Render scenario/campaign name to surface. (directories start with an
asteriks)
====================================================================
*/
void gui_render_scen_file_name( void *item, SDL_Surface *buffer );

/*
====================================================================
Handle the selection of a scenario file (display info and 
load scen_info from full path)
====================================================================
*/
void gui_render_scen_info( const char *path, const char *camp_entry, SDL_Surface *buffer );

/*
====================================================================
Handle the selection of a file to load.
@camp_entry is unused
====================================================================
*/
void gui_render_load_menu( const char *path, const char *camp_entry, SDL_Surface *buffer );

/*
====================================================================
Handle the selection of a file to save.
@camp_entry is unused
====================================================================
*/
void gui_render_save_menu( const char *path, const char *camp_entry, SDL_Surface *buffer );

/*
====================================================================
Handle the selection of a campaign file (display info and 
load scen_info from full path)
====================================================================
*/
void gui_render_camp_info( const char *path, const char *camp_entry, SDL_Surface *buffer );

/*
====================================================================
Handle loading saved file (render confirmation dialogue)
====================================================================
*/
void gui_render_load( const char *path, SDL_Surface *buffer );

/*
====================================================================
Creates a new message pane structure.
====================================================================
*/
struct MessagePane *gui_create_message_pane();

/*
====================================================================
Deletes the given message pane structure.
====================================================================
*/
void gui_delete_message_pane(struct MessagePane *pane);

/*
====================================================================
Sets the text for the message pane.
====================================================================
*/
void gui_set_message_pane_text( struct MessagePane *pane, const char *text );

/*
====================================================================
Sets the default id for the message pane.
====================================================================
*/
void gui_set_message_pane_default( struct MessagePane *pane, const char *default_id );

/*
====================================================================
Returns the currently selected id or 0 if nothing is selected.
====================================================================
*/
const char *gui_get_message_pane_selection( struct MessagePane *pane );

/*
====================================================================
Fills in options for the given message pane.
ids const char * list of ids
descs const char * list of textual description being mapped to ids.
====================================================================
*/
void gui_set_message_pane_options( struct MessagePane *pane, struct _List *ids, struct _List *descs );

/*
====================================================================
Draws and fills with text the message pane.
====================================================================
*/
void gui_draw_message_pane( struct MessagePane *pane );

/*
====================================================================
Handles events on the message pane.
pane - message pane data
mx, my - screen coordinates of mouse
button - mouse button
pressed - 1 if mouse button was pressed, 0 if released (undefined
	if button is BUTTON_NONE)
====================================================================
*/
void gui_handle_message_pane_event( struct MessagePane *pane, int mx, int my, int button, int pressed );

/*
====================================================================
Open scenario setup and set first player as selected.
====================================================================
*/
void gui_open_scen_setup();

/*
====================================================================
Open campaign setup.
====================================================================
*/
void gui_open_camp_setup();

/*
====================================================================
Render the player name in the scenario setup
====================================================================
*/
void gui_render_player_name( void *item, SDL_Surface *buffer );

/*
====================================================================
Handle the selection of a player in setup.
====================================================================
*/
void gui_handle_player_select( void *item );

/*
====================================================================
Load a module's info.
@camp_entry is unused
====================================================================
*/
void gui_render_module_info( const char *path, const char *camp_entry, SDL_Surface *buffer );

/*
====================================================================
Handle the selection of a mod folder to load.
@camp_entry is unused
====================================================================
*/
void gui_render_mod_select_info( const char *path, const char *camp_entry, SDL_Surface *buffer );

/*
====================================================================
Mirror position of asymmetric windows.
====================================================================
*/
void gui_mirror_asymm_windows();

/** Show video mode selection */
void gui_vmode_dlg_show();

#endif

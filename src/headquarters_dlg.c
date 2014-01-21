/***************************************************************************
                          headquartes.h  -  description
                             -------------------
    begin                : Mon Jan 20 2014
    copyright            : (C) 2014 by Andrzej Owsiejczuk
    email                : andre.owsiejczuk@wp.pl
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
#include "sdl.h"
#include "image.h"
#include "list.h"
#include "windows.h"
#include "headquarters_dlg.h"
#include "gui.h"
#include "localize.h"

extern Sdl sdl;
extern GUI *gui;
extern Player *cur_player;
extern List *units;
/*
#include <SDL.h>
#include <stdlib.h>
#include "lgeneral.h"
#include "sdl.h"
#include "date.h"
#include "event.h"
#include "image.h"
#include "list.h"
#include "windows.h"
#include "unit.h"
#include "purchase_dlg.h"
#include "gui.h"
#include "localize.h"
#include "scenario.h"
#include "file.h"
#include "campaign.h"

extern Sdl sdl;
extern GUI *gui;
extern SDL_Surface *gui_create_frame( int w, int h );
extern Nation *nations;
extern int nation_count;
extern int nation_flag_width, nation_flag_height;
extern Unit_Class *unit_classes;
extern int unit_class_count;
extern Player *cur_player;
extern List *unit_lib;
extern Scen_Info *scen_info;
extern SDL_Surface *nation_flags;
extern Trgt_Type *trgt_types;
extern int trgt_type_count;
extern Mov_Type *mov_types;
extern int mov_type_count;
extern List *reinf;
extern List *avail_units;
extern List *units;
extern int turn;
extern Config config;
extern enum CampaignState camp_loaded;
*/
/****** Private *****/

/** Get all units from active units list which belong to current player.
 *
 * Return as new list with pointers to global units list (will not be deleted 
 * on deleting list) */
static List *get_active_player_units( void )
{
	List *l = list_create( LIST_NO_AUTO_DELETE, LIST_NO_CALLBACK );
	Unit *u = NULL;
	
	if (l == NULL) {
		fprintf( stderr, tr("Out of memory\n") );
		return NULL;
	}
	
	list_reset(units);
	while ((u = list_next(units)))
		if (u->player == cur_player)
			list_add( l, u );
	return l;
}

/** Render purchase info for unit lib entry @entry to surface @buffer at 
 * position @x, @y. *
static void render_unit_lib_entry_info( Unit_Lib_Entry *entry,
					SDL_Surface *buffer, int x, int y )
{
	char str[128];
	int i, start_y = y;
	
	gui->font_std->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
	
	/* first column: name, type, ammo, fuel, spot, mov, ini, range *
	write_line( buffer, gui->font_std, entry->name, x, &y );
	write_line( buffer, gui->font_std, 
				unit_classes[entry->class].name, x, &y );
	y += 5;
	sprintf( str, tr("Cost:       %3i"), entry->cost );
	write_line( buffer, gui->font_std, str, x, &y );
	if ( entry->ammo == 0 )
		sprintf( str, tr("Ammo:         --") );
	else
		sprintf( str, tr("Ammo:        %2i"), entry->ammo );
	write_line( buffer, gui->font_std, str, x, &y );
	if ( entry->fuel == 0 )
		sprintf( str, tr("Fuel:        --") );
	else
		sprintf( str, tr("Fuel:        %2i"), entry->fuel );
	write_line( buffer, gui->font_std, str, x, &y );
	y += 5;
	sprintf( str, tr("Spotting:    %2i"), entry->spt );
	write_line( buffer, gui->font_std, str, x, &y );
	sprintf( str, tr("Movement:    %2i"), entry->mov );
	write_line( buffer, gui->font_std, str, x, &y );
	sprintf( str, tr("Initiative:  %2i"), entry->ini );
	write_line( buffer, gui->font_std, str, x, &y );
	sprintf( str, tr("Range:       %2i"), entry->rng );
	write_line( buffer, gui->font_std, str, x, &y );

	/* second column: move type, target type, attack, defense *
	y = start_y;
	x += 135;
	sprintf( str, tr("%s Movement"), mov_types[entry->mov_type].name );
	write_line( buffer, gui->font_std, str, x, &y );
	sprintf( str, tr("%s Target"), trgt_types[entry->trgt_type].name );
	write_line( buffer, gui->font_std, str, x, &y );
	y += 5;
	for ( i = 0; i < trgt_type_count; i++ ) {
		char valstr[8];
		int j;
		
		sprintf( str, tr("%s Attack:"), trgt_types[i].name );
		for (j = strlen(str); j < 14; j++)
			strcat( str, " " );
		if ( entry->atks[i] < 0 )
			snprintf( valstr, 8, "[%2i]", -entry->atks[i] );
		else
			snprintf( valstr, 8, "  %2i", entry->atks[i] );
		strcat(str, valstr);
		write_line( buffer, gui->font_std, str, x, &y );
	}
	y += 5;
	sprintf( str, tr("Ground Defense: %2i"), entry->def_grnd );
	write_line( buffer, gui->font_std, str, x, &y );
	sprintf( str, tr("Air Defense:    %2i"), entry->def_air );
	write_line( buffer, gui->font_std, str, x, &y );
	sprintf( str, tr("Close Defense:  %2i"), entry->def_cls );
	write_line( buffer, gui->font_std, str, x, &y );
}

/** Calculate how many units player may purchase: Unit limit - placed units - 
 * ordered units - deployable units. Return number or -1 if no limit. *
int player_get_purchase_unit_limit( Player *player, int type )
{
    int limit;
    if (type == CORE)
        limit = player->core_limit;
    else
        limit = player->unit_limit;
	Unit *u = NULL;
	
	if (limit == 0)
		return -1;
	
    if ((type == AUXILIARY) && (camp_loaded != NO_CAMPAIGN) && config.use_core_units)
        limit = player->unit_limit - player->core_limit;
	/* units placed on map *
	list_reset( units );
	while ((u = list_next( units )))
    {
        if (u->player == player && u->killed == 0 && u->core == type )
                limit--;
    }
	/* units ordered, not yet arrived *
	list_reset( reinf );
	while ((u = list_next( reinf )))
        if (u->player == player && u->killed == 0 && u->core == type )
			limit--;
	/* units arrived, not yet placed *
    list_reset( avail_units );
    while ((u = list_next( avail_units )))
        if (u->player == player && u->killed == 0 && u->core == type )
            limit--;
	
	return limit;
}

/** Check whether player @p can purchase unit lib entry @unit with
 * transporter @trsp (if not NULL) regarding available prestige and unit
 * capacity. Return 1 if purchase okay, 0 otherwise.
 * It is not checked whether @unit may really have @trsp as transporter,
 * this has to be asserted before calling this function. *
int player_can_purchase_unit( Player *p, Unit_Lib_Entry *unit,
							Unit_Lib_Entry *trsp)
{
	int cost = unit->cost + ((trsp)?trsp->cost:0);
	
    if (!config.use_core_units)
    {
        if (player_get_purchase_unit_limit(p,AUXILIARY) == 0)
            return 0;
    }
    else
    {
        if ( (player_get_purchase_unit_limit(p,AUXILIARY) == 0) && (camp_loaded == NO_CAMPAIGN) )
            return 0;
        else
            if (player_get_purchase_unit_limit(p,CORE) + player_get_purchase_unit_limit(p,AUXILIARY) == 0 &&
               (camp_loaded != NO_CAMPAIGN))
                return 0;
    }
	if (p->cur_prestige < cost)
		return 0;
	return 1;
}

/** Render info of selected unit (unit, transporter, total cost, ... ) 
 * and enable/disable purchase button depending on whether enough prestige
 * and capacity. *
static void update_unit_purchase_info( PurchaseDlg *pdlg )
{
	int total_cost = 0;
	SDL_Surface *contents = pdlg->main_group->frame->contents;
	Unit_Lib_Entry *unit_entry = NULL, *trsp_entry = NULL;
	Unit *reinf_unit = NULL;
	
	SDL_FillRect( contents, 0, 0x0 );
	
	/* get selected objects *
	reinf_unit = lbox_get_selected_item( pdlg->reinf_lbox );
	if (reinf_unit) {
		unit_entry = &reinf_unit->prop;
		if (reinf_unit->trsp_prop.id)
			trsp_entry = &reinf_unit->trsp_prop;
	} else {
		unit_entry = lbox_get_selected_item( pdlg->unit_lbox );
		trsp_entry = lbox_get_selected_item( pdlg->trsp_lbox );
	}
	
	/* unit info *
	if (unit_entry) {
		render_unit_lib_entry_info( unit_entry, contents, 10, 10 );
		total_cost += unit_entry->cost;
		
		/* if no transporter allowed clear transporter list. if allowed
		 * but list empty, fill it. *
		if (reinf_unit)
			; /* unit/trsp has been cleared already *
		else if(!(unit_entry->flags & GROUND_TRSP_OK)) {
			lbox_clear_items(pdlg->trsp_lbox);
			trsp_entry = NULL;
		} else if (lbox_is_empty(pdlg->trsp_lbox) && pdlg->trsp_uclass)
			lbox_set_items(pdlg->trsp_lbox,
					get_purchasable_unit_lib_entries(
					pdlg->cur_nation->id,
					pdlg->trsp_uclass->id,
					(const Date*)&scen_info->start_date));
	}
	
	/* transporter info *
	if (trsp_entry) {
		render_unit_lib_entry_info( trsp_entry, contents, 10, 155 );
		total_cost += trsp_entry->cost;
	}
	
	/* show cost if any selection; mark red if not purchasable *
	if ( total_cost > 0 ) {
		char str[128];
		Font *font = gui->font_std;
		int y = group_get_height( pdlg->main_group ) - 10;
		
		snprintf( str, 128, tr("Total Cost: %d"), total_cost );
		if (!reinf_unit && !player_can_purchase_unit(cur_player, 
						unit_entry, trsp_entry)) {
			font = gui->font_error;
			total_cost = 0; /* indicates "not purchasable" here *
		}
		font->align = ALIGN_X_LEFT | ALIGN_Y_BOTTOM;
		write_text( font, contents, 10, y, str, 255 );
		
		if (total_cost > 0) {
			font->align = ALIGN_X_RIGHT | ALIGN_Y_BOTTOM;
			write_text( font, contents, 
				group_get_width( pdlg->main_group ) - 62, y,
				reinf_unit?tr("REFUND?"):tr("REQUEST?"), 255 );
		}
	}
	
	/* if total cost is set, purchase is possible *
	if (total_cost > 0)
		group_set_active( pdlg->main_group, ID_PURCHASE_OK, 1 );
	else
		group_set_active( pdlg->main_group, ID_PURCHASE_OK, 0 );

	/* apply rendered contents *
	frame_apply( pdlg->main_group->frame );
}

/** Modify @pdlg's current unit limit by adding @add and render unit limit. *
static void update_purchase_unit_limit( PurchaseDlg *pdlg, int add, int type )
{
	SDL_Surface *contents = pdlg->ulimit_frame->contents;
	char str[16];
	int y = 7;
	
    if (type == CORE)
    {
        if (pdlg->cur_core_unit_limit != -1)
        {
            pdlg->cur_core_unit_limit += add;
            if (pdlg->cur_core_unit_limit < 0)
                pdlg->cur_core_unit_limit = 0;
        }
    }
    else
    {
        if (pdlg->cur_unit_limit != -1)
        {
            pdlg->cur_unit_limit += add;
            if (pdlg->cur_unit_limit < 0)
                pdlg->cur_unit_limit = 0;
	    }
    }

	SDL_FillRect( contents, 0, 0x0 );
	gui->font_std->align = ALIGN_X_CENTER | ALIGN_Y_TOP;
    if (!config.use_core_units)
        write_line(contents, gui->font_std, tr("Available:"), contents->w/2, &y );
    else
    {
        write_line(contents, gui->font_std, tr("Core:"), contents->w/2, &y );
        if (pdlg->cur_core_unit_limit == -1)
            strcpy( str, tr("None") );
        else
            snprintf(str, 16, "%d %s", pdlg->cur_core_unit_limit,
                    (pdlg->cur_core_unit_limit == 1)?tr("Unit"):tr("Units"));
        write_line( contents, gui->font_std, str, contents->w/2, &y );
        write_line(contents, gui->font_std, tr("Auxiliary:"), contents->w/2, &y );
    }
	if (pdlg->cur_unit_limit == -1)
		strcpy( str, tr("None") );
	else
		snprintf(str, 16, "%d %s", pdlg->cur_unit_limit,
			(pdlg->cur_unit_limit==1)?tr("Unit"):tr("Units"));
	write_line( contents, gui->font_std, str, contents->w/2, &y );
	
	frame_apply( pdlg->ulimit_frame );
}

/** Update listboxes (and clear current selection) for unit and transporter
 * according to current settings in @pdlg. *
static void update_purchasable_units( PurchaseDlg *pdlg )
{
	lbox_set_items(pdlg->unit_lbox, get_purchasable_unit_lib_entries(
				pdlg->cur_nation->id, pdlg->cur_uclass->id,
				(const Date*)&scen_info->start_date));
	lbox_clear_items(pdlg->trsp_lbox);
	update_unit_purchase_info(pdlg); /* clear info *
}

/** Purchase unit for player @player. @nation is nation of unit (should match
 * with list of player but is not checked), @type is CORE or AUXILIARY,
 * @unit_prop and @trsp_prop are the  * unit lib entries (trsp_prop may be NULL
 * for no transporter). *
void player_purchase_unit( Player *player, Nation *nation, int type,
			Unit_Lib_Entry *unit_prop, Unit_Lib_Entry *trsp_prop )
{
	Unit *unit = NULL, unit_base;
	
	/* build unit base info *
	memset( &unit_base, 0, sizeof( Unit ) );
	unit_base.nation = nation;
	unit_base.player = player;
	/* FIXME: no global counter to number unit so use plain name *
	snprintf(unit_base.name, sizeof(unit_base.name), "%s", unit_prop->name);
    if (config.purchase != INSTANT_PURCHASE)
        unit_base.delay = turn + 1; /* available next turn *
    else
        unit_base.delay = turn; /* available right away *
	unit_base.str = 10;
    unit_base.max_str = 10;
	unit_base.orient = cur_player->orient;
	
	/* create unit and push to reinf list *
	if ((unit = unit_create( unit_prop, trsp_prop, 0, &unit_base )) == NULL) {
		fprintf( stderr, tr("Out of memory\n") );
		return;
	}
    unit->core = type;
    if (config.purchase != INSTANT_PURCHASE)
        list_add( reinf, unit );
    else
        list_add( avail_units, unit );
	
	/* pay for it *
	player->cur_prestige -= unit_prop->cost;
	if (trsp_prop)
		player->cur_prestige -= trsp_prop->cost;
}

/** Search for unit @u in reinforcements list. If found, delete it and refund
 * prestige to player. *
static void player_refund_unit( Player *p, Unit *u )
{
	Unit *e;
	
	list_reset(reinf);
	while ((e = list_next(reinf))) {
		if (e != u)
			continue;
		p->cur_prestige += u->prop.cost;
		if (u->trsp_prop.id)
			p->cur_prestige += u->trsp_prop.cost;
		list_delete_current(reinf);
		return;
	}
    if (config.purchase == INSTANT_PURCHASE)
    {
        list_reset(avail_units);
        while ((e = list_next(avail_units))) {
            if (e != u)
                continue;
            p->cur_prestige += u->prop.cost;
            if (u->trsp_prop.id)
                p->cur_prestige += u->trsp_prop.cost;
            list_delete_current(avail_units);
            return;
        }
    }
}

/** Handle click on purchase button: Purchase unit according to settings in
 * @pdlg and update info and reinf. It is not checked whether current player 
 * may really purchase this unit. This has to be asserted before calling this 
 * function. *
static void handle_purchase_button( PurchaseDlg *pdlg )
{
	Nation *nation = lbox_get_selected_item( pdlg->nation_lbox );
	Unit_Lib_Entry *unit_prop = lbox_get_selected_item( pdlg->unit_lbox );
	Unit_Lib_Entry *trsp_prop = lbox_get_selected_item( pdlg->trsp_lbox );
	Unit *reinf_unit = lbox_get_selected_item( pdlg->reinf_lbox );

	if (reinf_unit == NULL) {
        if (pdlg->cur_core_unit_limit > 0)
        {
            player_purchase_unit( cur_player, nation, CORE, unit_prop, trsp_prop );
            update_purchase_unit_limit( pdlg, -1, CORE );
        }
        else
        {
            player_purchase_unit( cur_player, nation, AUXILIARY, unit_prop, trsp_prop );
            update_purchase_unit_limit( pdlg, -1, AUXILIARY );
        }
	}
    else
    {
		player_refund_unit( cur_player, reinf_unit );
        update_purchase_unit_limit( pdlg, 1, reinf_unit->core );
	}
	lbox_set_items( pdlg->reinf_lbox, get_reinf_units() );
	update_unit_purchase_info( pdlg );
}
*/

/** Render icon and name of unit lib entry @data to surface @buffer. */
static void render_lbox_unit( void *data, SDL_Surface *buffer )
{
	Unit_Lib_Entry *e = (Unit_Lib_Entry*)data;
	char name[13];
	
	SDL_FillRect( buffer, 0, 0x0 );
	gui->font_std->align = ALIGN_X_CENTER | ALIGN_Y_BOTTOM;
	
        DEST( buffer, (buffer->w - e->icon_w)/2, (buffer->h - e->icon_h)/2, 
							e->icon_w, e->icon_h );
        SOURCE( e->icon, 0, 0 );
        blit_surf();
	
	snprintf(name,13,"%s",e->name); /* truncate name */
        write_text( gui->font_std, buffer, buffer->w / 2, buffer->h, name, 255 );
}

/****** Public *****/

/** Create and return pointer to headquarters dialogue. Use graphics from theme
 * path @theme_path. */
HeadquartersDlg *headquarters_dlg_create( char *theme_path )
{
	HeadquartersDlg *hdlg = NULL;
	char path[MAX_PATH], transitionPath[MAX_PATH];
	int sx, sy;
	
	hdlg = calloc( 1, sizeof(HeadquartersDlg) );
	if (hdlg == NULL)
		return NULL;
	
	/* create main group (= main window) */
	snprintf( transitionPath, MAX_PATH, "headquarters_buttons" );
    search_file_name( path, 0, transitionPath, theme_path, 'i' );
	hdlg->main_group = group_create( gui_create_frame( 500, 400 ), 160, 
				load_surf( path, SDL_SWSURFACE, 0, 0, 0, 0 ),
				20, 20, 2, ID_HEADQUARTERS_CLOSE,
				sdl.screen, 0, 0 );
	if (hdlg->main_group == NULL)
		goto failure;
	sx = group_get_width( hdlg->main_group ) - 30; 
	sy = group_get_height( hdlg->main_group ) - 25;
	group_add_button( hdlg->main_group, ID_HEADQUARTERS_CLOSE, sx, sy, 0, 
							tr("Close"), 2 );
	group_add_button( hdlg->main_group, ID_HEADQUARTERS_GO_TO_UNIT, sx - 30, sy, 0, 
							tr("Center On Unit"), 2 );
	
	/* create unit limit info frame *
    pdlg->ulimit_frame = frame_create( gui_create_frame( 112, 65 ), 160,
                     sdl.screen, 0, 0);
	if (pdlg->ulimit_frame == NULL)
		goto failure;
	
	/* create nation listbox *
	snprintf( transitionPath, MAX_PATH, "scroll_buttons" );
    search_file_name( path, 0, transitionPath, theme_path, 'i' );
	pdlg->nation_lbox = lbox_create( gui_create_frame( 112, 74 ), 160, 6,
			load_surf( path, SDL_SWSURFACE, 0, 0, 0, 0 ), 24, 24,
			3, 1, 100, 12, 1, 0x0000ff,
			render_lbox_nation, sdl.screen, 0, 0);
	if (pdlg->nation_lbox == NULL)
		goto failure;
	
	/* create class listbox *
	snprintf( transitionPath, MAX_PATH, "scroll_buttons" );
    search_file_name( path, 0, transitionPath, theme_path, 'i' );
	pdlg->uclass_lbox = lbox_create( gui_create_frame( 112, 166 ), 160, 6,
			load_surf( path, SDL_SWSURFACE, 0, 0, 0, 0 ), 24, 24,
			10, 2, 100, 12, 1, 0x0000ff,
			render_lbox_uclass, sdl.screen, 0, 0);
	if (pdlg->uclass_lbox == NULL)
		goto failure;
	
	/* create units listbox */
	snprintf( transitionPath, MAX_PATH, "scroll_buttons" );
    search_file_name( path, 0, transitionPath, theme_path, 'i' );
	hdlg->units_lbox = lbox_create( gui_create_frame( 112, 380 ), 160, 6,
			load_surf( path, SDL_SWSURFACE, 0, 0, 0, 0 ), 24, 24,
			8, 3, 100, 40, 1, 0x0000ff,
			render_lbox_unit, sdl.screen, 0, 0);
	if (hdlg->units_lbox == NULL)
		goto failure;
	
	/* create transporters listbox *
	snprintf( transitionPath, MAX_PATH, "scroll_buttons" );
    search_file_name( path, 0, transitionPath, theme_path, 'i' );
	pdlg->trsp_lbox = lbox_create( gui_create_frame( 112, 120 ), 160, 6,
			load_surf( path, SDL_SWSURFACE, 0, 0, 0, 0 ), 24, 24,
			2, 1, 100, 40, 1, 0x0000ff,
			render_lbox_unit, sdl.screen, 0, 0);
	if (pdlg->trsp_lbox == NULL)
		goto failure;
	
	/* create reinforcements listbox *
	snprintf( transitionPath, MAX_PATH, "scroll_buttons" );
    search_file_name( path, 0, transitionPath, theme_path, 'i' );
	pdlg->reinf_lbox = lbox_create( gui_create_frame( 112, 280 ), 160, 6,
			load_surf( path, SDL_SWSURFACE, 0, 0, 0, 0 ), 24, 24,
			6, 3, 100, 40, 1, 0x0000ff,
			render_lbox_reinf, sdl.screen, 0, 0);
	if (pdlg->reinf_lbox == NULL)
		goto failure;
	*/
	return hdlg;
failure:
	fprintf(stderr, tr("Failed to create headquarters dialogue\n"));
	headquarters_dlg_delete(&hdlg);
	return NULL;
}

void headquarters_dlg_delete( HeadquartersDlg **hdlg )
{
	if (*hdlg) {
		HeadquartersDlg *ptr = *hdlg;
		
		group_delete( &ptr->main_group );
		lbox_delete( &ptr->units_lbox );
		free( ptr );
	}
	*hdlg = NULL;
}

/** Return width of headquarters dialogue @hdlg by adding up all components. */
int headquarters_dlg_get_width(HeadquartersDlg *hdlg)
{
	return group_get_width( hdlg->main_group ) +
		lbox_get_width( hdlg->units_lbox );
}

/** Return height of headquarters dialogue @hdlg by adding up all components. */
int headquarters_dlg_get_height(HeadquartersDlg *hdlg)
{
	return group_get_height( hdlg->main_group );
}

/** Move headquarters dialogue @hdlg to position @px, @py by moving all separate
 * components. */
void headquarters_dlg_move( HeadquartersDlg *hdlg, int px, int py)
{
	int sx = px, sy = py;
	int units_offset = (group_get_height( hdlg->main_group ) -
			lbox_get_height( hdlg->units_lbox )) / 2;
	
	group_move( hdlg->main_group, sx, sy );
	sx += group_get_width( hdlg->main_group );
	lbox_move(hdlg->units_lbox, sx, sy + units_offset);
}

/* Hide, draw, draw background, get background for headquarters dialogue @hdlg by
 * applying action to all separate components. */
void headquarters_dlg_hide( HeadquartersDlg *hdlg, int value)
{
	group_hide(hdlg->main_group, value);
	lbox_hide(hdlg->units_lbox, value);
}
void headquarters_dlg_draw( HeadquartersDlg *hdlg)
{
	group_draw(hdlg->main_group);
	lbox_draw(hdlg->units_lbox);
}
void headquarters_dlg_draw_bkgnd( HeadquartersDlg *hdlg)
{
	group_draw_bkgnd(hdlg->main_group);
	lbox_draw_bkgnd(hdlg->units_lbox);
}
void headquarters_dlg_get_bkgnd( HeadquartersDlg *hdlg)
{
	group_get_bkgnd(hdlg->main_group);
	lbox_get_bkgnd(hdlg->units_lbox);
}

/** Handle mouse motion for purchase dialogue @pdlg by checking all components.
 * @cx, @cy is absolute mouse position in screen. Return 1 if action has been
 * handled by some window, 0 otherwise. */
int headquarters_dlg_handle_motion( HeadquartersDlg *hdlg, int cx, int cy)
{
	int ret = 1;
	void *item = NULL;
	
	if (!group_handle_motion(hdlg->main_group,cx,cy))
	if (!lbox_handle_motion(hdlg->units_lbox,cx,cy,&item))
		ret = 0;
	return ret;
}

/** Handle mouse click to screen position @cx,@cy with button @bid. 
 * Return 1 if a button was clicked that needs upstream processing (e.g., to
 * close dialogue) and set @pbtn then. Otherwise process event internally and
 * return 0. */
int headquarters_dlg_handle_button( HeadquartersDlg *hdlg, int bid, int cx, int cy,
		Button **pbtn )
{
	void *item = NULL;
	
	if (group_handle_button(hdlg->main_group,bid,cx,cy,pbtn)) {
		/* catch and handle purchase button internally */
/*		if ((*pbtn)->id == ID_PURCHASE_OK) {
			handle_purchase_button( hdlg );
			return 0;
		}*/
		return 1;
	}
/*	if (lbox_handle_button(pdlg->nation_lbox,bid,cx,cy,pbtn,&item)) {
		if ( item && item != pdlg->cur_nation ) {
			pdlg->cur_nation = (Nation*)item;
			update_purchasable_units(pdlg);
		}
		return 0;
	}
	if (lbox_handle_button(pdlg->uclass_lbox,bid,cx,cy,pbtn,&item)) {
		if ( item && item != pdlg->cur_uclass ) {
			pdlg->cur_uclass = (Unit_Class*)item;
			update_purchasable_units(pdlg);
		}
		return 0;
	}*/
	if (lbox_handle_button(hdlg->units_lbox,bid,cx,cy,pbtn,&item)) {
		if (item) {
			/* clear reinf selection since we share info window */
//			lbox_clear_selected_item( pdlg->reinf_lbox );
			gui_show_full_info( hdlg->main_group->frame, lbox_get_selected_item( hdlg->units_lbox ) );
		}
		return 0;
	}
/*	if (lbox_handle_button(pdlg->trsp_lbox,bid,cx,cy,pbtn,&item)) {
		if (item) {
			if (bid == BUTTON_RIGHT) /* deselect entry *
				lbox_clear_selected_item( pdlg->trsp_lbox );
			/* clear reinf selection since we share info window *
			lbox_clear_selected_item( pdlg->reinf_lbox );
			update_unit_purchase_info( pdlg );
		}
		return 0;
	}
	if (lbox_handle_button(pdlg->reinf_lbox,bid,cx,cy,pbtn,&item)) {
		if (item) {
			/* clear unit selection since we share info window *
			lbox_clear_selected_item( pdlg->unit_lbox );
			lbox_clear_items( pdlg->trsp_lbox );
			update_unit_purchase_info( pdlg );
		}
		return 0;
	}*/
	return 0;
}

/** Reset purchase dialogue settings for global @cur_player */
void headquarters_dlg_reset( HeadquartersDlg *hdlg )
{
	lbox_set_items( hdlg->units_lbox, get_active_player_units() );
}

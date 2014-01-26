/***************************************************************************
                          message_dlg.h  -  description
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
#include "message_dlg.h"
#include "gui.h"
#include "localize.h"
#include "file.h"
#include "event.h"

extern Sdl sdl;
extern GUI *gui;
extern int hex_h;
extern Player *cur_player;

char messages[MAX_NAME][MAX_MESSAGE_LINE]; /* message dialogue box previous text */
int messages_count = 0;                    /* number of messages stored */
int last_selected_message = 0;             /* last message to display */

/****** Public *****/

/** Create and return pointer to message dialogue. Use graphics from theme
 * path @theme_path. */
MessageDlg *message_dlg_create( char *theme_path )
{
	MessageDlg *mdlg = NULL;
	char path[MAX_PATH], transitionPath[MAX_PATH];
	int sx, sy;
	
	mdlg = calloc( 1, sizeof(MessageDlg) );
	if (mdlg == NULL)
		return NULL;
	
	/* create main group (= main window) */
	snprintf( transitionPath, MAX_PATH, "scroll_buttons" );
    search_file_name( path, 0, transitionPath, theme_path, 'i' );
	mdlg->main_group = group_create( gui_create_frame( sdl.screen->w - 4, 100 ), 160, 
				load_surf( path, SDL_SWSURFACE, 0, 0, 0, 0 ),
				24, 24, 2, ID_MESSAGE_LIST_UP,
				sdl.screen, 0, 0 );
	if (mdlg->main_group == NULL)
		goto failure;
	sx = group_get_width( mdlg->main_group ) - 30; 
	sy = 1;
	group_add_button( mdlg->main_group, ID_MESSAGE_LIST_UP, sx, sy, 0, 
							tr("Scroll Up"), 2 );
	sy = group_get_height( mdlg->main_group ) - 25;
	group_add_button( mdlg->main_group, ID_MESSAGE_LIST_DOWN, sx, sy, 0, 
							tr("Scroll Down"), 2 );
	
	/* create edit box */
	mdlg->edit_box = edit_create( gui_create_frame( sdl.screen->w - 4, 30 ), 160, gui->font_std, MAX_MESSAGE_LINE,
                                  sdl.screen, 0, 0 );
	if (mdlg->edit_box == NULL)
		goto failure;
	
	return mdlg;
failure:
	fprintf(stderr, tr("Failed to create message dialogue\n"));
	message_dlg_delete(&mdlg);
	return NULL;
}

void message_dlg_delete( MessageDlg **mdlg )
{
	if (*mdlg) {
		MessageDlg *ptr = *mdlg;
		
		group_delete( &ptr->main_group );
		edit_delete( &ptr->edit_box );
		free( ptr );
	}
	*mdlg = NULL;
}

/** Return height of message dialogue @mdlg by adding up all components. */
int message_dlg_get_height(MessageDlg *mdlg)
{
	return group_get_height( mdlg->main_group ) + mdlg->edit_box->label->frame->frame->h;
}

/** Move message dialogue @mdlg to position @px, @py by moving all separate
 * components. */
void message_dlg_move( MessageDlg *mdlg, int px, int py)
{
	int sx = px, sy = py;
	
	group_move( mdlg->main_group, sx, sy );
	sy += group_get_height( mdlg->main_group );
	edit_move(mdlg->edit_box, sx, sy);
}

/* Hide, draw, draw background, get background for message dialogue @mdlg by
 * applying action to all separate components. */
void message_dlg_hide( MessageDlg *mdlg, int value)
{
	group_hide(mdlg->main_group, value);
	edit_hide(mdlg->edit_box, value);
}
void message_dlg_draw( MessageDlg *mdlg)
{
	group_draw(mdlg->main_group);
	edit_draw(mdlg->edit_box);
}
void message_dlg_draw_bkgnd( MessageDlg *mdlg)
{
	group_draw_bkgnd(mdlg->main_group);
	edit_draw_bkgnd(mdlg->edit_box);
}
void message_dlg_get_bkgnd( MessageDlg *mdlg)
{
	group_get_bkgnd(mdlg->main_group);
	edit_get_bkgnd(mdlg->edit_box);
}

/** Handle mouse motion for message dialogue @mdlg by checking all components.
 * @cx, @cy is absolute mouse position in screen. Return 1 if action has been
 * handled by some window, 0 otherwise. */
int message_dlg_handle_motion( MessageDlg *mdlg, int cx, int cy)
{
	int ret = 1;
	
	if (!group_handle_motion(mdlg->main_group,cx,cy))
		ret = 0;
	return ret;
}

/** Handle mouse click to screen position @cx,@cy with button @bid. 
 * Return 1 if a button was clicked that needs upstream processing (e.g., to
 * close dialogue) and set @hbtn then. Otherwise process event internally and
 * return 0. */
int message_dlg_handle_button( MessageDlg *mdlg, int bid, int cx, int cy,
		Button **mbtn )
{
	group_handle_button(mdlg->main_group,bid,cx,cy,mbtn);
        /* handle button/wheel */
    if ( !mdlg->main_group->frame->img->bkgnd->hide )
    {
        int x = 5, y = 5, i;
        if ( ( *mbtn && (*mbtn)->id == ID_MESSAGE_LIST_UP ) || bid == WHEEL_UP ) {
            /* scroll up */
            last_selected_message -= 6;
            if ( last_selected_message < 6 )
                last_selected_message = 6;

            /* draw text */
            SDL_Surface *contents = mdlg->main_group->frame->contents;
            SDL_FillRect( contents, 0, 0x0 );
            gui->font_std->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
            for ( i = last_selected_message - 7; i <= last_selected_message; i++ )
            {
                if ( i >= 0 )
                    write_line( contents, gui->font_std, messages[i], x, &y );
            }
            frame_apply( mdlg->main_group->frame );
            return 0;
        }
        else
            if ( ( *mbtn && (*mbtn)->id == ID_MESSAGE_LIST_DOWN ) || bid == WHEEL_DOWN ) {
                /* scroll down */
                last_selected_message += 6;
                if ( last_selected_message >= messages_count )
                    last_selected_message = messages_count;

                /* draw text */
                SDL_Surface *contents = mdlg->main_group->frame->contents;
                SDL_FillRect( contents, 0, 0x0 );
                gui->font_std->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
                for ( i = last_selected_message - 7; i <= last_selected_message; i++ )
                {
                    if ( i >= 0 )
                        write_line( contents, gui->font_std, messages[i], x, &y );
                }
                frame_apply( mdlg->main_group->frame );
                return 0;
            }
	}
	return 0;
}

/** Reset message dialogue */
void message_dlg_reset( MessageDlg *mdlg )
{
    free( mdlg->edit_box->text );
    mdlg->edit_box->text = calloc( mdlg->edit_box->limit + 1, sizeof( char ) );
    mdlg->edit_box->cursor_pos = 0;
    mdlg->edit_box->cursor_x = mdlg->edit_box->label->frame->img->img->w / 2;
    edit_show( mdlg->edit_box, mdlg->edit_box->text );
}

/** Handle adding new text to message box */
void message_dlg_draw_text( MessageDlg *mdlg )
{
    int x = 5, y = 5, i;
    SDL_Surface *contents = mdlg->main_group->frame->contents;
    SDL_FillRect( contents, 0, 0x0 );
    gui->font_std->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    for ( i = last_selected_message - 7; i <= last_selected_message; i++ )
    {
        if ( i >= 0 )
            write_line( contents, gui->font_std, messages[i], x, &y );
    }
    frame_apply( mdlg->main_group->frame );
}

/** Handle adding new text to message box */
void message_dlg_add_text( MessageDlg *mdlg )
{
    if ( messages_count < MAX_NAME )
    {
        messages_count++;
        /* add message */
        if ( cur_player )
            snprintf( messages[messages_count - 1], MAX_MESSAGE_LINE, "%s: %s", cur_player->name, mdlg->edit_box->text );
        else
            snprintf( messages[messages_count - 1], MAX_MESSAGE_LINE, "nobody: %s", mdlg->edit_box->text );
        /* draw text */
        if ( messages_count - 1 == last_selected_message )
        {
            last_selected_message++;
            message_dlg_draw_text( mdlg );
        }
    }
    else
    {
        int i;
        /* remove last message */
        for ( i = 1; i < MAX_NAME; i++ )
            snprintf( messages[i - 1], MAX_MESSAGE_LINE, "%s", messages[i] );
        /* add message */
        if ( cur_player )
            snprintf( messages[messages_count - 1], MAX_MESSAGE_LINE, "%s: %s", cur_player->name, mdlg->edit_box->text );
        else
            snprintf( messages[messages_count - 1], MAX_MESSAGE_LINE, "nobody: %s", mdlg->edit_box->text );
        /* draw last text */
        if ( messages_count == last_selected_message )
        {
            message_dlg_draw_text( mdlg );
        }
        else
            /* redraw first text */
            if ( last_selected_message == 6 )
            {
                message_dlg_draw_text( mdlg );
            }
    }
}

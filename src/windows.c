/***************************************************************************
                          windows.c  -  description
                             -------------------
    begin                : Tue Mar 21 2002
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

#include "lgeneral.h"
#include "file.h"
#include "event.h"
#include "unit.h"
#include "windows.h"
#include "localize.h"
#include "purchase_dlg.h"
#include "gui.h"

/*
====================================================================
Externals
====================================================================
*/
extern Sdl sdl;
extern int hex_w;
extern int old_mx, old_my;
extern Config config;
extern GUI *gui;

/*
====================================================================
Create a framed label.
A label is hidden and empty per default.
The current draw position is x,y in 'surf'.
====================================================================
*/
Label *label_create( SDL_Surface *frame, int alpha, Font *def_font, SDL_Surface *surf, int x, int y )
{
    Label *label = calloc( 1, sizeof( Label ) );
    if ( ( label->frame = frame_create( frame, alpha, surf, x, y ) ) == 0 ) {
        free( label );
        return 0;
    }
    label->def_font = def_font;
    return label;
}
void label_delete( Label **label )
{
    if ( *label ) {
        frame_delete( &(*label)->frame );
        free( *label ); *label = 0;
    }
}

/*
====================================================================
Write text as label contents and set hide = 0.
If 'font' is NULL label's default font pointer is used.
====================================================================
*/
void label_write( Label *label, Font *_font, const char *text )
{
    Font *font = _font;
    if ( font == 0 ) font = label->def_font;
    font->align = ALIGN_X_CENTER | ALIGN_Y_CENTER;
    SDL_FillRect( label->frame->contents, 0, 0x0 );
    write_text( font, label->frame->contents, label->frame->contents->w >> 1, label->frame->contents->h >> 1, text, 255 );
    frame_apply( label->frame );
    frame_hide( label->frame, 0 );
}


/*
====================================================================
Create button group. At maximum 'limit' buttons may be added.
A buttons tooltip is displayed in corresponding gui->label.
An actual button id is computed as base_id + id.
====================================================================
*/
Group *group_create( SDL_Surface *frame, int alpha, SDL_Surface *img, int w, int h, int limit, int base_id, 
                     SDL_Surface *surf, int x, int y )
{
    Group *group = calloc( 1, sizeof( Group ) );
    if ( ( group->frame = frame_create( frame, alpha, surf, x, y ) ) == 0 ) goto failure;
    if ( img == 0 ) {
        fprintf( stderr, "group_create: passed button surface is NULL: %s\n", SDL_GetError() );
        goto failure;
    }
    SDL_SetColorKey( img, SDL_SRCCOLORKEY, 0x0 );
    group->img = img;
    group->w = w; group->h = h;
    group->base_id = base_id;
    group->button_limit = limit;
    if ( ( group->buttons = calloc( group->button_limit, sizeof( Button ) ) ) == 0 ) {
        fprintf( stderr, tr("Out of memory\n") );
        goto failure;
    }
    return group;
failure:
    group_delete( &group );
    return 0;
}
void group_delete( Group **group )
{
    if ( *group ) {
        frame_delete( &(*group)->frame );
        if ( (*group)->img ) SDL_FreeSurface( (*group)->img );
        if ( (*group)->buttons ) free( (*group)->buttons );
        free( *group ); *group = 0;
    }
}

/*
====================================================================
Add a button at x,y in group. If lock_info is set this button is a switch.
'id' * group::h is the y_offset of the button.
====================================================================
*/
int group_add_button( Group *group, int id, int x, int y, char **lock_info, const char *tooltip, int states )
{
    return group_add_button_complex( group, id, id - group->base_id, 
                                     x, y, lock_info, tooltip, states );
}

/*
====================================================================
Add a button at x,y in group. If lock_info is set this button is a switch.
'icon_id' is used for the button icon instead of 'id' (allows
multiple buttons of the same icon)
====================================================================
*/
int group_add_button_complex( Group *group, int id, int icon_id, int x, int y, char **lock_info, const char *tooltip, int states )
{
    if ( group->button_count == group->button_limit ) {
        fprintf( stderr, tr("This group has reached it's maximum number of buttons.\n") );
        return 0;
    }
    group->buttons[group->button_count].surf_rect.x = x + group->frame->img->bkgnd->surf_rect.x;
    group->buttons[group->button_count].surf_rect.y = y + group->frame->img->bkgnd->surf_rect.y;
    group->buttons[group->button_count].surf_rect.w = group->w;
    group->buttons[group->button_count].surf_rect.h = group->h;
    group->buttons[group->button_count].button_rect.x = 0;
    group->buttons[group->button_count].button_rect.y = icon_id * group->h;
    group->buttons[group->button_count].button_rect.w = group->w;
    group->buttons[group->button_count].button_rect.h = group->h;
    group->buttons[group->button_count].id = id;
    group->buttons[group->button_count].active = 1;
    strcpy_lt( group->buttons[group->button_count].tooltip_center, tooltip, 31 );
    if ( lock_info == 0 )
        group->buttons[group->button_count].lock = 0;
    else
    {
        int i;
        group->buttons[group->button_count].lock = 1;
        group->buttons[group->button_count].lock_info = calloc( states, sizeof( char * ) );
        for ( i = 0; i < states; i++ )
            group->buttons[group->button_count].lock_info[i] = strdup( lock_info[i] );
    }
    group->buttons[group->button_count].states = states;
    group->buttons[group->button_count].hidden = 0;
    group->button_count++;
    return 1;
}

/*
====================================================================
Get button by global id.
====================================================================
*/
Button* group_get_button( Group *group, int id )
{
    int i;
    for ( i = 0; i < group->button_count; i++ )
        if ( group->buttons[i].id == id )
            return &group->buttons[i];
    return 0;
}
/*
====================================================================
Set active state of a button by global id.
====================================================================
*/
void group_set_active( Group *group, int id, int active )
{
    int i;
    for ( i = 0; i < group->button_count; i++ )
        if ( id == group->buttons[i].id ) {
            group->buttons[i].active = active;
            if ( active ) {
               group->buttons[i].button_rect.x = 0;
               group->buttons[i].down = 0;
            }
            else
               group->buttons[i].button_rect.x = (group->buttons[i].states + 1) * group->w;
            break;
        }
}
/*
====================================================================
Set hidden state of a button by global id.
====================================================================
*/
void group_set_hidden( Group *group, int id, int hidden )
{
    int i;
    for ( i = 0; i < group->button_count; i++ )
        if ( id == group->buttons[i].id ) {
            group->buttons[i].hidden = hidden;
            break;
        }
}

/*
====================================================================
Lock button.
====================================================================
*/
void group_lock_button( Group *group, int id, int down )
{
    Button *button = group_get_button( group, id );
    if ( button && button->lock && !button->hidden ) {
        button->down = down;
        if ( !down )
            button->button_rect.x = 0;
        else
            button->button_rect.x = (button->down + 1) * group->w;
    }
}
/*
====================================================================
Check motion event and modify buttons and labels.
Return True if in this frame.
====================================================================
*/
int group_handle_motion( Group *group, int x, int y )
{
    int i;
    if ( !group->frame->img->bkgnd->hide )
    if ( x >= group->frame->img->bkgnd->surf_rect.x && y >= group->frame->img->bkgnd->surf_rect.y )
    if ( x < group->frame->img->bkgnd->surf_rect.x + group->frame->img->bkgnd->surf_rect.w )
    if ( y < group->frame->img->bkgnd->surf_rect.y + group->frame->img->bkgnd->surf_rect.h ) {
        for ( i = 0; i < group->button_count; i++ )
            if ( group->buttons[i].active && !group->buttons[i].hidden ) {
                if ( button_focus( &group->buttons[i], x, y ) ) {
                    if ( !STRCMP(group->buttons[i].tooltip_left, "") )
                        label_write( gui->label_left, 0, group->buttons[i].tooltip_left );
                    if ( !STRCMP(group->buttons[i].tooltip_center, "") )
                        label_write( gui->label_center, 0, group->buttons[i].tooltip_center );
                    if ( !STRCMP(group->buttons[i].tooltip_right, "") )
                        label_write( gui->label_right, 0, group->buttons[i].tooltip_right );
                    if ( group->buttons[i].lock )
                        label_write( gui->label_right, 0, group->buttons[i].lock_info[group->buttons[i].down] );
                    else
                        label_write( gui->label_right, 0, "" );
                    if ( !group->buttons[i].down )
                        group->buttons[i].button_rect.x = group->w;
                }
                else
                    if ( !group->buttons[i].down || !group->buttons[i].lock )
                        group->buttons[i].button_rect.x = 0;
            }
        return 1;
    }
    for ( i = 0; i < group->button_count; i++ )
        if ( group->buttons[i].active && !group->buttons[i].hidden && ( !group->buttons[i].down || !group->buttons[i].lock ) )
            group->buttons[i].button_rect.x = 0;
    return 0;
}
/*
====================================================================
Check click event. Return True if button was clicked and return
the button id.
====================================================================
*/
int group_handle_button( Group *group, int button_id, int x, int y, Button **button )
{
    int i;
    if ( button_id == BUTTON_LEFT ) 
    if ( !group->frame->img->bkgnd->hide )
    if ( x >= group->frame->img->bkgnd->surf_rect.x && y >= group->frame->img->bkgnd->surf_rect.y )
    if ( x < group->frame->img->bkgnd->surf_rect.x + group->frame->img->bkgnd->surf_rect.w )
    if ( y < group->frame->img->bkgnd->surf_rect.y + group->frame->img->bkgnd->surf_rect.h ) {
        for ( i = 0; i < group->button_count; i++ )
            if ( group->buttons[i].active && !group->buttons[i].hidden )
                if ( button_focus( &group->buttons[i], x, y ) ) {
                    *button = &group->buttons[i];
                    if ( (*button)->down < ((*button)->states) - 1 )
                        ((*button)->down)++;
                    else
                        (*button)->down = 0;
                    if ( (*button)->down != 0 )
                        (*button)->button_rect.x = ( (*button)->down + 1 ) * group->w;
                    else
                        (*button)->button_rect.x = 0;
                    if ( group->buttons[i].lock )
                        label_write( gui->label_right, 0, group->buttons[i].lock_info[group->buttons[i].down] );
                    return 1;
                }
    }
    return 0;
}

/*
====================================================================
Draw group.
====================================================================
*/
void group_draw( Group *group )
{
    int i;
    frame_draw( group->frame );
    if ( !group->frame->img->bkgnd->hide )
        for ( i = 0; i < group->button_count; i++ )
            if ( !group->buttons[i].hidden )
                SDL_BlitSurface( group->img, &group->buttons[i].button_rect,
                             group->frame->img->bkgnd->surf, &group->buttons[i].surf_rect );
}

/*
====================================================================
Modify settings.
====================================================================
*/
void group_move( Group *group, int x, int y )
{
    int i;
    int rel_x = x - group->frame->img->bkgnd->surf_rect.x;
    int rel_y = y - group->frame->img->bkgnd->surf_rect.y;
    frame_move( group->frame, x, y );
    for ( i = 0; i < group->button_count; i++ ) {
        group->buttons[i].surf_rect.x += rel_x;
        group->buttons[i].surf_rect.y += rel_y;
    }
}

/*
====================================================================
Return True if x,y is on group button.
====================================================================
*/
int button_focus( Button *button, int x, int y )
{
    if ( x >= button->surf_rect.x && y >= button->surf_rect.y )
        if ( x < button->surf_rect.x + button->surf_rect.w && y < button->surf_rect.y + button->surf_rect.h )
            return 1;
    return 0;
}


/*
====================================================================
Created edit.
====================================================================
*/
Edit *edit_create( SDL_Surface *frame, int alpha, Font *font, int limit, SDL_Surface *surf, int x, int y )
{
    Edit *edit = calloc( 1, sizeof( Edit ) );
    if ( ( edit->label = label_create( frame, alpha, font, surf, x, y ) ) == 0 ) goto failure;
    if ( ( edit->text = calloc( limit + 1, sizeof( char ) ) ) == 0 ) goto failure;
    edit->limit = limit;
    edit->cursor_pos = 0;
    edit->cursor_x = frame->w / 2;
    edit->delay = 0; edit->last_sym = -1;
    return edit;
failure:
    if ( edit->text ) free( edit->text );
    free( edit );
    return 0;
}
void edit_delete( Edit **edit )
{
    if ( *edit ) {
        label_delete( &(*edit)->label );
        if ( (*edit)->text ) free( (*edit)->text );
        free( *edit ); *edit = 0;
    }
}

/*
====================================================================
Draw Edit
====================================================================
*/
void edit_draw( Edit *edit )
{
    SDL_Rect rect;
    label_draw( edit->label );
    if ( !edit->label->frame->img->bkgnd->hide && edit->blink ) {
        /* cursor */
        rect.w = 1; rect.h = edit->label->def_font->height;
        rect.x = edit->label->frame->img->bkgnd->surf_rect.x + edit->cursor_x; 
        rect.y = edit->label->frame->img->bkgnd->surf_rect.y + 
                 ( (edit->label->frame->img->img->h - edit->label->def_font->height ) >> 1 );
        SDL_FillRect( sdl.screen, &rect, 0xffff );
    }
}

/*
====================================================================
Set buffer and adjust cursor x.
====================================================================
*/
void edit_show( Edit *edit, const char *newtext )
{
    int i, w = 0;
    if ( newtext )
        strcpy_lt( edit->text, newtext, edit->limit );
    /* clear last key */
    edit->last_sym = -1;
    /* clear sdl event queue */
    event_clear_sdl_queue();
    /* adjust cursor_x */
    for ( i = 0; i < edit->cursor_pos; i++ )
        w += char_width( edit->label->def_font, edit->text[i] );
    edit->cursor_x = ( ( edit->label->frame->img->img->w - text_width( edit->label->def_font, edit->text ) ) >> 1 ) + w;
    /* show cursor */
    edit->blink = 1;
    edit->blink_time = 0; 
    /* apply */
    label_write( edit->label, 0, edit->text );
}

/*
====================================================================
Modify text buffer according to unicode and keysym.
====================================================================
*/
void edit_handle_key( Edit *edit, int keysym, int modifier, int unicode )
{
    int i, changed = 0;
    switch ( keysym ) {
        case SDLK_RIGHT:
            if ( edit->cursor_pos < strlen( edit->text ) ){
                changed = 1;
                edit->cursor_pos++;
            }
            break;
        case SDLK_LEFT:
            if ( edit->cursor_pos > 0 ) {
                changed = 1;
                edit->cursor_pos--;
            }
            break;
        case SDLK_HOME:
            changed = 1;
            edit->cursor_pos = 0;
            break;
        case SDLK_END:
            changed = 1;
            edit->cursor_pos = strlen( edit->text );
            break;
        case SDLK_BACKSPACE:
            if ( edit->cursor_pos > 0 ) {
                --edit->cursor_pos;
                for ( i = edit->cursor_pos; i < strlen( edit->text ) - 1; i++ )
                    edit->text[i] = edit->text[i + 1];
                edit->text[i] = 0;
                changed = 1;
            }
            break;
        case SDLK_DELETE:
            if ( edit->cursor_pos < strlen( edit->text ) ) {
                for ( i = edit->cursor_pos; i < strlen( edit->text ) - 1; i++ )
                    edit->text[i] = edit->text[i + 1];
                edit->text[i] = 0;
                changed = 1;
            }
            break;
        default:
            /* control commands */
            if ( ( modifier & KMOD_LCTRL ) || ( modifier & KMOD_RCTRL ) ) {
                switch ( keysym ) {
                    case SDLK_u: /* CTRL+U: delete line */
                        edit->text[0] = '\0';
                        edit->cursor_pos = 0;
                        changed = 1;
                        break;
                }
                break;
            }

            if ( unicode >= 32 && edit->cursor_pos < edit->limit && strlen( edit->text ) < edit->limit ) {
                for ( i = edit->limit - 1; i > edit->cursor_pos; i-- )
                    edit->text[i] = edit->text[i - 1];
                edit->text[edit->cursor_pos++] = unicode;
                changed = 1;
            }
        break;
    }
    if ( changed ) {
        edit->delay = keysym != edit->last_sym ? 750 : 100;
        edit_show( edit, 0 );
        edit->last_sym = keysym;
        edit->last_mod = modifier;
        edit->last_uni = unicode;
    }
    else
        edit->last_sym = -1;
}

/*
====================================================================
Update blinking cursor and add keys if keydown. Return True if
key was written.
====================================================================
*/
int edit_update( Edit *edit, int ms )
{
    edit->blink_time += ms;
    if ( edit->blink_time > 500 ) {
        edit->blink_time = 0;
        edit->blink = !edit->blink;
    }
    if ( edit->last_sym != -1 ) {
        if ( !event_check_key( edit->last_sym ) )
            edit->last_sym = -1;
        else {
            edit->delay -= ms;
            if ( edit->delay <= 0 ) {
                edit->delay = 0;
                edit_handle_key( edit, edit->last_sym, edit->last_mod, edit->last_uni );
                return 1;
            }
        }
    }
    return 0;
}


/*
====================================================================
Create a listbox with a number of cells. The item list is
NULL by default.
====================================================================
*/
LBox *lbox_create( SDL_Surface *frame, int alpha, int border, SDL_Surface *buttons, int button_w, int button_h, 
                   int cell_count, int step, int cell_w, int cell_h, int cell_gap, int cell_color,
                   void (*cb)(void*, SDL_Surface*), 
                   SDL_Surface *surf, int x, int y )
{
    int bx, by;
    LBox *lbox = calloc( 1, sizeof( LBox ) );
    /* group */
    if ( ( lbox->group = group_create( frame , alpha, buttons, button_w, button_h, 2, ID_INTERN_UP,
           surf, x, y ) ) == 0 )
        goto failure;
    /* cells */
    lbox->step = step;
    lbox->cell_count = cell_count;
    lbox->cell_w = cell_w;
    lbox->cell_h = cell_h;
    lbox->cell_gap = cell_gap;
    lbox->cell_color = cell_color;
    lbox->cell_x = lbox->cell_y = border;
    lbox->cell_buffer = create_surf( cell_w, cell_h, SDL_SWSURFACE );
    lbox->render_cb = cb;
    /* up/down */
    bx = ( frame->w / 2 - button_w ) / 2;
    by = frame->h - border - button_h;
    group_add_button( lbox->group, ID_INTERN_UP, bx, by, 0, tr("Up"), 2 ); 
    group_add_button( lbox->group, ID_INTERN_DOWN, frame->w - bx - button_w, by, 0, tr("Down"), 2 ); 
    return lbox;
failure:
    free( lbox );
    return 0;
}
void lbox_delete( LBox **lbox )
{
    if ( *lbox ) {
        group_delete( &(*lbox)->group );
        free_surf( &(*lbox)->cell_buffer );
        if ( (*lbox)->items ) list_delete( (*lbox)->items );
        free( *lbox ); *lbox = 0;
    }
}

/*
====================================================================
Rebuild the listbox graphic (lbox->group->frame).
====================================================================
*/
void lbox_apply( LBox *lbox )
{
    int i;
    void *item;
    int sx = lbox->cell_x, sy = lbox->cell_y;
    SDL_Surface *contents = lbox->group->frame->contents;
    SDL_FillRect( contents, 0, 0x0 );
    /* items */
    SDL_FillRect( contents, 0, 0x0 );
    if ( lbox->items ) {
        for ( i = 0; i < lbox->cell_count; i++ ) {
            item = list_get( lbox->items, i + lbox->cell_offset );
            if ( item ) {
                if ( item == lbox->cur_item ) {
                    DEST( contents, sx, sy, lbox->cell_w, lbox->cell_h );
                    fill_surf( lbox->cell_color );
                }
                (lbox->render_cb)( item, lbox->cell_buffer );
                DEST( contents, sx, sy, lbox->cell_w, lbox->cell_h );
                SOURCE( lbox->cell_buffer, 0, 0 );
                blit_surf();
                sy += lbox->cell_h + lbox->cell_gap;
            }
        }
    }
    frame_apply( lbox->group->frame );
}

/*
====================================================================
Delete the old item list (if any) and use the new one (will be 
deleted by this listbox)
====================================================================
*/
void lbox_set_items( LBox *lbox, List *items )
{
    if ( lbox->items )
        list_delete( lbox->items );
    lbox->items = items;
    lbox->cur_item = 0;
    lbox->cell_offset = 0;
    lbox_apply( lbox );
}

/** Select first entry in list and return pointer or NULL if empty. */
void *lbox_select_first_item( LBox *lbox )
{
	if (lbox->items == NULL)
		return NULL;
	lbox->cur_item = list_first(lbox->items);
	lbox->cell_offset = 0;
	lbox_apply(lbox);
	return lbox->cur_item;
}

/*
====================================================================
handle_motion sets button if up/down has focus and returns the item
if any was selected.
====================================================================
*/
int lbox_handle_motion( LBox *lbox, int cx, int cy, void **item )
{
    int i;
    if ( !lbox->group->frame->img->bkgnd->hide ) {
        if ( !group_handle_motion( lbox->group, cx, cy ) ) {
            /* check if above cell */
            if (lbox->items == NULL)
                return 0;
            for ( i = 0; i < lbox->cell_count; i++ )
                if ( FOCUS( cx, cy, 
                            lbox->group->frame->img->bkgnd->surf_rect.x + lbox->cell_x,
                            lbox->group->frame->img->bkgnd->surf_rect.y + lbox->cell_y + i * ( lbox->cell_h + lbox->cell_gap ),
                            lbox->cell_w, lbox->cell_y ) ) {
                    *item = list_get( lbox->items, lbox->cell_offset + i );
                    return 1;
                }
        }
        else
            return 1;
    }
    return 0;
}
/*
====================================================================
handle_click internally handles up/down click and returns the item
if one was selected.
Note: If item is clicked, @button is not changed, but @item is set.
Thus if true is returned, @item has to be checked (@item == NULL,
button was clicked and @button is set; @item not NULL, item was
clicked and @button may be undefined).
====================================================================
*/
int lbox_handle_button( LBox *lbox, int button_id, int cx, int cy, Button **button, void **item )
{
    int i;
	
    *item = 0;

    /* check focus, if not in listbox nothing to do */	
    if ( !FOCUS( cx, cy, lbox->group->frame->img->bkgnd->surf_rect.x,
               lbox->group->frame->img->bkgnd->surf_rect.y,
               group_get_width(lbox->group), group_get_height(lbox->group)) )
        return 0;

    /* see if up/down button was hit */	
    group_handle_button( lbox->group, button_id, cx, cy, button );

    /* handle button/wheel or selection if not hidden */
    if ( !lbox->group->frame->img->bkgnd->hide ) {
    if ( ( *button && (*button)->id == ID_INTERN_UP ) || button_id == WHEEL_UP ) {
        /* scroll up */
        lbox->cell_offset -= lbox->step;
        if ( lbox->cell_offset < 0 )
            lbox->cell_offset = 0;
            lbox_apply( lbox );
        return 1;
    }
    else
        if ( ( *button && (*button)->id == ID_INTERN_DOWN ) || button_id == WHEEL_DOWN ) {
            /* scroll down */
            if ( lbox->items == NULL || lbox->cell_count >= lbox->items->count )
                lbox->cell_offset = 0;
            else {
                lbox->cell_offset += lbox->step;
                if ( lbox->cell_offset + lbox->cell_count >= lbox->items->count )
                    lbox->cell_offset = lbox->items->count - lbox->cell_count;
            }
            lbox_apply( lbox );
            return 1;
        }
        else 
            if ( lbox->items ) 
                for ( i = 0; i < lbox->cell_count; i++ )
                    /* check if above cell */
                    if ( FOCUS( cx, cy, 
                                lbox->group->frame->img->bkgnd->surf_rect.x + lbox->cell_x,
                                lbox->group->frame->img->bkgnd->surf_rect.y + lbox->cell_y + i * ( lbox->cell_h + lbox->cell_gap ),
                                lbox->cell_w, lbox->cell_h ) ) {
                        lbox->cur_item = list_get( lbox->items, lbox->cell_offset + i );
                        lbox_apply( lbox );
                        *item = lbox->cur_item;
                        return 1;
                    }
    }
    return 0;
}

/** Render listbox item @item (which is a simple string) to listbox cell 
 * surface @buffer. */
void lbox_render_text( void *item, SDL_Surface *buffer )
{
	const char *str = (const char*)item;
	
	SDL_FillRect( buffer, 0, 0x0 );
	gui->font_std->align = ALIGN_X_LEFT | ALIGN_Y_CENTER;
	write_text( gui->font_std, buffer, 4, buffer->h >> 1, str, 255 );
}

/*
====================================================================
Create a file dialogue. The listbox is empty as default.
The first two buttons in conf_buttons are added as ok and cancel.
For all other buttons found in conf_buttons there is spaces
reserved and this buttons may be added by fdlg_add_button().
====================================================================
*/
FDlg *fdlg_create( 
                   SDL_Surface *lbox_frame, int alpha, int border,
                   SDL_Surface *lbox_buttons, int lbox_button_w, int lbox_button_h,
                   int cell_h,
                   SDL_Surface *conf_frame,
                   SDL_Surface *conf_buttons, int conf_button_w, int conf_button_h,
                   int id_ok, 
                   void (*lbox_cb)( void*, SDL_Surface* ),
                   void (*file_cb)( const char*, const char*, SDL_Surface* ),
                   SDL_Surface *surf, int x, int y, int arrangement, int emptyFile, int dir_only, int file_type )
{
    int info_w, info_h, button_count;
    int cell_count, cell_w;
    FDlg *dlg = calloc( 1, sizeof( FDlg ) );
    /* listbox */
    cell_w = lbox_frame->w - 2 * border;
    cell_count = ( lbox_frame->h - 2 * border - lbox_button_h ) / ( cell_h + 1 );
    if ( ( dlg->lbox = lbox_create( lbox_frame, alpha, border,
                                    lbox_buttons, lbox_button_w, lbox_button_h, 
                                    cell_count, 4, cell_w, cell_h, 1, 0x0000ff, 
                                    lbox_cb, surf, x, y ) ) == 0 )
        goto failure;
    /* frame */
    button_count = conf_buttons->h / conf_button_h;
    switch ( arrangement )
    {
        case ARRANGE_COLUMNS:
            if ( ( dlg->group = group_create( conf_frame, alpha, conf_buttons, conf_button_w, conf_button_h, button_count,
                                              id_ok, surf, x + lbox_frame->w, y ) ) == 0 )
                goto failure;
            break;
        case ARRANGE_ROWS:
            if ( ( dlg->group = group_create( conf_frame, alpha, conf_buttons, conf_button_w, conf_button_h, button_count,
                                              id_ok, surf, x, y + lbox_frame->h ) ) == 0 )
                goto failure;
            break;
        case ARRANGE_INSIDE:
            break;
    }
    /* buttons */
    dlg->button_y = conf_frame->h - border - conf_button_h;
    dlg->button_x = conf_frame->w;
    dlg->button_dist = 10 + conf_button_w;
    group_add_button( dlg->group, id_ok, dlg->button_x - dlg->button_dist * 2, dlg->button_y, 0, tr("Ok"), 2 );
    group_add_button( dlg->group, id_ok + 1, dlg->button_x - dlg->button_dist, dlg->button_y, 0, tr("Cancel"), 2 );
    /* file callback */
    dlg->file_cb = file_cb;
    /* path */
    switch ( arrangement )
    {
        case ARRANGE_COLUMNS:
            info_w = conf_frame->w - 2 * border;
            info_h = conf_frame->h - 3 * border - conf_button_h;
            strcpy( dlg->root, "/" );
            break;
        case ARRANGE_ROWS:
            info_w = conf_frame->w - 2 * border;
            info_h = conf_frame->h;
            strcpy( dlg->root, "/pg/Save" );
            break;
        case ARRANGE_INSIDE:
            break;
    }
    /* info region */
    dlg->info_x = border;
    dlg->info_y = border;
    dlg->info_buffer = create_surf( info_w, info_h, SDL_SWSURFACE );
    dlg->current_name = 0;
    dlg->subdir[0] = 0;
    dlg->arrangement = arrangement;
    dlg->emptyFile = emptyFile;
    dlg->dir_only = dir_only;
    dlg->file_type = file_type;
    return dlg;
failure:
    fdlg_delete( &dlg );
    return 0;
}
void fdlg_delete( FDlg **fdlg )
{
    if ( *fdlg ) {
        group_delete( &(*fdlg)->group );
        lbox_delete( &(*fdlg)->lbox );
        free_surf( &(*fdlg)->info_buffer );
        free( *fdlg ); *fdlg = 0;
    }
}

/*
====================================================================
Draw file dialogue
====================================================================
*/
void fdlg_hide( FDlg *fdlg, int hide )
{
    buffer_hide( fdlg->lbox->group->frame->img->bkgnd, hide );
    buffer_hide( fdlg->group->frame->img->bkgnd, hide );
}
void fdlg_get_bkgnd( FDlg *fdlg )
{
    buffer_get( fdlg->lbox->group->frame->img->bkgnd );
    buffer_get( fdlg->group->frame->img->bkgnd );
}
void fdlg_draw_bkgnd( FDlg *fdlg )
{
    buffer_draw( fdlg->lbox->group->frame->img->bkgnd );
    buffer_draw( fdlg->group->frame->img->bkgnd );
}
void fdlg_draw( FDlg *fdlg )
{
    group_draw( fdlg->lbox->group );
    group_draw( fdlg->group );
}

/*
====================================================================
Modify file dialogue settings
====================================================================
*/
void fdlg_set_surface( FDlg *fdlg, SDL_Surface *surf )
{
    group_set_surface( fdlg->lbox->group, surf );
    group_set_surface( fdlg->group, surf );
}

void fdlg_move( FDlg *fdlg, int x, int y )
{
    group_move( fdlg->lbox->group, x, y );
    switch ( fdlg->arrangement )
    {
        case ARRANGE_COLUMNS:
            group_move( fdlg->group, x + fdlg->lbox->group->frame->img->img->w - 1, y );
            break;
        case ARRANGE_ROWS:
            group_move( fdlg->group, x, y + fdlg->lbox->group->frame->img->img->h - 1 );
            break;
        case ARRANGE_INSIDE:
            break;
    }
}

/*
====================================================================
Add button. Graphic is taken from conf_buttons.
====================================================================
*/
void fdlg_add_button( FDlg *fdlg, int id, char **lock_info, const char *tooltip )
{
    int x = fdlg->button_x - ( id - fdlg->group->base_id + 1 ) * fdlg->button_dist;
    group_add_button( fdlg->group, id, x, fdlg->button_y, lock_info, tooltip, 2 );
}

/*
====================================================================
Show file dialogue at directory root.
====================================================================
*/
void fdlg_open( FDlg *fdlg, const char *root, const char *subdir )
{
    char path[MAX_PATH];
    strcpy( fdlg->root, root );
    if ( strcmp( subdir, "" ) == 0 )
    {
        fdlg->subdir[0] = 0;
        snprintf( path, MAX_PATH, "%s", root );
    }
    else
    {
        strcpy( fdlg->subdir, subdir );
        snprintf( path, MAX_PATH, "%s/%s", root, subdir );
    }
    lbox_set_items( fdlg->lbox, dir_get_entries( path, root, fdlg->file_type, fdlg->emptyFile, fdlg->dir_only ) );
                    (fdlg->file_cb)( 0, 0, fdlg->info_buffer );
    
    SDL_FillRect( fdlg->group->frame->contents, 0, 0x0 );
    frame_apply( fdlg->group->frame );
    fdlg_hide( fdlg, 0 );
}

/*
====================================================================
handle_motion updates the focus of the buttons
====================================================================
*/
int fdlg_handle_motion( FDlg *fdlg, int cx, int cy )
{
    void *item;
    if ( !fdlg->lbox->group->frame->img->bkgnd->hide ) {
        if ( !lbox_handle_motion( fdlg->lbox, cx, cy, &item ) )
        if ( !group_handle_motion( fdlg->group, cx, cy ) )
            return 0;
        return 1;
    }
    return 0;
}
/*
====================================================================
handle_click 
====================================================================
*/
int fdlg_handle_button( FDlg *fdlg, int button_id, int cx, int cy, Button **button )
{
    char path[MAX_PATH];
    void *item = 0;
    char *fname;
    Name_Entry_Type *name_entry;
    if ( !fdlg->lbox->group->frame->img->bkgnd->hide ) {
        if ( group_handle_button( fdlg->group, button_id, cx, cy, button ) )
            return 1;
        if ( lbox_handle_button( fdlg->lbox, button_id, cx, cy, button, &item ) ) {
            if ( item ) {
                SDL_FillRect( fdlg->group->frame->contents, 0, 0x0 );
                if ( fdlg->file_type == LIST_ALL )
                {
                    /* no file name switching */
                    fname = (char*)item;
                    if ( ( fname[0] == '*' ) && ( !fdlg->dir_only ) ) {
                        /* switch directory */
                        if ( fname[1] == '.' ) {
                            /* one up */
                            if ( strrchr( fdlg->subdir, '/' ) )
                                (strrchr( fdlg->subdir, '/' ))[0] = 0;
                            else
                                fdlg->subdir[0] = 0;
                        }
                        else {
                            if ( fdlg->subdir[0] != 0 )
                                strcat( fdlg->subdir, "/" );
                            strcat( fdlg->subdir, fname + 1 );
                        }
                        if ( fdlg->subdir[0] == 0 )
                            strcpy( path, fdlg->root );
                        else
                            sprintf( path, "%s/%s", fdlg->root, fdlg->subdir );
                        lbox_set_items( fdlg->lbox, dir_get_entries( path, fdlg->root, fdlg->file_type, fdlg->emptyFile, fdlg->dir_only ) );
                        (fdlg->file_cb)( 0, 0, fdlg->info_buffer );
                    }
                    else {
                        /* file info */
                        if ( fdlg->subdir[0] == 0 )
                            strcpy( path, fname );
                        else
                            sprintf( path, "%s/%s", fdlg->subdir, fname );
                        fdlg->current_name = fname;
                        (fdlg->file_cb)( path, 0, fdlg->info_buffer );
                    }
                }
                else
                {
                    /* file name must be extracted from structure */
                    name_entry = (Name_Entry_Type*)item;
                    if ( ( name_entry->internal_name[0] == '*' ) && ( !fdlg->dir_only ) ) {
                        /* switch directory */
                        if ( name_entry->internal_name[1] == '.' ) {
                            /* one up */
                            if ( strrchr( fdlg->subdir, '/' ) )
                                (strrchr( fdlg->subdir, '/' ))[0] = 0;
                            else
                                fdlg->subdir[0] = 0;
                        }
                        else {
                            if ( fdlg->subdir[0] != 0 )
                                strcat( fdlg->subdir, "/" );
                            strcat( fdlg->subdir, name_entry->internal_name + 1 );
                        }
                        if ( fdlg->subdir[0] == 0 )
                            strcpy( path, fdlg->root );
                        else
                            sprintf( path, "%s/%s", fdlg->root, fdlg->subdir );
                        lbox_set_items( fdlg->lbox, dir_get_entries( path, fdlg->root, fdlg->file_type, fdlg->emptyFile, fdlg->dir_only ) );
                        (fdlg->file_cb)( 0, 0, fdlg->info_buffer );
                    }
                    else {
                        /* file info */
                        if ( fdlg->subdir[0] == 0 )
                            strcpy( path, name_entry->file_name );
                        else
                            sprintf( path, "%s/%s", fdlg->subdir, name_entry->file_name );
                        fdlg->current_name = name_entry->file_name;
                        (fdlg->file_cb)( path, name_entry->internal_name, fdlg->info_buffer );
                    }
                }
                DEST( fdlg->group->frame->contents, fdlg->info_x, fdlg->info_y, fdlg->info_buffer->w, fdlg->info_buffer->h );
                SOURCE( fdlg->info_buffer, 0, 0 );
                blit_surf();
                frame_apply( fdlg->group->frame );
            }
        }
        return 0;
    }
    return 0;
}


/*
====================================================================
Create scenario setup dialogue.
====================================================================
*/
SDlg *sdlg_create( SDL_Surface *list_frame, SDL_Surface *list_buttons,
                   int list_button_w, int list_button_h, int cell_h,
                   SDL_Surface *ctrl_frame, SDL_Surface *ctrl_buttons,
                   int ctrl_button_w, int ctrl_button_h, int id_ctrl,
                   SDL_Surface *mod_frame, SDL_Surface *mod_buttons,
                   int mod_button_w, int mod_button_h, int id_mod,
                   SDL_Surface *conf_frame, SDL_Surface *conf_buttons,
                   int conf_button_w, int conf_button_h, int id_conf,
                   void (*list_render_cb)(void*,SDL_Surface*),
                   void (*list_select_cb)(void*),
                   SDL_Surface *surf, int x, int y )
{
    int border = 10, alpha = 160, px, py;
    int cell_w = list_frame->w - 2 * border;
    int cell_count = ( list_frame->h - 2 * border ) / ( cell_h + 1 );
    char **button_temp;

    SDlg *sdlg = calloc( 1, sizeof( SDlg ) );

    /* listbox for players */
    if ( ( sdlg->list = lbox_create( list_frame, alpha, border, list_buttons, list_button_w, list_button_h,
                                     cell_count, 2, cell_w, cell_h, 1, 0x0000ff, list_render_cb, surf, x, y ) ) == 0 )
        goto failure;

    /* group with human/cpu control button */
    if ( ( sdlg->ctrl = group_create( ctrl_frame, alpha, ctrl_buttons, 
                                      ctrl_button_w, ctrl_button_h, 1, id_ctrl, surf, x + list_frame->w - 1, y ) ) == 0 )
        goto failure;
    group_add_button( sdlg->ctrl, id_ctrl, ctrl_frame->w - border - ctrl_button_w, ( ctrl_frame->h - ctrl_button_h ) / 2, 0, tr("Switch Control"), 2 );

    /* group with ai module select button */
    if ( ( sdlg->module = group_create( mod_frame, alpha, mod_buttons, 
                                        mod_button_w, mod_button_h, 1, id_mod, surf, 
                                        x + list_frame->w - 1, y + ctrl_frame->h ) ) == 0 )
        goto failure;
    group_add_button( sdlg->module, id_mod, ctrl_frame->w - border - ctrl_button_w, ( mod_frame->h - mod_button_h ) / 2, 0, tr("Select AI Module"), 2 );
#ifndef USE_DL
    group_set_active( sdlg->module, id_mod, 0 );
#endif

    /* group with settings and confirm buttons; id_conf is id of first button
     * in image conf_buttons */
    if ( ( sdlg->confirm = group_create( conf_frame, alpha, conf_buttons, 
                                         conf_button_w, conf_button_h, 7, id_conf, surf, 
                                         x + list_frame->w - 1, y + ctrl_frame->h + mod_frame->h ) ) == 0 )
        goto failure;
    px = conf_frame->w - (border + conf_button_w);
    py = (conf_frame->h - conf_button_h) / 2;
    group_add_button( sdlg->confirm, ID_SCEN_SETUP_OK, px, py, 0, tr("Ok"), 2 );
    px = border;
    button_temp = calloc( 2, sizeof( char * ) );
    button_temp[0] = strdup( "Fog: OFF" );
    button_temp[1] = strdup( "Fog: ON" );
    group_add_button( sdlg->confirm, ID_SCEN_SETUP_FOG, px, py, button_temp, tr("Fog Of War"), 2 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp );
    button_temp = calloc( 2, sizeof( char * ) );
    button_temp[0] = strdup( "Supply: OFF" );
    button_temp[1] = strdup( "Supply: ON" );
    group_add_button( sdlg->confirm, ID_SCEN_SETUP_SUPPLY, px, py, button_temp, tr("Unit Supply"), 2 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp );
    button_temp = calloc( 3, sizeof( char * ) );
    button_temp[0] = strdup( "Weather: OFF" );
    button_temp[1] = strdup( "Weather: FIXED" );
    button_temp[2] = strdup( "Weather: RANDOM" );
    group_add_button( sdlg->confirm, ID_SCEN_SETUP_WEATHER, px, py, button_temp, tr("Weather Influence"), 3 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp[2] );
    free( button_temp );
    button_temp = calloc( 2, sizeof( char * ) );
    button_temp[0] = strdup( "Redeploy Turn: OFF" );
    button_temp[1] = strdup( "Redeploy Turn: ON" );
    group_add_button( sdlg->confirm, ID_SCEN_SETUP_DEPLOYTURN, px, py, button_temp, tr("Deploy Turn"), 2 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp );
    button_temp = calloc( 3, sizeof( char * ) );
    button_temp[0] = strdup( "Purchase: OFF" );
    button_temp[1] = strdup( "Purchase: 1 Turn Delay" );
    button_temp[2] = strdup( "Purchase: 0 Delay Inactive" );
    group_add_button( sdlg->confirm, ID_SCEN_SETUP_PURCHASE, px, py, button_temp, tr("Purchase Option"), 3 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp[2] );
    free( button_temp );
    button_temp = calloc( 2, sizeof( char * ) );
    button_temp[0] = strdup( "Merge" );
    button_temp[1] = strdup( "Replacements" );
    group_add_button( sdlg->confirm, ID_SCEN_SETUP_MERGE_REPLACEMENTS, px, py, button_temp, tr("Merge/Replacements Option"), 2 );
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp );
    group_lock_button( sdlg->confirm, ID_SCEN_SETUP_FOG, config.fog_of_war );
    group_lock_button( sdlg->confirm, ID_SCEN_SETUP_SUPPLY, config.supply );
    group_lock_button( sdlg->confirm, ID_SCEN_SETUP_WEATHER, config.weather );
    group_lock_button( sdlg->confirm, ID_SCEN_SETUP_DEPLOYTURN, config.deploy_turn );
    group_lock_button( sdlg->confirm, ID_SCEN_SETUP_PURCHASE, config.purchase );
    group_lock_button( sdlg->confirm, ID_SCEN_SETUP_MERGE_REPLACEMENTS, config.merge_replacements );
    sdlg->select_cb = list_select_cb;
    return sdlg;
failure:
    sdlg_delete( &sdlg );
    return 0;
}
/*
====================================================================
Create campaign setup dialogue.
====================================================================
*/
SDlg *sdlg_camp_create( SDL_Surface *conf_frame, SDL_Surface *conf_buttons,
                   int conf_button_w, int conf_button_h, int id_conf,
                   void (*list_render_cb)(void*,SDL_Surface*),
                   void (*list_select_cb)(void*),
                   SDL_Surface *surf, int x, int y )
{
    int border = 10, alpha = 160, px, py;
    char **button_temp;

    SDlg *sdlg = calloc( 1, sizeof( SDlg ) );

    /* set unused surfaces as 0 */
    sdlg->list = 0;
    sdlg->ctrl = 0;
    sdlg->module = 0;

    /* group with settings and confirm buttons; id_conf is id of first button
     * in image conf_buttons */
    if ( ( sdlg->confirm = group_create( conf_frame, alpha, conf_buttons, 
                                         conf_button_w, conf_button_h, 5, id_conf, surf, 
                                         x - 1, y ) ) == 0 )
        goto failure;
    px = conf_frame->w - (border + conf_button_w);
    py = (conf_frame->h - conf_button_h) / 2;
    group_add_button( sdlg->confirm, ID_CAMP_SETUP_OK, px, py, 0, tr("Ok"), 2 );
    px = border;
    button_temp = calloc( 2, sizeof( char * ) );
    button_temp[0] = strdup( "Merge" );
    button_temp[1] = strdup( "Replacements" );
    group_add_button( sdlg->confirm, ID_CAMP_SETUP_MERGE_REPLACEMENTS, px, py, button_temp, tr("Merge/Replacements Option"), 2 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp );
    button_temp = calloc( 2, sizeof( char * ) );
    button_temp[0] = strdup( "Core Units: OFF" );
    button_temp[1] = strdup( "Core Units: ON" );
    group_add_button( sdlg->confirm, ID_CAMP_SETUP_CORE, px, py, button_temp, tr("Core Units Option"), 2 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp );
    button_temp = calloc( 2, sizeof( char * ) );
    button_temp[0] = strdup( "Purchase: 1 Turn Delay" );
    button_temp[1] = strdup( "Purchase: 0 Delay Inactive" );
    group_add_button( sdlg->confirm, ID_CAMP_SETUP_PURCHASE, px, py, button_temp, tr("Purchase Option"), 2 );
    px += border + conf_button_w;
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp );
    button_temp = calloc( 3, sizeof( char * ) );
    button_temp[0] = strdup( "Weather: OFF" );
    button_temp[1] = strdup( "Weather: FIXED" );
    button_temp[2] = strdup( "Weather: RANDOM" );
    group_add_button( sdlg->confirm, ID_CAMP_SETUP_WEATHER, px, py, button_temp, tr("Weather Influence"), 3 );
    free( button_temp[0] );
    free( button_temp[1] );
    free( button_temp[2] );
    free( button_temp );
    group_lock_button( sdlg->confirm, ID_CAMP_SETUP_MERGE_REPLACEMENTS, config.merge_replacements );
    group_lock_button( sdlg->confirm, ID_CAMP_SETUP_CORE, config.use_core_units );
    group_lock_button( sdlg->confirm, ID_CAMP_SETUP_PURCHASE, config.campaign_purchase );
    group_lock_button( sdlg->confirm, ID_CAMP_SETUP_WEATHER, config.weather );
    sdlg->select_cb = list_select_cb;
    return sdlg;
failure:
    sdlg_delete( &sdlg );
    return 0;
}
void sdlg_delete( SDlg **sdlg )
{
    if ( *sdlg ) {
        lbox_delete( &(*sdlg)->list );
        group_delete( &(*sdlg)->ctrl );
        group_delete( &(*sdlg)->module );
        group_delete( &(*sdlg)->confirm );
        free( *sdlg ); *sdlg = 0;
    }
}

/*
====================================================================
Draw setup dialogue.
====================================================================
*/
void sdlg_hide( SDlg *sdlg, int hide )
{
    if ( sdlg->list )
        lbox_hide( sdlg->list, hide );
    if ( sdlg->ctrl )
        group_hide( sdlg->ctrl, hide );
    if ( sdlg->module )
        group_hide( sdlg->module, hide );
    group_hide( sdlg->confirm, hide );
}
void sdlg_get_bkgnd( SDlg *sdlg )
{
    if ( sdlg->list )
        lbox_get_bkgnd( sdlg->list );
    if ( sdlg->ctrl )
        group_get_bkgnd( sdlg->ctrl );
    if ( sdlg->module )
        group_get_bkgnd( sdlg->module );
    group_get_bkgnd( sdlg->confirm );
}
void sdlg_draw_bkgnd( SDlg *sdlg )
{
    if ( sdlg->list )
        lbox_draw_bkgnd( sdlg->list );
    if ( sdlg->ctrl )
        group_draw_bkgnd( sdlg->ctrl );
    if ( sdlg->module )
        group_draw_bkgnd( sdlg->module );
    group_draw_bkgnd( sdlg->confirm );
}
void sdlg_draw( SDlg *sdlg )
{
    if ( sdlg->list )
        lbox_draw( sdlg->list );
    if ( sdlg->ctrl )
        group_draw( sdlg->ctrl );
    if ( sdlg->module )
        group_draw( sdlg->module );
    group_draw( sdlg->confirm );
}

/*
====================================================================
Scenario setup dialogue
====================================================================
*/
void sdlg_set_surface( SDlg *sdlg, SDL_Surface *surf )
{
    lbox_set_surface( sdlg->list, surf );
    group_set_surface( sdlg->ctrl, surf );
    group_set_surface( sdlg->module, surf );
    group_set_surface( sdlg->confirm, surf );
}
void sdlg_move( SDlg *sdlg, int x, int y )
{
    if ( sdlg->list )
    {
        lbox_move( sdlg->list, x, y );
        group_move( sdlg->confirm, x + sdlg->list->group->frame->img->img->w - 1, 
                sdlg->ctrl->frame->img->img->h + sdlg->module->frame->img->img->h + y );
    }
    else
    {
        group_move( sdlg->confirm, x - 1, y );
    }
    if ( sdlg->ctrl )
        group_move( sdlg->ctrl, x + sdlg->list->group->frame->img->img->w - 1, y );
    if ( sdlg->module )
        group_move( sdlg->module, x + sdlg->list->group->frame->img->img->w - 1, 
                sdlg->ctrl->frame->img->img->h + y );
}

/*
====================================================================
handle_motion updates the focus of the buttons
====================================================================
*/
int sdlg_handle_motion( SDlg *sdlg, int cx, int cy )
{
    void *item;
    if ( !sdlg->confirm->frame->img->bkgnd->hide ) {
        if ( sdlg->ctrl )
        {
            if ( !group_handle_motion( sdlg->ctrl, cx, cy ) )
            if ( !group_handle_motion( sdlg->module, cx, cy ) )
            if ( !group_handle_motion( sdlg->confirm, cx, cy ) )
            if ( !lbox_handle_motion( sdlg->list, cx, cy, &item ) )
                return 0;
        }
        else
        {
            if ( !group_handle_motion( sdlg->confirm, cx, cy ) )
                return 0;
        }
        return 1;
    }
    return 0;
}
/*
====================================================================
handle_button 
====================================================================
*/
int sdlg_handle_button( SDlg *sdlg, int button_id, int cx, int cy, Button **button )
{
    void *item = 0;
    if ( !sdlg->confirm->frame->img->bkgnd->hide ) {
        if ( sdlg->ctrl )
            if ( group_handle_button( sdlg->ctrl, button_id, cx, cy, button ) )
                return 1;
        if ( sdlg->module )
            if ( group_handle_button( sdlg->module, button_id, cx, cy, button ) )
                return 1;
        if ( group_handle_button( sdlg->confirm, button_id, cx, cy, button ) )
            return 1;
        if ( sdlg->list )
            if ( lbox_handle_button( sdlg->list, button_id, cx, cy, button, &item ) ) {
                if ( item ) {
                    (sdlg->select_cb)(item);
                }
        }
    }
    return 0;
}


/** Select dialog:
 * A listbox for selection with OK/Cancel buttons */

SelectDlg *select_dlg_create(
	SDL_Surface *lbox_frame, 
	SDL_Surface *lbox_buttons, int lbox_button_w, int lbox_button_h, 
	int lbox_cell_count, int lbox_cell_w, int lbox_cell_h,
	void (*lbox_render_cb)(void*, SDL_Surface*),
	SDL_Surface *conf_frame,
	SDL_Surface *conf_buttons, int conf_button_w, int conf_button_h,
	int id_ok, 
	SDL_Surface *surf, int x, int y )
{
	SelectDlg *sdlg = NULL;
	int sx, sy;
	
	sdlg = calloc(1, sizeof(SelectDlg));
	if (sdlg == NULL)
		goto failure;
	
	sdlg->button_group = group_create(conf_frame, 160, conf_buttons,
				conf_button_w, conf_button_h, 2, id_ok, sdl.screen, 0, 0);
	if (sdlg->button_group == NULL)
		goto failure;
	sx = group_get_width( sdlg->button_group ) - 60; 
	sy = 5;
	group_add_button( sdlg->button_group, id_ok, sx, sy, 0, tr("Apply"), 2 );
	group_add_button( sdlg->button_group, id_ok+1, sx + 30, sy, 0, 
							tr("Cancel"), 2 );
	
	sdlg->select_lbox = lbox_create( lbox_frame, 160, 6, 
			lbox_buttons, lbox_button_w, lbox_button_h,
			lbox_cell_count, lbox_cell_count/2, 
			lbox_cell_w, lbox_cell_h, 
			1, 0x0000ff, lbox_render_cb, sdl.screen, 0, 0);
	if (sdlg->select_lbox == NULL)
		goto failure;
	
	return sdlg;
failure:
	fprintf(stderr, tr("Failed to create select dialogue\n"));
	select_dlg_delete(&sdlg);
	return NULL;
}
void select_dlg_delete( SelectDlg **sdlg )
{
	if (*sdlg) {
		SelectDlg *ptr = *sdlg;
		group_delete(&ptr->button_group);
		lbox_delete(&ptr->select_lbox);
		free(ptr);
	}
	*sdlg = NULL;
}

int select_dlg_get_width(SelectDlg *sdlg)
{
	return lbox_get_width(sdlg->select_lbox);
}
int select_dlg_get_height(SelectDlg *sdlg)
{
	return lbox_get_height(sdlg->select_lbox) + 
				group_get_height(sdlg->button_group);
}

void select_dlg_move( SelectDlg *sdlg, int px, int py)
{
	lbox_move(sdlg->select_lbox, px, py);
	group_move(sdlg->button_group, px, py + 
			lbox_get_height(sdlg->select_lbox));
}

void select_dlg_hide( SelectDlg *sdlg, int value)
{
	group_hide(sdlg->button_group, value);
	lbox_hide(sdlg->select_lbox, value);
}
void select_dlg_draw( SelectDlg *sdlg)
{
	group_draw(sdlg->button_group);
	lbox_draw(sdlg->select_lbox);
}
void select_dlg_draw_bkgnd( SelectDlg *sdlg)
{
	group_draw_bkgnd(sdlg->button_group);
	lbox_draw_bkgnd(sdlg->select_lbox);
}
void select_dlg_get_bkgnd( SelectDlg *sdlg)
{
	group_get_bkgnd(sdlg->button_group);
	lbox_get_bkgnd(sdlg->select_lbox);
}

int select_dlg_handle_motion( SelectDlg *sdlg, int cx, int cy)
{
	int ret = 1;
	void *item = NULL;
	
	if (!group_handle_motion(sdlg->button_group,cx,cy))
	if (!lbox_handle_motion(sdlg->select_lbox,cx,cy,&item))
		ret = 0;
	return ret;
	
}

int select_dlg_handle_button( SelectDlg *sdlg, int bid, int cx, int cy, 
	Button **pbtn )
{
	void *item = NULL;
	
	if (group_handle_button(sdlg->button_group,bid,cx,cy,pbtn))
		return 1;
	if (lbox_handle_button(sdlg->select_lbox,bid,cx,cy,pbtn,&item))
		return 0; /* internal up/down buttons */
	return 0;
}

/***************************************************************************
                          windows.h  -  description
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

#ifndef __WINDOWS_H
#define __WINDOWS_H

#include "image.h"

enum {
    ARRANGE_COLUMNS = 0,
    ARRANGE_ROWS,
    ARRANGE_INSIDE
};

/*
====================================================================
Label. A frame with a text drawn to it.
====================================================================
*/
typedef struct {
    Frame *frame;
    Font  *def_font;
} Label;

/*
====================================================================
Create a framed label.
A label is hidden and empty per default.
The current draw position is x,y in 'surf'.
====================================================================
*/
Label *label_create( SDL_Surface *frame, int alpha, Font *def_font, SDL_Surface *surf, int x, int y );
void label_delete( Label **label );

/*
====================================================================
Draw label
====================================================================
*/
#define label_hide( label, hide ) buffer_hide( label->frame->img->bkgnd, hide )
#define label_get_bkgnd( label ) buffer_get( label->frame->img->bkgnd )
#define label_draw_bkgnd( label ) buffer_draw( label->frame->img->bkgnd )
#define label_draw( label ) frame_draw( label->frame )

/*
====================================================================
Modify label settings
====================================================================
*/
#define label_move( label, x, y ) buffer_move( label->frame->img->bkgnd, x, y )
#define label_set_surface( label, surf ) buffer_set_surface( label->frame->img->bkgnd, surf )

/*
====================================================================
Write text as label contents and set hide = 0.
If 'font' is NULL label's default font pointer is used.
====================================================================
*/
void label_write( Label *label, Font *font, const char *text );

/*
====================================================================
Button group. A frame with some click buttons.
====================================================================
*/
typedef struct {
    SDL_Rect button_rect;   /* region in button surface */
    SDL_Rect surf_rect;     /* region in buffer::surface */ 
    int id;                 /* if returned on click */
    char tooltip_left[32];  /* displayed as short help */
    char tooltip_center[32];/* displayed as short help */
    char tooltip_right[32]; /* displayed as short help */
    int active;             /* true if button may be used */
    int down;               /* consecutive numbers are different state positions */
    int lock;               /* keep button down when released */
    int states;             /* number of possible states for this button; 2 state buttons are most common */
    int hidden;             /* this button is hidden and inactive in menu */
} Button;
typedef struct {
    Frame *frame;       /* frame of group */
    SDL_Surface *img;   /* button surface. a row is a button, a column is a state */
    int w, h;           /* button size */
    Button *buttons;    
    int button_count;   /* number of buttons */
    int button_limit;   /* limit of buttons */
    int base_id; 
} Group;

/*
====================================================================
Create button group. At maximum 'limit' buttons may be added.
A buttons tooltip is displayed in 'label'.
An actual button id is computed as base_id + id.
====================================================================
*/
Group *group_create( SDL_Surface *frame, int alpha, SDL_Surface *img, int w, int h, int limit, int base_id, 
                     SDL_Surface *surf, int x, int y );
void group_delete( Group **group );

/*
====================================================================
Add a button at x,y in group. If lock is true this button is a switch.
'id' * group::h is the y_offset of the button.
====================================================================
*/
int group_add_button( Group *group, int id, int x, int y, int lock, const char *tooltip, int states );

/*
====================================================================
Add a button at x,y in group. If lock is true this button is a switch.
'icon_id' is used for the button icon instead of 'id' (allows
multiple buttons of the same icon)
====================================================================
*/
int group_add_button_complex( Group *group, int id, int icon_id, int x, int y, int lock, const char *tooltip, int states );

/*
====================================================================
Get button by global id.
====================================================================
*/
Button* group_get_button( Group *group, int id );

/*
====================================================================
Set active state of a button by global id.
====================================================================
*/
void group_set_active( Group *group, int id, int active );

/*
====================================================================
Set hidden state of a button by global id.
====================================================================
*/
void group_set_hidden( Group *group, int id, int hidden );

/*
====================================================================
Lock button.
====================================================================
*/
void group_lock_button( Group *group, int id, int down );

/*
====================================================================
Check motion event and modify buttons and label.
Return True if in this frame.
====================================================================
*/
int group_handle_motion( Group *group, int x, int y );
/*
====================================================================
Check click event. Return True if button was clicked and return
the button.
====================================================================
*/
int group_handle_button( Group *group, int button_id, int x, int y, Button **button );

/*
====================================================================
Draw group.
====================================================================
*/
#define group_hide( group, hide ) buffer_hide( group->frame->img->bkgnd, hide )
#define group_get_bkgnd( group ) buffer_get( group->frame->img->bkgnd )
#define group_draw_bkgnd( group ) buffer_draw( group->frame->img->bkgnd )
void group_draw( Group *group );

/*
====================================================================
Modify/Get settings.
====================================================================
*/
#define group_set_surface( group, surf ) buffer_set_surface( group->frame->img->bkgnd, surf )
#define group_get_geometry(group,x,y,w,h) buffer_get_geometry(group->frame->img->bkgnd,x,y,w,h)
void group_move( Group *group, int x, int y );
#define group_get_width( group ) frame_get_width( (group)->frame )
#define group_get_height( group ) frame_get_height( (group)->frame )

/*
====================================================================
Return True if x,y is on group button.
====================================================================
*/
int button_focus( Button *button, int x, int y );


/*
====================================================================
Editable label.
====================================================================
*/
typedef struct {
    Label *label;   
    char  *text;    /* text buffer */
    int   limit;    /* max size of text */
    int   blink;    /* if True cursor is displayed (blinks) */
    int   blink_time; /* used to switch blink */
    int   cursor_pos; /* position in text */
    int   cursor_x;   /* position in label */
    int   last_sym;   /* keysym if last key (or -1 if none) */
    int   last_mod;   /* modifier flags of last key */
    int   last_uni;   /* unicode */
    int   delay;      /* delay between key down handle key events */
} Edit;

/*
====================================================================
Created edit.
====================================================================
*/
Edit *edit_create( SDL_Surface *frame, int alpha, Font *font, int limit, SDL_Surface *surf, int x, int y );
void edit_delete( Edit **edit );

/*
====================================================================
Draw label
====================================================================
*/
#define edit_hide( edit, hide ) buffer_hide( edit->label->frame->img->bkgnd, hide )
#define edit_get_bkgnd( edit ) buffer_get( edit->label->frame->img->bkgnd )
#define edit_draw_bkgnd( edit ) buffer_draw( edit->label->frame->img->bkgnd )
void edit_draw( Edit *edit );

/*
====================================================================
Modify label settings
====================================================================
*/
#define edit_move( edit, x, y ) buffer_move( edit->label->frame->img->bkgnd, x, y )
#define edit_set_surface( edit, surf ) buffer_set_surface( edit->label->frame->img->bkgnd, surf )

/*
====================================================================
Set buffer and adjust cursor x. If 'newtext' is NULL the current
text is displayed.
====================================================================
*/
void edit_show( Edit *edit, const char *newtext );

/*
====================================================================
Modify text buffer according to unicode, keysym, and modifier.
====================================================================
*/
void edit_handle_key( Edit *edit, int keysym, int modifier, int unicode );

/*
====================================================================
Update blinking cursor and add keys if keydown. Return True if
key was written.
====================================================================
*/
int edit_update( Edit *edit, int ms );


/*
====================================================================
Listbox
====================================================================
*/
typedef struct _LBox {
    Group *group;       /* basic frame + up/down buttons */
    /* selection */
    List *items;        /* list of items */
    void *cur_item;     /* currently highlighted item */
    /* cells */
    int step;           /* jump this number of cells if wheel up/down */
    int cell_offset;    /* id of first item displayed */
    int cell_count;     /* number of cells */
    int cell_x, cell_y; /* draw position of first cell */
    int cell_w, cell_h; /* size of a cell */
    int cell_gap;       /* gap between cells (vertical) */
    int cell_color;     /* color of selection */
    SDL_Surface *cell_buffer; /* buffer used to render a single cell */
    void  (*render_cb)( void*, SDL_Surface* ); /* render item to surface */
} LBox;

/*
====================================================================
Create a listbox with a number of cells. The item list is
NULL by default.
====================================================================
*/
LBox *lbox_create( SDL_Surface *frame, int alpha, int border, SDL_Surface *buttons, int button_w, int button_h, 
                   int cell_count, int step, int cell_w, int cell_h, int cell_gap, int cell_color, 
                   void (*cb)(void*, SDL_Surface*), 
                   SDL_Surface *surf, int x, int y );
void lbox_delete( LBox **lbox );

/*
====================================================================
Draw listbox
====================================================================
*/
#define lbox_hide( lbox, hide ) buffer_hide( lbox->group->frame->img->bkgnd, hide )
#define lbox_get_bkgnd( lbox ) buffer_get( lbox->group->frame->img->bkgnd )
#define lbox_draw_bkgnd( lbox ) buffer_draw( lbox->group->frame->img->bkgnd )
#define lbox_draw( lbox ) group_draw( lbox->group )

/*
====================================================================
Modify/Get listbox settings
====================================================================
*/
#define lbox_set_surface( lbox, surf ) buffer_set_surface( lbox->group->frame->img->bkgnd, surf )
#define lbox_move( lbox, x, y ) group_move( lbox->group, x, y )
#define lbox_get_width( lbox ) group_get_width( (lbox)->group )
#define lbox_get_height( lbox ) group_get_height( (lbox)->group )
#define lbox_get_selected_item( lbox ) ( (lbox)->cur_item )
#define lbox_get_selected_item_index( lbox ) \
	(list_check( (lbox)->items, (lbox)->cur_item ))
#define lbox_clear_selected_item( lbox ) \
	do { (lbox)->cur_item = 0; lbox_apply(lbox); } while (0)
void *lbox_select_first_item( LBox *lbox );
#define lbox_is_empty( lbox ) \
	((lbox)->items == NULL || (lbox)->items->count == 0)
#define lbox_select_item(lbox, item) \
	do { (lbox)->cur_item = (item); lbox_apply(lbox); } while (0)
	
/*
====================================================================
Rebuild the listbox graphic (lbox->group->frame).
====================================================================
*/
void lbox_apply( LBox *lbox );

/*
====================================================================
Delete the old item list (if any) and use the new one (will be 
deleted by this listbox)
====================================================================
*/
void lbox_set_items( LBox *lbox, List *items );
#define lbox_clear_items( lbox ) lbox_set_items( lbox, NULL )

/*
====================================================================
handle_motion sets button if up/down has focus and returns the item
if any was selected.
====================================================================
*/
int lbox_handle_motion( LBox *lbox, int cx, int cy, void **item );
/*
====================================================================
handle_button internally handles up/down click and returns the item
if one was selected.
====================================================================
*/
int lbox_handle_button( LBox *lbox, int button_id, int cx, int cy, Button **button, void **item );

/** Render listbox item @item (which is a simple string) to listbox cell 
 * surface @buffer. */
void lbox_render_text( void *item, SDL_Surface *buffer );


/*
====================================================================
File dialogue with space to display an info about a file
(e.g. scenario information)
====================================================================
*/
typedef struct {
    LBox *lbox;                         /* file list */
    Group *group;                       /* info frame with ok/cancel buttons */
    char root[1024];                    /* root directory */
    char subdir[1024];                  /* current directory in root directory */
    void (*file_cb)( const char*, const char*, SDL_Surface* );
                                        /* called if a file was selected
                                               the full path is given */
    int info_x, info_y;                 /* info position */
    SDL_Surface *info_buffer;           /* buffer of information */
    int button_x, button_y;             /* position of first button (right upper corner,
                                           new buttons are added to the left) */
    int button_dist;                    /* distance between to buttons (gap + width) */
    int emptyFile;                      /* indicates if this file dialogue reqires empty file */
    int arrangement;                    /* arrangement of two windows
                                           0: arranged next to eachother
                                           1: arranged one above another */
    int dir_only;                       /* this file dialogue searches only folders */
    char *current_name;                 /* current selected file */
    int file_type;                      /* show specific file type in this file dialogue window */
} FDlg;

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
                   SDL_Surface *surf, int x, int y, int arrangement, int emptyFile, int dir_only, int file_type );
void fdlg_delete( FDlg **fdlg );

/*
====================================================================
Draw file dialogue
====================================================================
*/
void fdlg_hide( FDlg *fdlg, int hide );
void fdlg_get_bkgnd( FDlg *fdlg );
void fdlg_draw_bkgnd( FDlg *fdlg );
void fdlg_draw( FDlg *fdlg );

/*
====================================================================
Modify file dialogue settings
====================================================================
*/
void fdlg_set_surface( FDlg *fdlg, SDL_Surface *surf );
void fdlg_move( FDlg *fdlg, int x, int y );

/*
====================================================================
Add button. Graphic is taken from conf_buttons.
====================================================================
*/
void fdlg_add_button( FDlg *fdlg, int id, int lock, const char *tooltip );

/*
====================================================================
Show file dialogue at directory root.
====================================================================
*/
void fdlg_open( FDlg *fdlg, const char *root, const char *subdir );

/*
====================================================================
handle_motion updates the focus of the buttons
====================================================================
*/
int fdlg_handle_motion( FDlg *fdlg, int cx, int cy );
/*
====================================================================
handle_button 
====================================================================
*/
int fdlg_handle_button( FDlg *fdlg, int button_id, int cx, int cy, Button **button );


/*
====================================================================
Setup dialogue
====================================================================
*/
typedef struct {
    LBox *list;
    Group *ctrl;
    Group *module;
    Group *confirm;
    void (*select_cb)(void*); /* called on selecting an item */
    int sel_id; /* id of selected entry */
} SDlg;

/*
====================================================================
Create setup dialogue.
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
                   SDL_Surface *surf, int x, int y );
SDlg *sdlg_camp_create( SDL_Surface *conf_frame, SDL_Surface *conf_buttons,
                   int conf_button_w, int conf_button_h, int id_conf,
                   void (*list_render_cb)(void*,SDL_Surface*),
                   void (*list_select_cb)(void*),
                   SDL_Surface *surf, int x, int y );
void sdlg_delete( SDlg **sdlg );

/*
====================================================================
Draw setup dialogue.
====================================================================
*/
void sdlg_hide( SDlg *sdlg, int hide );
void sdlg_get_bkgnd( SDlg *sdlg );
void sdlg_draw_bkgnd( SDlg *sdlg );
void sdlg_draw( SDlg *sdlg );

/*
====================================================================
Scenario setup dialogue
====================================================================
*/
void sdlg_set_surface( SDlg *sdlg, SDL_Surface *surf );
void sdlg_move( SDlg *sdlg, int x, int y );

/*
====================================================================
handle_motion updates the focus of the buttons
====================================================================
*/
int sdlg_handle_motion( SDlg *sdlg, int cx, int cy );
/*
====================================================================
handle_button 
====================================================================
*/
int sdlg_handle_button( SDlg *sdlg, int button_id, int cx, int cy, Button **button );

/*
====================================================================
Select dialog:
A listbox for selection with OK/Cancel buttons 
====================================================================
*/
typedef struct {
	Group *button_group; /* okay/cancel buttons */
	LBox *select_lbox;
} SelectDlg;

SelectDlg *select_dlg_create(
	SDL_Surface *lbox_frame, 
	SDL_Surface *lbox_buttons, int lbox_button_w, int lbox_button_h, 
	int lbox_cell_count, int lbox_cell_w, int lbox_cell_h,
	void (*lbox_render_cb)(void*, SDL_Surface*),
	SDL_Surface *conf_frame,
	SDL_Surface *conf_buttons, int conf_button_w, int conf_button_h,
	int id_ok, 
	SDL_Surface *surf, int x, int y );
void select_dlg_delete( SelectDlg **sdlg );
int select_dlg_get_width(SelectDlg *sdlg); 
int select_dlg_get_height(SelectDlg *sdlg);
void select_dlg_move( SelectDlg *sdlg, int px, int py);
void select_dlg_hide( SelectDlg *sdlg, int value);
void select_dlg_draw( SelectDlg *sdlg);
void select_dlg_draw_bkgnd( SelectDlg *sdlg);
void select_dlg_get_bkgnd( SelectDlg *sdlg);
int select_dlg_handle_motion( SelectDlg *sdlg, int cx, int cy);
int select_dlg_handle_button( SelectDlg *sdlg, int bid, int cx, int cy, 
	Button **pbtn );
#define select_dlg_set_items( sdlg, items ) \
	lbox_set_items( sdlg->select_lbox, items )
#define select_dlg_clear_items( sdlg ) \
	lbox_clear_items( sdlg->select_lbox )
#define select_dlg_get_selected_item( sdlg ) \
	lbox_get_selected_item( sdlg->select_lbox )
#define select_dlg_get_selected_item_index( sdlg ) \
	lbox_get_selected_item_index( sdlg->select_lbox )

#endif

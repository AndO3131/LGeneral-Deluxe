/***************************************************************************
                          message_dlg.h  -  description
                             -------------------
    begin                : Fri Jan 24 2014
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

#ifndef __MESSAGEDLG_H
#define __MESSAGEDLG_H

/** Dialogue used to display messages. */

typedef struct {
	Group *main_group; /* main frame with scroll buttons */
	Edit *edit_box;    /* edit box */
} MessageDlg;

/** Message dialogue interface, see C-file for detailed comments. */

MessageDlg *message_dlg_create( char *theme_path );
void message_dlg_delete( MessageDlg **mdlg );

int message_dlg_get_height(MessageDlg *mdlg);

void message_dlg_move( MessageDlg *mdlg, int px, int py);
void message_dlg_hide( MessageDlg *mdlg, int value);
void message_dlg_draw( MessageDlg *mdlg);
void message_dlg_draw_bkgnd( MessageDlg *mdlg);
void message_dlg_get_bkgnd( MessageDlg *mdlg);

int message_dlg_handle_motion( MessageDlg *mdlg, int cx, int cy);
int message_dlg_handle_button( MessageDlg *mdlg, int bid, int cx, int cy,
		Button **mbtn );

void message_dlg_reset( MessageDlg *mdlg );
void message_dlg_draw_text( MessageDlg *mdlg );
void message_dlg_add_text( MessageDlg *mdlg );
#endif

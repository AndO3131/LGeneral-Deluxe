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

#ifndef __HEADQURTERSDLG_H
#define __HEADQURTERSDLG_H

/** Dialogue used to inspect army force. */

typedef struct {
	Group *main_group; /* main frame with buttons */
	LBox *units_lbox; /* units of current player */
} HeadquartersDlg;

/** Headquarters  dialogue interface, see C-file for detailed comments. */

HeadquartersDlg *headquarters_dlg_create( char *theme_path );
void headquarters_dlg_delete( HeadquartersDlg **hdlg );

int headquarters_dlg_get_width(HeadquartersDlg *hdlg);
int headquarters_dlg_get_height(HeadquartersDlg *hdlg);

void headquarters_dlg_move( HeadquartersDlg *hdlg, int px, int py);
void headquarters_dlg_hide( HeadquartersDlg *hdlg, int value);
void headquarters_dlg_draw( HeadquartersDlg *hdlg);
void headquarters_dlg_draw_bkgnd( HeadquartersDlg *hdlg);
void headquarters_dlg_get_bkgnd( HeadquartersDlg *hdlg);
#endif

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

#ifndef __PURCHASEDLG_H
#define __PURCHASEDLG_H

/** Dialogue to purchase units or refund purchased reinforcements. */

typedef struct {
	Group *main_group; /* main frame with purchase/exit buttons */
	Frame *ulimit_frame; /* info frame with unit limit */
	LBox *nation_lbox; /* current player's nations */
	LBox *uclass_lbox; /* unit classes */
	LBox *unit_lbox; /* purchasable units in selected class */
	LBox *trsp_lbox; /* purchasable transporters for selected unit */
	LBox *reinf_lbox; /* purchased but not yet deployable units */
	
	Nation *cur_nation; /* pointer in global nations */
	Unit_Class *cur_uclass; /* pointer in global unit_classes */
	Unit_Class *trsp_uclass; /* pointer in global unit classes */
    int cur_core_unit_limit; /* number of core units that can be purchased */
	int cur_unit_limit; /* number of units that can be purchased */
} PurchaseDlg;

/** Purchase dialogue interface, see C-file for detailed comments. */

PurchaseDlg *purchase_dlg_create( const char *theme_path );
void purchase_dlg_delete( PurchaseDlg **pdlg );

int purchase_dlg_get_width(PurchaseDlg *pdlg); 
int purchase_dlg_get_height(PurchaseDlg *pdlg);

void purchase_dlg_move( PurchaseDlg *pdlg, int px, int py);
void purchase_dlg_hide( PurchaseDlg *pdlg, int value);
void purchase_dlg_draw( PurchaseDlg *pdlg);
void purchase_dlg_draw_bkgnd( PurchaseDlg *pdlg);
void purchase_dlg_get_bkgnd( PurchaseDlg *pdlg);

int purchase_dlg_handle_motion( PurchaseDlg *pdlg, int cx, int cy);
int purchase_dlg_handle_button( PurchaseDlg *pdlg, int bid, int cx, int cy, 
	Button **pbtn );

void purchase_dlg_reset( PurchaseDlg *pdlg );

#endif

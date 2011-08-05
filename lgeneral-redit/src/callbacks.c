#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "parser.h"
#include "misc.h"

extern GtkWidget *window;

extern List *unitlib[2][CLASS_COUNT];
extern List *trplib[2];
extern List *reinf[2][SCEN_COUNT];
extern int cur_scen, cur_player, cur_class, cur_unit, cur_trp;
extern int cur_pin, cur_reinf;
extern char *unit_classes[];
extern char *scenarios[];

void
on_file_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_load_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    load_reinf();
}


void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    save_reinf();
}


void
on_build_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    save_reinf();
    build_reinf();
}


void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    finalize();
    gtk_exit( 0 );
}


void
on_scenarios_select_row                (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gchar aux[64];
    GtkWidget *widget = lookup_widget( window, "add_label" );
    if ( widget ) {
        snprintf( aux, 64, "Add Unit For %s", scenarios[row] );
        gtk_label_set_text( GTK_LABEL(widget), aux );
    }
    cur_scen = row; cur_reinf = -1;
    update_reinf_list( cur_scen, cur_player );
}


void
on_players_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    gchar *text = gtk_editable_get_chars( editable, 0, -1 );
    if ( STRCMP( text, "Axis" ) )
        cur_player = AXIS;
    else
        cur_player = ALLIES;
    cur_reinf = -1; cur_unit = -1; cur_trp = -1;
    update_unit_list( cur_player, cur_class );
    update_trp_list( cur_player );
    update_reinf_list( cur_scen, cur_player );
    g_free( text );
}


void
on_classes_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    int i;
    gchar *text = gtk_editable_get_chars( editable, 0, -1 );
    for ( i = 0; i < CLASS_COUNT; i++ )
        if ( STRCMP( text, unit_classes[i * 2 + 1] ) ) {
            cur_class = i;
            cur_unit = -1;
            update_unit_list( cur_player, cur_class );
        }
    g_free( text );
}


void
on_units_select_row                    (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    cur_unit = row;
}


void
on_transporters_select_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    cur_trp = row;
}

void
on_add_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *widget;
    GtkEditable *editable;
    gchar *text;
    UnitLib_Entry *entry, *trp_entry = 0;
    Unit *unit;
    if ( cur_unit == -1 )
        return;
    unit = calloc( 1, sizeof( Unit ) );
    if ( unit ) {
        unit->delay = 1; unit->str = 10; unit->exp = 0;
        /* pin */
        unit->pin = cur_pin++;
        /* id */
        entry = list_get( unitlib[cur_player][cur_class], cur_unit );
        unit->uid = entry->nid;
        strcpy_lt( unit->id, entry->id, 7 );
        /* trsp id */
        if ( cur_trp > 0 ) {
            trp_entry = list_get( trplib[cur_player], cur_trp );
            unit->tid = trp_entry->nid;
            strcpy_lt( unit->trp, trp_entry->id, 7 );
        }
        else {
            unit->tid = -1;
            strcpy( unit->trp, "none" );
        }
        /* delay */
        if ( ( widget = lookup_widget( window, "delay" ) ) )
            unit->delay = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON( widget ) );
        /* strength */
        if ( ( widget = lookup_widget( window, "strength" ) ) ) {
            editable = GTK_EDITABLE( GTK_COMBO(widget)->entry );
            text = gtk_editable_get_chars( editable, 0, -1 );
            unit->str = atoi( text );
            g_free( text );
        }
        /* experience */
        if ( ( widget = lookup_widget( window, "exp" ) ) ) {
            editable = GTK_EDITABLE( GTK_COMBO(widget)->entry );
            text = gtk_editable_get_chars( editable, 0, -1 );
            unit->exp = atoi( text );
            g_free( text );
        }
        /* nation (-1 == auto) */
        unit->nation = -1;
        /* build info string */
        build_unit_info( unit, cur_player );
        /* save list settings to restore from reinf list */
        unit->class_id = cur_class;
        unit->unit_id = cur_unit;
        unit->trp_id = cur_trp;
        /* add */
        insert_reinf_unit( unit, cur_player, cur_scen );
        update_reinf_list( cur_scen, cur_player );
    }
}


void
on_remove_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    if ( cur_reinf == -1 ) return;
    list_delete_pos( reinf[cur_player][cur_scen], cur_reinf );
    update_reinf_list( cur_scen, cur_player );
    cur_reinf = -1;
}

void
on_reinforcements_select_row           (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GtkWidget *widget;
    Unit *unit;
    int pin, i;
    gchar *text, *hash;
    gchar aux[16];
    gtk_clist_get_text( clist, row, 0, &text );
    hash = strrchr( text, '#' ); hash++;
    pin = atoi( hash ); i = 0; cur_reinf = -1;
    list_reset( reinf[cur_player][cur_scen] );
    while ( ( unit = list_next( reinf[cur_player][cur_scen] ) ) ) {
        if ( unit->pin == pin ) {
            cur_reinf = i;
            /* set unit class */
            if ( ( widget = lookup_widget( window, "classes" ) ) ) {
                cur_class = unit->class_id;
                gtk_entry_set_text( 
                    GTK_ENTRY( GTK_COMBO(widget)->entry ), 
                    unit_classes[unit->class_id * 2 + 1] );
                update_unit_list( cur_player, cur_class );
            }
            /* set unit type */
            if ( ( widget = lookup_widget( window, "units" ) ) )
                gtk_clist_select_row( GTK_CLIST(widget), unit->unit_id, 0 );
            /* set transporter type */
            if ( unit->trp_id == -1 ) unit->trp_id = 0;
            if ( ( widget = lookup_widget( window, "transporters" ) ) )
                gtk_clist_select_row( GTK_CLIST(widget), unit->trp_id, 0 );
            /* set delay, strength ,experience */
            if ( ( widget = lookup_widget( window, "delay" ) ) ) {
                gtk_spin_button_set_value( GTK_SPIN_BUTTON(widget), unit->delay );
            }
            if ( ( widget = lookup_widget( window, "strength" ) ) ) {
                sprintf( aux, "%i", unit->str );
                gtk_entry_set_text( 
                    GTK_ENTRY( GTK_COMBO(widget)->entry ), aux );
            }
            if ( ( widget = lookup_widget( window, "exp" ) ) ) {
                sprintf( aux, "%i", unit->exp );
                gtk_entry_set_text( 
                    GTK_ENTRY( GTK_COMBO(widget)->entry ), aux );
            }
            break;
        }
        i++;
    }
}

gboolean
on_reinforcements_key_press_event      (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if ( cur_reinf == -1 ) return FALSE;
    list_delete_pos( reinf[cur_player][cur_scen], cur_reinf );
    update_reinf_list( cur_scen, cur_player );
    cur_reinf = -1;
    return FALSE;
}


void
on_replace_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    if ( cur_reinf == -1 ) return;
    list_delete_pos( reinf[cur_player][cur_scen], cur_reinf );
    on_add_clicked( 0, 0 );
    update_reinf_list( cur_scen, cur_player );
    cur_reinf = -1;
}

void
on_window_destroy                      (GtkObject       *object,
                                        gpointer         user_data)
{
    finalize();
    gtk_exit( 0 );
}

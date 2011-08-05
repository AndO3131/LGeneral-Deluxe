#include <gtk/gtk.h>


void
on_file_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_load_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_build_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_scenarios_select_row                (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_players_changed                     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_classes_changed                     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_units_select_row                    (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_transporters_select_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_add_clicked                         (GtkButton       *button,
                                        gpointer         user_data);

void
on_remove_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_reinforcements_select_row           (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_reinforcements_button_press_event   (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_reinforcements_key_press_event      (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_replace_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_remove_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_window_destroy                      (GtkObject       *object,
                                        gpointer         user_data);

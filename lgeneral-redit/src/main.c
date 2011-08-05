/*
 * Anfängliche Datei main.c, erzeugt durch Glade. Bearbeiten Sie
 * Sie, wie Sie wollen. Glade wird diese Datei nicht überschreiben.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

#include "interface.h"
#include "support.h"
#include "list.h"
#include "misc.h"

GtkWidget *window;
  
int
main (int argc, char *argv[])
{

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  window = create_window ();

  init();
    
  gtk_widget_show (window);
  gtk_main ();
  return 0;
}


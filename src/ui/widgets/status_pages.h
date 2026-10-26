#pragma once

#include <adwaita.h>

/* The non-list states of the main window. */
GtkWidget *dy_status_page_starting_new (void);
GtkWidget *dy_status_page_empty_new    (void);
/* title and description are filled in when shown */
GtkWidget *dy_status_page_failed_new   (void);

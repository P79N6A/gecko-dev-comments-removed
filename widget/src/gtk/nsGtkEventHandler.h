




































#ifndef __nsGtkEventHandler_h
#define __nsGtkEventHandler_h

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "gdksuperwin.h"

void handle_size_allocate(GtkWidget *w, GtkAllocation *alloc, gpointer p);

gint handle_key_release_event(GtkObject *w, GdkEventKey* event, gpointer p);
gint handle_key_press_event(GtkObject *w, GdkEventKey* event, gpointer p);



void handle_xlib_shell_event(GdkSuperWin *superwin, XEvent *event, gpointer p);
void handle_superwin_paint(gint aX, gint aY,
                           gint aWidth, gint aHeight, gpointer aData);
void handle_superwin_flush(gpointer aData);
void handle_gdk_event (GdkEvent *event, gpointer data);

#endif  





#ifndef GTKWINDOW_WRAPPER_H
#define GTKWINDOW_WRAPPER_H

#define gtk_window_group_get_current_grab gtk_window_group_get_current_grab_
#define gtk_window_get_window_type gtk_window_get_window_type_
#include_next <gtk/gtkwindow.h>
#undef gtk_window_group_get_current_grab
#undef gtk_window_get_window_type

static inline GtkWidget *
gtk_window_group_get_current_grab(GtkWindowGroup *window_group)
{
  if (!window_group->grabs)
    return NULL;

  return GTK_WIDGET(window_group->grabs->data);
}

static inline GtkWindowType
gtk_window_get_window_type(GtkWindow *window)
{
  gint type;
  g_object_get(window, "type", &type, (void*)NULL);
  return (GtkWindowType)type;
}
#endif 

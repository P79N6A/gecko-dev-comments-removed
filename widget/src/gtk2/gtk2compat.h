











































#ifndef _GTK2_COMPAT_H_
#define _GTK2_COMPAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !GTK_CHECK_VERSION(2, 14, 0)
static inline GdkWindow*
gtk_widget_get_window(GtkWidget *widget)
{
  return widget->window;
}

static inline const guchar *
gtk_selection_data_get_data(GtkSelectionData *selection_data)
{
  return selection_data->data;
}

static inline gint
gtk_selection_data_get_length(GtkSelectionData *selection_data)
{
  return selection_data->length;
}

static inline GdkAtom
gtk_selection_data_get_target(GtkSelectionData *selection_data)
{
  return selection_data->target;
}

static inline GtkWidget *
gtk_dialog_get_content_area(GtkDialog *dialog)
{
  return dialog->vbox;
}
#endif


#if !GTK_CHECK_VERSION(2, 16, 0)
static inline GdkAtom
gtk_selection_data_get_selection(GtkSelectionData *selection_data)
{
  return selection_data->selection;
}
#endif

#if !GTK_CHECK_VERSION(2, 18, 0)
static inline gboolean
gtk_widget_get_visible(GtkWidget *widget)
{
  return GTK_WIDGET_VISIBLE(widget);
}

static inline gboolean
gtk_widget_has_focus(GtkWidget *widget)
{
  return GTK_WIDGET_HAS_FOCUS(widget);
}

static inline void
gtk_widget_get_allocation(GtkWidget *widget, GtkAllocation *allocation)
{
  *allocation = widget->allocation;
}

static inline gboolean
gdk_window_is_destroyed(GdkWindow *window)
{
  return GDK_WINDOW_OBJECT(window)->destroyed;
}
#endif

#if !GTK_CHECK_VERSION(2, 22, 0)
static inline gint
gdk_visual_get_depth(GdkVisual *visual)
{
  return visual->depth;
}

static inline GdkDragAction
gdk_drag_context_get_actions(GdkDragContext *context)
{
  return context->actions;
}
#endif

#if !GTK_CHECK_VERSION(2, 24, 0)
static inline GdkWindow *
gdk_x11_window_lookup_for_display(GdkDisplay *display, Window window)
{
  return gdk_window_lookup_for_display(display, window);
}

static inline GdkDisplay *
gdk_window_get_display(GdkWindow *window)
{
  return gdk_drawable_get_display(GDK_DRAWABLE(window));
}
#endif

static inline Window 
gdk_x11_window_get_xid(GdkWindow *window)
{
  return(GDK_WINDOW_XWINDOW(window));
}

#ifdef __cplusplus
}
#endif 

#endif

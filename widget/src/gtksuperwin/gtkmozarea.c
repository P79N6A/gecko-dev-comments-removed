






































#include "gtkmozarea.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

static void gtk_mozarea_class_init (GtkMozAreaClass *klass);
static void gtk_mozarea_init       (GtkMozArea      *mozarea);
static void gtk_mozarea_realize    (GtkWidget       *widget);
static void gtk_mozarea_unrealize  (GtkWidget       *widget);
static void gtk_mozarea_size_allocate (GtkWidget    *widget,
                                       GtkAllocation *allocation);
static void gtk_mozarea_destroy    (GtkObject       *object);

static void
attach_toplevel_listener(GtkMozArea *mozarea);

static void
remove_toplevel_listener(GtkMozArea *mozarea);

static Window
get_real_toplevel(Window aWindow);

static GdkWindow *
get_real_gdk_toplevel(GtkMozArea *mozarea);

static GdkFilterReturn
toplevel_window_filter(GdkXEvent   *aGdkXEvent,
                       GdkEvent    *aEvent,
                       gpointer     data);

static GdkFilterReturn
widget_window_filter(GdkXEvent     *aGdkXEvent,
                     GdkEvent      *aEvent,
                     gpointer       data);

GtkWidgetClass *parent_class = NULL;

enum {
  TOPLEVEL_FOCUS_IN,
  TOPLEVEL_FOCUS_OUT,
  TOPLEVEL_CONFIGURE,
  LAST_SIGNAL
};

static guint mozarea_signals[LAST_SIGNAL] = { 0 };

GtkType
gtk_mozarea_get_type (void)
{
  static GtkType mozarea_type = 0;
  
  if (!mozarea_type)
    {
      static const GtkTypeInfo mozarea_info =
      {
        "GtkMozArea",
        sizeof (GtkMozArea),
        sizeof (GtkMozAreaClass),
        (GtkClassInitFunc) gtk_mozarea_class_init,
        (GtkObjectInitFunc) gtk_mozarea_init,
         NULL,
         NULL,
        (GtkClassInitFunc) NULL
      };
      
      mozarea_type = gtk_type_unique (GTK_TYPE_WIDGET, &mozarea_info);
    }

  return mozarea_type;
}

static void
gtk_mozarea_class_init (GtkMozAreaClass *klass)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  object_class = GTK_OBJECT_CLASS (klass);

  widget_class->realize = gtk_mozarea_realize;
  widget_class->unrealize = gtk_mozarea_unrealize;
  widget_class->size_allocate = gtk_mozarea_size_allocate;

  object_class->destroy = gtk_mozarea_destroy;

  parent_class = gtk_type_class(gtk_widget_get_type());

  

  mozarea_signals[TOPLEVEL_FOCUS_IN] =
    gtk_signal_new("toplevel_focus_in",
                   GTK_RUN_FIRST,
                   object_class->type,
                   GTK_SIGNAL_OFFSET(GtkMozAreaClass, toplevel_focus_in),
                   gtk_marshal_NONE__NONE,
                   GTK_TYPE_NONE, 0);
  mozarea_signals[TOPLEVEL_FOCUS_OUT] =
    gtk_signal_new("toplevel_focus_out",
                   GTK_RUN_FIRST,
                   object_class->type,
                   GTK_SIGNAL_OFFSET(GtkMozAreaClass, toplevel_focus_out),
                   gtk_marshal_NONE__NONE,
                   GTK_TYPE_NONE, 0);
  mozarea_signals[TOPLEVEL_CONFIGURE] =
    gtk_signal_new("toplevel_configure",
                   GTK_RUN_FIRST,
                   object_class->type,
                   GTK_SIGNAL_OFFSET(GtkMozAreaClass, toplevel_configure),
                   gtk_marshal_NONE__NONE,
                   GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals(object_class, mozarea_signals, LAST_SIGNAL);

}

static void
gtk_mozarea_init (GtkMozArea *mozarea)
{
  mozarea->superwin = NULL;
  mozarea->toplevel_focus = FALSE;
  mozarea->toplevel_window = NULL;
  gtk_widget_set_name(GTK_WIDGET(mozarea), "gtkmozarea");
}

static void 
gtk_mozarea_realize (GtkWidget *widget)
{
  GtkMozArea *mozarea;
  
  g_return_if_fail (GTK_IS_MOZAREA (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  mozarea = GTK_MOZAREA (widget);

  mozarea->superwin = gdk_superwin_new (gtk_widget_get_parent_window (widget),
                                        widget->allocation.x,
                                        widget->allocation.y,
                                        widget->allocation.width,
                                        widget->allocation.height);
  gdk_window_set_user_data (mozarea->superwin->shell_window, mozarea);
  widget->window = mozarea->superwin->shell_window;
  widget->style = gtk_style_attach (widget->style, widget->window);
  


  gdk_window_ref(widget->window);

  
  attach_toplevel_listener(mozarea);

  


  gdk_window_add_filter(widget->window, widget_window_filter, mozarea);
}

static void
gtk_mozarea_unrealize(GtkWidget *widget)
{
  GtkMozArea *mozarea;
  
  g_return_if_fail(GTK_IS_MOZAREA(widget));

  GTK_WIDGET_UNSET_FLAGS(widget, GTK_REALIZED);

  mozarea = GTK_MOZAREA(widget);
  
  if (mozarea->superwin) {
    gtk_object_unref(GTK_OBJECT(mozarea->superwin));
    mozarea->superwin = NULL;
  }
  GTK_WIDGET_CLASS(parent_class)->unrealize(widget);
}

static void 
gtk_mozarea_size_allocate (GtkWidget    *widget,
			   GtkAllocation *allocation)
{
  GtkMozArea *mozarea;
  
  g_return_if_fail (GTK_IS_MOZAREA (widget));

  mozarea = GTK_MOZAREA (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move (mozarea->superwin->shell_window,
		       allocation->x, allocation->y);
      gdk_superwin_resize (mozarea->superwin,
			   allocation->width, allocation->height);
    }
}

static void
gtk_mozarea_destroy(GtkObject *object)
{
  GtkMozArea *mozarea;

  g_return_if_fail(GTK_IS_MOZAREA(object));
  
  mozarea = GTK_MOZAREA(object);

  
  remove_toplevel_listener(mozarea);

  GTK_OBJECT_CLASS(parent_class)->destroy(object);
}

GtkWidget*
gtk_mozarea_new (GdkWindow *parent_window)
{
  return GTK_WIDGET (gtk_type_new (GTK_TYPE_MOZAREA));
}

gboolean
gtk_mozarea_get_toplevel_focus(GtkMozArea *area)
{
  g_return_val_if_fail(GTK_IS_MOZAREA(area), FALSE);
  
  return area->toplevel_focus;
}
                               


static void
attach_toplevel_listener(GtkMozArea *mozarea)
{
  
  GdkWindow *gdk_window = get_real_gdk_toplevel(mozarea);

  

  gdk_window_add_filter(gdk_window, toplevel_window_filter, mozarea);

  
  mozarea->toplevel_window = gdk_window;
}


static void
remove_toplevel_listener(GtkMozArea *mozarea)
{
  gdk_window_remove_filter(mozarea->toplevel_window, toplevel_window_filter,
                           mozarea);
}


static Window
get_real_toplevel(Window aWindow)
{
  Window current = aWindow;
  Atom   atom;

  


  atom = XInternAtom(GDK_DISPLAY(), "WM_STATE", FALSE);

  while (current) {
    Atom type = None;
    int format;
    unsigned long nitems, after;
    unsigned char *data;

    Window root_return;
    Window parent_return;
    Window *children_return = NULL;
    unsigned int nchildren_return;

    
    XGetWindowProperty(GDK_DISPLAY(), current, atom,
		       0, 0, 
		       False, 
		       AnyPropertyType,
		       &type, &format, &nitems, &after, &data);

    
    if (type != None) {
      XFree(data);
      data = NULL;
      

      break;
    }
    
    
    XQueryTree(GDK_DISPLAY(), current, &root_return,
	       &parent_return, &children_return,
	       &nchildren_return);

    if (children_return)
      XFree(children_return);

    


    if (parent_return == root_return)
      break;

    current = parent_return;
  }

  return current;
}

static GdkWindow *
get_real_gdk_toplevel(GtkMozArea *mozarea)
{
  
  GtkWidget *widget = GTK_WIDGET(mozarea);
  Window window = GDK_WINDOW_XWINDOW(widget->window);
  
  Window toplevel = get_real_toplevel(window);
  
  

  GdkWindow *gdk_window = gdk_window_lookup(toplevel);
  
  
  if (!gdk_window) {
    
    gdk_window = gdk_window_foreign_new(toplevel);
    
    gdk_window_set_events(gdk_window, GDK_FOCUS_CHANGE_MASK);
  }

  return gdk_window;
}

static GdkFilterReturn
toplevel_window_filter(GdkXEvent   *aGdkXEvent,
                       GdkEvent    *aEvent,
                       gpointer     data)
{
  XEvent       *xevent = (XEvent *)aGdkXEvent;
  GtkMozArea   *mozarea = (GtkMozArea *)data;

  switch (xevent->xany.type) {
  case FocusIn:
    mozarea->toplevel_focus = TRUE;
    gtk_signal_emit(GTK_OBJECT(mozarea), mozarea_signals[TOPLEVEL_FOCUS_IN]);
    break;
  case FocusOut:
    mozarea->toplevel_focus = FALSE;
    gtk_signal_emit(GTK_OBJECT(mozarea), mozarea_signals[TOPLEVEL_FOCUS_OUT]);
    break;
  case ConfigureNotify:
    gtk_signal_emit(GTK_OBJECT(mozarea), mozarea_signals[TOPLEVEL_CONFIGURE]);
    break;
  default:
    break;
  }

  return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
widget_window_filter(GdkXEvent     *aGdkXEvent,
                     GdkEvent      *aEvent,
                     gpointer       data)
{
  XEvent       *xevent = (XEvent *)aGdkXEvent;
  GtkMozArea   *mozarea = (GtkMozArea *)data;

  switch(xevent->xany.type) {
  case ReparentNotify:
    
    remove_toplevel_listener(mozarea);
    attach_toplevel_listener(mozarea);
    break;
  default:
    break;
  }
  return GDK_FILTER_CONTINUE;
}

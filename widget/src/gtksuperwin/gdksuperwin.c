





































#include "gdksuperwin.h"

static void gdk_superwin_class_init(GdkSuperWinClass *klass);
static void gdk_superwin_init(GdkSuperWin *superwin);
static void gdk_superwin_expose_area  (GdkSuperWin *superwin,
                                       gint         x,
                                       gint         y,
                                       gint         width,
                                       gint         height);
static void gdk_superwin_flush(GdkSuperWin *superwin);
static void gdk_superwin_destroy(GtkObject *object);

static void gdk_superwin_add_translation(GdkSuperWin *superwin, unsigned long serial,
                                         gint dx, gint dy);
static void gdk_superwin_add_antiexpose (GdkSuperWin *superwin, unsigned long serial,
                                         gint x, gint y,
                                         gint width, gint height);

static void gdk_superwin_handle_expose (GdkSuperWin *superwin, XEvent *xevent,
                                        GdkRegion **region, gboolean dont_recurse);

static Bool gdk_superwin_expose_predicate(Display  *display,
                                          XEvent   *xevent,
                                          XPointer  arg);

typedef struct _GdkSuperWinTranslate GdkSuperWinTranslate;

enum {
  GDK_SUPERWIN_TRANSLATION = 1,
  GDK_SUPERWIN_ANTIEXPOSE
};
  

struct _GdkSuperWinTranslate
{
  int type;
  unsigned long serial;
  union {
    struct {
      gint dx;
      gint dy;
    } translation;
    struct {
      GdkRectangle rect;
    } antiexpose;
  } data;
};

GtkType
gdk_superwin_get_type(void)
{
  static GtkType superwin_type = 0;
  
  if (!superwin_type)
    {
      static const GtkTypeInfo superwin_info =
      {
        "GtkSuperWin",
          sizeof(GdkSuperWin),
          sizeof(GdkSuperWinClass),
          (GtkClassInitFunc) gdk_superwin_class_init,
          (GtkObjectInitFunc) gdk_superwin_init,
           NULL,
           NULL,
          (GtkClassInitFunc) NULL
      };
      
      superwin_type = gtk_type_unique (gtk_object_get_type(), &superwin_info);
    }
  return superwin_type;
}

static void
gdk_superwin_class_init(GdkSuperWinClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS(klass);
  object_class->destroy = gdk_superwin_destroy;

}

static void
gdk_superwin_init(GdkSuperWin *superwin)
{
  
}

static GdkFilterReturn  gdk_superwin_bin_filter   (GdkXEvent *gdk_xevent,
                                                   GdkEvent  *event,
                                                   gpointer   data);
static GdkFilterReturn  gdk_superwin_shell_filter (GdkXEvent *gdk_xevent,
                                                   GdkEvent  *event,
                                                   gpointer   data);

static gboolean gravity_works;

GdkSuperWin *
gdk_superwin_new (GdkWindow *parent_window,
                  guint      x,
                  guint      y,
                  guint      width,
                  guint      height)
{
  GdkWindowAttr         attributes;
  gint                  attributes_mask;
  Window                bin_xwindow;
  Display              *xdisplay;
  XSetWindowAttributes  xattr;
  unsigned long         xattr_mask;

  GdkSuperWin *superwin = gtk_type_new(GDK_TYPE_SUPERWIN);

  superwin->translate_queue = NULL;

  superwin->shell_func = NULL;
  superwin->paint_func = NULL;
  superwin->flush_func = NULL;
  superwin->func_data = NULL;
  superwin->notify = NULL;

  
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = x;
  attributes.y = y;
  attributes.width = width;
  attributes.height = height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.colormap = gdk_rgb_get_cmap();
  attributes.visual = gdk_rgb_get_visual();
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = GDK_WA_VISUAL | GDK_WA_X | GDK_WA_Y | GDK_WA_COLORMAP;

  superwin->shell_window = gdk_window_new (parent_window,
					   &attributes, attributes_mask);

  

  gdk_window_set_back_pixmap (superwin->shell_window, NULL, FALSE);

  
  g_assert((superwin->shell_window));

  

  attributes.x = 0;
  attributes.y = 0;
  attributes.event_mask = GDK_EXPOSURE_MASK;

  superwin->bin_window = gdk_window_new (superwin->shell_window,
                                         &attributes, attributes_mask);

  

  gdk_window_set_back_pixmap (superwin->bin_window, NULL, FALSE);

  
  bin_xwindow = GDK_WINDOW_XWINDOW(superwin->bin_window);
  xdisplay = GDK_WINDOW_XDISPLAY(superwin->bin_window);
  
  xattr.backing_store = WhenMapped;
  xattr_mask = CWBackingStore;
  

  gdk_window_show (superwin->bin_window);

  gdk_window_add_filter (superwin->shell_window, gdk_superwin_shell_filter, superwin);
  gdk_window_add_filter (superwin->bin_window, gdk_superwin_bin_filter, superwin);

  gravity_works = gdk_window_set_static_gravities (superwin->bin_window, TRUE);

  return superwin;
}




void gdk_superwin_destroy(GtkObject *object)
{
  
  GdkSuperWin *superwin = NULL;

  g_return_if_fail(object != NULL);
  g_return_if_fail(GTK_IS_OBJECT(object));
  g_return_if_fail(GTK_OBJECT_CONSTRUCTED(object));
  g_return_if_fail(GDK_IS_SUPERWIN(object));

  superwin = GDK_SUPERWIN(object);

  gdk_window_remove_filter(superwin->shell_window,
                           gdk_superwin_shell_filter,
                           superwin);
  gdk_window_remove_filter(superwin->bin_window,
                           gdk_superwin_bin_filter,
                           superwin);
  gdk_window_destroy(superwin->bin_window);
  gdk_window_destroy(superwin->shell_window);

  if (superwin->translate_queue) {
    GSList *tmp_list = superwin->translate_queue;
    while (tmp_list) {
      g_free(tmp_list->data);
      tmp_list = tmp_list->next;
    }
    g_slist_free(superwin->translate_queue);
  }
}

void gdk_superwin_reparent(GdkSuperWin *superwin,
                           GdkWindow   *parent_window)
{
  gdk_window_reparent(superwin->shell_window,
                      parent_window, 0, 0);
}

void         
gdk_superwin_scroll (GdkSuperWin *superwin,
                     gint dx,
                     gint dy)
{
  gint width, height;

  gint first_resize_x = 0;
  gint first_resize_y = 0;
  gint first_resize_width = 0;
  gint first_resize_height = 0;

  unsigned long first_resize_serial = 0;
  unsigned long move_serial = 0;
  unsigned long last_resize_serial = 0;
  gint move_x = 0;
  gint move_y = 0;

  
  gdk_window_get_size (superwin->shell_window, &width, &height);

  

  

  first_resize_width = width;
  first_resize_height = height;

  

  if (dx < 0) {
    
    first_resize_x = 0;
    

    first_resize_width = width + ABS(dx);
  }

  

  if (dx > 0) {
    

    first_resize_x = -dx;
    

    first_resize_width = width + dx;
  }

  

  if (dy < 0) {
    
    first_resize_y = 0;
    
    first_resize_height = height + ABS(dy);
  }

  

  if (dy > 0) {
    
    first_resize_y = -dy;
    

    first_resize_height = height + dy; 
  }

  

  

  if (dx < 0) {
    
    move_x = dx;
  }

  


  if (dx > 0) {
    move_x = 0;
  }

  

  if (dy < 0) {
    
    move_y = dy;
  }

  



  if (dy > 0) {
    move_y = 0;
  }

  
  first_resize_serial = NextRequest(GDK_DISPLAY());

  
  gdk_window_move_resize(superwin->bin_window,
                         first_resize_x, first_resize_y,
                         first_resize_width, first_resize_height);

  





  move_serial = NextRequest(GDK_DISPLAY());

  gdk_window_move(superwin->bin_window,
                  move_x, move_y);

  


  
  last_resize_serial = NextRequest(GDK_DISPLAY());

  gdk_window_move_resize(superwin->bin_window,
                         0, 0, width, height);

  

  





  if (dx < 0) {
    gdk_superwin_expose_area(superwin, MAX(0, width - ABS(dx)), 0,
                             MIN(width, ABS(dx)), height);
    




    gdk_superwin_add_antiexpose(superwin, move_serial,
                                MAX(width, ABS(dx)),
                                0, MIN(width, ABS(dx)), height);
  }

  


  if (dx > 0) {
    gdk_superwin_expose_area(superwin, 0, 0, 
                             MIN(width, ABS(dx)), height);
    
    gdk_superwin_add_antiexpose(superwin, move_serial,
                                0, 0, MIN(width, ABS(dx)), height);
  }

  





  if (dy < 0) {
    gdk_superwin_expose_area(superwin, 0, MAX(0, height - ABS(dy)),
                             width, MIN(height, ABS(dy)));
    




        gdk_superwin_add_antiexpose(superwin, move_serial,
                                    0,
                                    MAX(height, ABS(dy)),
                                    width, MIN(height, ABS(dy)));
  }

  


  if (dy > 0) {
    gdk_superwin_expose_area(superwin, 0, 0,
                             width, MIN(height, ABS(dy)));
    
    gdk_superwin_add_antiexpose(superwin, move_serial,
                                0, 0, width, MIN(height, ABS(dy)));
  }

  



  if (dx > 0 || dy > 0) {
    gdk_superwin_add_translation(superwin, first_resize_serial,
                                 MAX(0, dx), MAX(0, dy));
  }

  



  if (dx < 0 || dy < 0) {
    gdk_superwin_add_translation(superwin, last_resize_serial,
                                 MIN(0, dx), MIN(0, dy));
  }

  
  XSync(GDK_DISPLAY(), False);
}

void  
gdk_superwin_set_event_funcs (GdkSuperWin               *superwin,
                              GdkSuperWinFunc            shell_func,
                              GdkSuperWinPaintFunc       paint_func,
                              GdkSuperWinPaintFlushFunc  flush_func,
                              GdkSuperWinKeyPressFunc    keyprs_func,
                              GdkSuperWinKeyReleaseFunc  keyrel_func,
                              gpointer                   func_data,
                              GDestroyNotify             notify)
{
  if (superwin->notify && superwin->func_data)
    superwin->notify (superwin->func_data);
  
  superwin->shell_func = shell_func;
  superwin->paint_func = paint_func;
  superwin->flush_func = flush_func;
  superwin->keyprs_func = keyprs_func;
  superwin->keyrel_func = keyrel_func;
  superwin->func_data = func_data;
  superwin->notify = notify;

}

void gdk_superwin_resize (GdkSuperWin *superwin,
			  gint         width,
			  gint         height)
{
  gdk_window_resize (superwin->bin_window, width, height);
  gdk_window_resize (superwin->shell_window, width, height);
}

static void
gdk_superwin_expose_area  (GdkSuperWin *superwin,
                           gint         x,
                           gint         y,
                           gint         width,
                           gint         height)
{
  if (superwin->paint_func)
    superwin->paint_func(x, y, width, height, superwin->func_data);
}

static void
gdk_superwin_flush(GdkSuperWin *superwin)
{
  if (superwin->flush_func)
    superwin->flush_func(superwin->func_data);
}

static GdkFilterReturn 
gdk_superwin_bin_filter (GdkXEvent *gdk_xevent,
                         GdkEvent  *event,
                         gpointer   data)
{
  XEvent *xevent = (XEvent *)gdk_xevent;
  GdkSuperWin *superwin = data;
  GdkFilterReturn retval = GDK_FILTER_CONTINUE;
  GdkRegion *region = NULL;

  switch (xevent->xany.type) {
  case Expose:
    region = gdk_region_new();
    retval = GDK_FILTER_REMOVE;
    gdk_superwin_handle_expose(superwin, xevent, &region, FALSE);
    gdk_region_destroy(region);
    break;
  case KeyPress:
    if (superwin->keyprs_func)
      superwin->keyprs_func(&xevent->xkey);
    break;
  case KeyRelease:
    if (superwin->keyrel_func)
      superwin->keyrel_func(&xevent->xkey);
    break;
  default:
    break;
  }
  return retval;
}
    

static GdkFilterReturn 
gdk_superwin_shell_filter (GdkXEvent *gdk_xevent,
			  GdkEvent  *event,
			  gpointer   data)
{
  XEvent *xevent = (XEvent *)gdk_xevent;
  GdkSuperWin *superwin = data;

  if (xevent->type == VisibilityNotify)
    {
      switch (xevent->xvisibility.state)
        {
        case VisibilityFullyObscured:
          superwin->visibility = GDK_VISIBILITY_FULLY_OBSCURED;
          break;
          
        case VisibilityPartiallyObscured:
          superwin->visibility = GDK_VISIBILITY_PARTIAL;
          break;
          
        case VisibilityUnobscured:
          superwin->visibility = GDK_VISIBILITY_UNOBSCURED;
          break;
        }
      
      return GDK_FILTER_REMOVE;
    }
  
  if (superwin->shell_func) {
    superwin->shell_func (superwin, xevent, superwin->func_data);
  }
  return GDK_FILTER_CONTINUE;
}





Bool gdk_superwin_expose_predicate(Display  *display,
                                   XEvent   *xevent,
                                   XPointer  arg) {
  GdkSuperWin *superwin = (GdkSuperWin *)arg;
  if (xevent->xany.window != GDK_WINDOW_XWINDOW(superwin->bin_window))
    return False;
  switch (xevent->xany.type) {
  case Expose:
    return True;
    break;
  default:
    return False;
    break;
  }
}


void gdk_superwin_add_translation(GdkSuperWin *superwin, unsigned long serial,
                                  gint dx, gint dy)
{
  GdkSuperWinTranslate *translate = g_new(GdkSuperWinTranslate, 1);
  translate->type = GDK_SUPERWIN_TRANSLATION;
  translate->serial = serial;
  translate->data.translation.dx = dx;
  translate->data.translation.dy = dy;
  superwin->translate_queue = g_slist_append(superwin->translate_queue, translate);
}


void gdk_superwin_add_antiexpose (GdkSuperWin *superwin, unsigned long serial,
                                  gint x, gint y,
                                  gint width, gint height)

{
  GdkSuperWinTranslate *translate = g_new(GdkSuperWinTranslate, 1);
  translate->type = GDK_SUPERWIN_ANTIEXPOSE;
  translate->serial = serial;
  translate->data.antiexpose.rect.x = x;
  translate->data.antiexpose.rect.y = y;
  translate->data.antiexpose.rect.width = width;
  translate->data.antiexpose.rect.height = height;
  superwin->translate_queue = g_slist_append(superwin->translate_queue, translate);
}


void gdk_superwin_handle_expose (GdkSuperWin *superwin, XEvent *xevent,
                                 GdkRegion **region, gboolean dont_recurse)
{
  GSList *tmp_list;
  gboolean send_event = TRUE;
  unsigned long serial = xevent->xany.serial;
  XEvent extra_event;
  GdkRectangle rect;
  GdkRegion *tmp_region = NULL;
  gboolean   is_special = TRUE;

  
  rect.x = xevent->xexpose.x;
  rect.y = xevent->xexpose.y;
  rect.width = xevent->xexpose.width;
  rect.height = xevent->xexpose.height;

  
  tmp_list = superwin->translate_queue;
  while (tmp_list) {
    GdkSuperWinTranslate *xlate = tmp_list->data;
    if (xlate->type == GDK_SUPERWIN_ANTIEXPOSE && serial == xlate->serial) {
      GdkRegion *antiexpose_region = gdk_region_new();
      tmp_region = gdk_region_union_with_rect(antiexpose_region, 
                                              &xlate->data.antiexpose.rect);
      gdk_region_destroy(antiexpose_region);
      antiexpose_region = tmp_region;
      

      if (gdk_region_rect_in(antiexpose_region, &rect) == GDK_OVERLAP_RECTANGLE_IN) {
        gdk_region_destroy(antiexpose_region);
        goto end;
      }
      gdk_region_destroy(antiexpose_region);

    }
    tmp_list = tmp_list->next;
  }

  
  tmp_list = superwin->translate_queue;
  while (tmp_list) {
    GdkSuperWinTranslate *xlate = tmp_list->data;
    
    if (xlate->type == GDK_SUPERWIN_TRANSLATION && serial < xlate->serial ) {
      rect.x += xlate->data.translation.dx;
      rect.y += xlate->data.translation.dy;
    }
    tmp_list = tmp_list->next;
  }

  

  tmp_region = gdk_region_union_with_rect(*region, &rect);
  gdk_region_destroy(*region);
  *region = tmp_region;

 end:

  
  tmp_list = superwin->translate_queue;
  while (tmp_list) {
    GdkSuperWinTranslate *xlate = tmp_list->data;
    if (serial > xlate->serial) {
      GSList *tmp_link = tmp_list;
      tmp_list = tmp_list->next;
      superwin->translate_queue = g_slist_remove_link(superwin->translate_queue,
                                                      tmp_link);
      g_free(tmp_link->data);
      g_slist_free_1(tmp_link);
    }
    else {
      tmp_list = tmp_list->next;
    }
  }

  
  if (dont_recurse)
    return;

  
  while (XCheckTypedWindowEvent(xevent->xany.display,
                                xevent->xany.window,
                                Expose,
                                &extra_event) == True) {
    gdk_superwin_handle_expose(superwin, &extra_event, region, TRUE);
  }

  
  if (gdk_region_empty(*region) == FALSE) {
      GdkRectangle clip_box;
      gdk_region_get_clipbox(*region, &clip_box);
      if (superwin->paint_func)
        superwin->paint_func(clip_box.x, clip_box.y,
                             clip_box.width, clip_box.height,
                             superwin->func_data);
  }

}

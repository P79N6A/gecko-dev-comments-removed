






































#ifndef __GDK_SUPERWIN_H__
#define __GDK_SUPERWIN_H__

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtkobject.h>
#ifdef MOZILLA_CLIENT
#include "nscore.h"
#ifdef _IMPL_GTKSUPERWIN_API
#define GTKSUPERWIN_API(type) NS_EXPORT_(type)
#else
#define GTKSUPERWIN_API(type) NS_IMPORT_(type)
#endif
#else
#define GTKSUPERWIN_API(type) type
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GdkSuperWin GdkSuperWin;
typedef struct _GdkSuperWinClass GdkSuperWinClass;

#define GDK_TYPE_SUPERWIN            (gdk_superwin_get_type())
#define GDK_SUPERWIN(obj)            (GTK_CHECK_CAST((obj), GDK_TYPE_SUPERWIN, GdkSuperWin))
#define GDK_SUPERWIN_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GDK_TYPE_SUPERWIN, GdkSuperWinClass))
#define GDK_IS_SUPERWIN(obj)         (GTK_CHECK_TYPE((obj), GDK_TYPE_SUPERWIN))
#define GDK_IS_SUPERWIN_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), GDK_TYPE_SUPERWIN))

typedef void (*GdkSuperWinFunc) (GdkSuperWin *super_win,
                                 XEvent      *event,
                                 gpointer     data);

typedef void (*GdkSuperWinPaintFunc) (gint x, gint y,
                                      gint width, gint height,
                                      gpointer data);
typedef void (*GdkSuperWinPaintFlushFunc) (gpointer data);

typedef void (*GdkSuperWinKeyPressFunc) (XKeyEvent *event);
typedef void (*GdkSuperWinKeyReleaseFunc) (XKeyEvent *event);

struct _GdkSuperWin
{
  GtkObject object;
  GdkWindow *shell_window;
  GdkWindow *bin_window;

  
  GSList                    *translate_queue;
  GdkSuperWinFunc            shell_func;
  GdkSuperWinPaintFunc       paint_func;
  GdkSuperWinPaintFlushFunc  flush_func;
  GdkSuperWinKeyPressFunc    keyprs_func;
  GdkSuperWinKeyReleaseFunc  keyrel_func;
  gpointer                   func_data;
  GDestroyNotify             notify;

  GdkVisibilityState         visibility;
};

struct _GdkSuperWinClass
{
  GtkObjectClass object_class;
};

GTKSUPERWIN_API(GtkType) gdk_superwin_get_type(void);

GTKSUPERWIN_API(GdkSuperWin*) gdk_superwin_new (GdkWindow      *parent_window,
                                                guint           x,
                                                guint           y,
                                                guint           width,
                                                guint           height);

GTKSUPERWIN_API(void)  gdk_superwin_reparent(GdkSuperWin *superwin,
                                             GdkWindow   *parent_window);

GTKSUPERWIN_API(void)  
gdk_superwin_set_event_funcs (GdkSuperWin               *superwin,
                              GdkSuperWinFunc            shell_func,
                              GdkSuperWinPaintFunc       paint_func,
                              GdkSuperWinPaintFlushFunc  flush_func,
                              GdkSuperWinKeyPressFunc    keyprs_func,
                              GdkSuperWinKeyReleaseFunc  keyrel_func,
                              gpointer                   func_data,
                              GDestroyNotify             notify);

GTKSUPERWIN_API(void) gdk_superwin_scroll (GdkSuperWin *superwin,
                                           gint         dx,
                                           gint         dy);
GTKSUPERWIN_API(void) gdk_superwin_resize (GdkSuperWin *superwin,
                                           gint         width,
                                           gint         height);

#ifdef __cplusplus
}
#endif 

#endif


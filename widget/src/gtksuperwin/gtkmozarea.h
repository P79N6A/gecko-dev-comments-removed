






































#ifndef __GTK_MOZAREA_H__
#define __GTK_MOZAREA_H__

#include <gtk/gtkwindow.h>
#include "gdksuperwin.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GtkMozArea GtkMozArea;
typedef struct _GtkMozAreaClass GtkMozAreaClass;

#define GTK_TYPE_MOZAREA                  (gtk_mozarea_get_type ())
#define GTK_MOZAREA(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_MOZAREA, GtkMozArea))
#define GTK_MOZAREA_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_MOZAREA, GtkMozAreaClass))
#define GTK_IS_MOZAREA(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_MOZAREA))
#define GTK_IS_MOZAREA_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MOZAREA))

struct _GtkMozArea
{
  GtkWidget widget;
  GdkSuperWin *superwin;
  gboolean     toplevel_focus;

  
  GdkWindow *toplevel_window;
};
  
struct _GtkMozAreaClass
{
  GtkWindowClass window_class;

  
  void (* toplevel_focus_in ) (GtkMozArea *area);
  void (* toplevel_focus_out) (GtkMozArea *area);
  void (* toplevel_configure) (GtkMozArea *area);
};

GTKSUPERWIN_API(GtkType)    gtk_mozarea_get_type (void);
GTKSUPERWIN_API(GtkWidget*) gtk_mozarea_new ();
GTKSUPERWIN_API(gboolean)   gtk_mozarea_get_toplevel_focus(GtkMozArea *area);

#ifdef __cplusplus
}
#endif 

#endif

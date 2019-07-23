






































#ifndef __GTK_MOZBOX_H__
#define __GTK_MOZBOX_H__

#include <gtk/gtkwindow.h>
#include "gdksuperwin.h" 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GtkMozBox GtkMozBox;
typedef struct _GtkMozBoxClass GtkMozBoxClass;

#define GTK_TYPE_MOZBOX                  (gtk_mozbox_get_type ())
#define GTK_MOZBOX(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_MOZBOX, GtkMozBox))
#define GTK_MOZBOX_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_MOZBOX, GtkMozBoxClass))
#define GTK_IS_MOZBOX(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_MOZBOX))
#define GTK_IS_MOZBOX_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MOZBOX))

struct _GtkMozBox
{
  GtkWindow window;
  GdkWindow *parent_window;
  gint x;
  gint y;
};
  
struct _GtkMozBoxClass
{
  GtkWindowClass window_class;
};

GTKSUPERWIN_API(GtkType)    gtk_mozbox_get_type (void);
GTKSUPERWIN_API(GtkWidget*) gtk_mozbox_new (GdkWindow *parent_window);
GTKSUPERWIN_API(void)       gtk_mozbox_set_position (GtkMozBox *mozbox,
                                                     gint       x,
                                                     gint       y);

#ifdef __cplusplus
}
#endif 

#endif

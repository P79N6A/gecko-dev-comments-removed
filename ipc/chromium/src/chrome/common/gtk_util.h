



#ifndef CHROME_COMMON_GTK_UTIL_H_
#define CHROME_COMMON_GTK_UTIL_H_

#include <gtk/gtk.h>

#include "webkit/glue/window_open_disposition.h"

typedef struct _GdkPixbuf GdkPixbuf;
typedef struct _GtkWidget GtkWidget;
class SkBitmap;

namespace event_utils {




WindowOpenDisposition DispositionFromEventFlags(guint state);

}  

namespace gfx {



GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap* bitmap);



GtkWidget* CreateGtkBorderBin(GtkWidget* child, const GdkColor* color,
                              int top, int bottom, int left, int right);


void RemoveAllChildren(GtkWidget* container);

}  

#endif  





#include "chrome/common/gtk_util.h"

#include <gtk/gtk.h>

#include "base/linux_util.h"
#include "skia/include/SkBitmap.h"

namespace {


void RemoveWidget(GtkWidget* widget, gpointer container) {
  gtk_container_remove(GTK_CONTAINER(container), widget);
}

void FreePixels(guchar* pixels, gpointer data) {
  free(data);
}

}  

namespace event_utils {

WindowOpenDisposition DispositionFromEventFlags(guint event_flags) {
  if ((event_flags & GDK_BUTTON2_MASK) || (event_flags & GDK_CONTROL_MASK)) {
    return (event_flags & GDK_SHIFT_MASK) ?
        NEW_FOREGROUND_TAB : NEW_BACKGROUND_TAB;
  }

  if (event_flags & GDK_SHIFT_MASK)
    return NEW_WINDOW;
  return false  ? SAVE_TO_DISK : CURRENT_TAB;
}

}  

namespace gfx {

GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap* bitmap) {
  bitmap->lockPixels();
  int width = bitmap->width();
  int height = bitmap->height();
  int stride = bitmap->rowBytes();
  const guchar* orig_data = static_cast<guchar*>(bitmap->getPixels());
  guchar* data = base::BGRAToRGBA(orig_data, width, height, stride);

  
  
  GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(
      data,
      GDK_COLORSPACE_RGB,  
      true,  
      8,
      width, height, stride, &FreePixels, data);

  bitmap->unlockPixels();
  return pixbuf;
}

GtkWidget* CreateGtkBorderBin(GtkWidget* child, const GdkColor* color,
                              int top, int bottom, int left, int right) {
  
  
  
  GtkWidget* ebox = gtk_event_box_new();
  gtk_widget_modify_bg(ebox, GTK_STATE_NORMAL, color);
  GtkWidget* alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), top, bottom, left, right);
  gtk_container_add(GTK_CONTAINER(alignment), child);
  gtk_container_add(GTK_CONTAINER(ebox), alignment);
  return ebox;
}

void RemoveAllChildren(GtkWidget* container) {
  gtk_container_foreach(GTK_CONTAINER(container), RemoveWidget, container);
}

}  

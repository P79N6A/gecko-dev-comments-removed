



#include "chrome/common/owned_widget_gtk.h"

#include <gtk/gtk.h>

#include "base/logging.h"

OwnedWidgetGtk::~OwnedWidgetGtk() {
  DCHECK(!widget_) << "You must explicitly call OwnerWidgetGtk::Destroy().";
}

void OwnedWidgetGtk::Own(GtkWidget* widget) {
  DCHECK(!widget_);
  
  
  DCHECK(g_object_is_floating(widget));
  
  DCHECK(G_OBJECT(widget)->ref_count == 1);

  
  g_object_ref_sink(widget);
  widget_ = widget;
}

void OwnedWidgetGtk::Destroy() {
  if (!widget_)
    return;

  GtkWidget* widget = widget_;
  widget_ = NULL;
  gtk_widget_destroy(widget);

  DCHECK(!g_object_is_floating(widget));
  
  DCHECK(G_OBJECT(widget)->ref_count == 1);
  g_object_unref(widget);
}

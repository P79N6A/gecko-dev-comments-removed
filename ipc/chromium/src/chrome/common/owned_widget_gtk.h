















































#ifndef BASE_OWNED_WIDGET_GTK_H_
#define BASE_OWNED_WIDGET_GTK_H_

#include "base/basictypes.h"

typedef struct _GtkWidget GtkWidget;

class OwnedWidgetGtk {
 public:
  
  OwnedWidgetGtk() : widget_(NULL) { }
  
  explicit OwnedWidgetGtk(GtkWidget* widget) : widget_(NULL) { Own(widget); }

  ~OwnedWidgetGtk();

  
  GtkWidget* get() const { return widget_; }

  
  
  
  
  
  void Own(GtkWidget* widget);

  
  
  
  
  
  void Destroy();

 private:
  GtkWidget* widget_;

  DISALLOW_COPY_AND_ASSIGN(OwnedWidgetGtk);
};

#endif  

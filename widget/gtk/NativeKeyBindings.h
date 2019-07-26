




#ifndef mozilla_widget_NativeKeyBindings_h_
#define mozilla_widget_NativeKeyBindings_h_

#include <gtk/gtk.h>
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsIWidget.h"

namespace mozilla {
namespace widget {

class NativeKeyBindings MOZ_FINAL
{
  typedef nsIWidget::NativeKeyBindingsType NativeKeyBindingsType;
  typedef nsIWidget::DoCommandCallback DoCommandCallback;

public:
  static NativeKeyBindings* GetInstance(NativeKeyBindingsType aType);
  static void Shutdown();

  void Init(NativeKeyBindingsType aType);

  bool Execute(const WidgetKeyboardEvent& aEvent,
               DoCommandCallback aCallback,
               void* aCallbackData);

private:
  ~NativeKeyBindings();

  bool ExecuteInternal(const WidgetKeyboardEvent& aEvent,
                       DoCommandCallback aCallback,
                       void* aCallbackData,
                       guint aKeyval);

  GtkWidget* mNativeTarget;

  static NativeKeyBindings* sInstanceForSingleLineEditor;
  static NativeKeyBindings* sInstanceForMultiLineEditor;
};

} 
} 

#endif 

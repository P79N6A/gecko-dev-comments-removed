




#ifndef mozilla_widget_NativeKeyBindings_h_
#define mozilla_widget_NativeKeyBindings_h_

#import <Cocoa/Cocoa.h>
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsDataHashtable.h"
#include "nsIWidget.h"

namespace mozilla {
namespace widget {

typedef nsDataHashtable<nsPtrHashKey<struct objc_selector>, CommandInt>
  SelectorCommandHashtable;

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
  NativeKeyBindings();

  SelectorCommandHashtable mSelectorToCommand;

  static NativeKeyBindings* sInstanceForSingleLineEditor;
  static NativeKeyBindings* sInstanceForMultiLineEditor;
}; 

} 
} 

#endif 






#ifndef NativeKeyBindings_h_
#define NativeKeyBindings_h_

#include "nsINativeKeyBindings.h"
#include "nsIWidget.h"

#import <Cocoa/Cocoa.h>
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsDataHashtable.h"


#define NS_NATIVEKEYBINDINGS_INPUT_CID \
  { 0x8477f934, 0xfebf, 0x4c79, \
    { 0xb7, 0xfe, 0xbb, 0x7f, 0x9e, 0xbb, 0x9b, 0x4f } }


#define NS_NATIVEKEYBINDINGS_TEXTAREA_CID \
  { 0x13a6e56f, 0xf00b, 0x4e19, \
    { 0x8c, 0xf6, 0x1a, 0x51, 0xee, 0x7c, 0xc4, 0xbf } }


#define NS_NATIVEKEYBINDINGS_EDITOR_CID \
  { 0x36bfbd29, 0x4e02, 0x40f4, \
    { 0x8f, 0xff, 0x09, 0x4f, 0x1a, 0x9e, 0xc9, 0x7c } }

namespace mozilla {
namespace widget {

typedef nsDataHashtable<nsPtrHashKey<struct objc_selector>, CommandInt>
  SelectorCommandHashtable;

class NativeKeyBindings MOZ_FINAL : public nsINativeKeyBindings
{
  typedef nsIWidget::NativeKeyBindingsType NativeKeyBindingsType;
  typedef nsIWidget::DoCommandCallback DoCommandCallback;

public:
  static already_AddRefed<NativeKeyBindings>
    GetInstance(NativeKeyBindingsType aType);
  static void Shutdown();

  NativeKeyBindings();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(NativeKeyBindingsType aType);

  
  NS_IMETHOD_(bool) KeyPress(const WidgetKeyboardEvent& aEvent,
                             DoCommandCallback aCallback,
                             void* aCallbackData);

private:
  SelectorCommandHashtable mSelectorToCommand;

  static NativeKeyBindings* sInstanceForSingleLineEditor;
  static NativeKeyBindings* sInstanceForMultiLineEditor;
}; 

} 
} 

#endif 







































#ifndef nsINativeKeyBindings_h_
#define nsINativeKeyBindings_h_

#include "nsISupports.h"

#define NS_INATIVEKEYBINDINGS_IID \
{0x606c54e7, 0x0593, 0x4750, {0x99, 0xd9, 0x4e, 0x1b, 0xcc, 0xec, 0x98, 0xd9}}

#define NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX \
  "@mozilla.org/widget/native-key-bindings;1?type="

struct nsNativeKeyEvent
{
  PRUint32 keyCode;
  PRUint32 charCode;
  PRBool   altKey;
  PRBool   ctrlKey;
  PRBool   shiftKey;
  PRBool   metaKey;
};

class nsINativeKeyBindings : public nsISupports
{
 public:
  typedef void (*DoCommandCallback)(const char *, void*);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INATIVEKEYBINDINGS_IID)

  virtual NS_HIDDEN_(PRBool) KeyDown(const nsNativeKeyEvent& aEvent,
                                     DoCommandCallback aCallback,
                                     void *aCallbackData) = 0;

  virtual NS_HIDDEN_(PRBool) KeyPress(const nsNativeKeyEvent& aEvent,
                                      DoCommandCallback aCallback,
                                      void *aCallbackData) = 0;

  virtual NS_HIDDEN_(PRBool) KeyUp(const nsNativeKeyEvent& aEvent,
                                   DoCommandCallback aCallback,
                                   void *aCallbackData) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINativeKeyBindings, NS_INATIVEKEYBINDINGS_IID)

#endif

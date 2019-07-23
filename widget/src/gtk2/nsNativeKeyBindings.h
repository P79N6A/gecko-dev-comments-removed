





































#ifndef nsNativeKeyBindings_h_
#define nsNativeKeyBindings_h_


#ifdef KeyPress
#undef KeyPress
#endif

#include "nsINativeKeyBindings.h"
#include <gtk/gtkwidget.h>

enum NativeKeyBindingsType {
  eKeyBindings_Input,
  eKeyBindings_TextArea
};

#define NS_NATIVEKEYBINDINGSINPUT_CID \
{0x5c337258, 0xa580, 0x472e, {0x86, 0x15, 0xf2, 0x77, 0xdd, 0xc5, 0xbb, 0x06}}

#define NS_NATIVEKEYBINDINGSINPUT_CONTRACTID \
NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX "input"

#define NS_NATIVEKEYBINDINGSTEXTAREA_CID \
{0x2a898043, 0x180f, 0x4c8b, {0x8e, 0x54, 0x41, 0x0c, 0x7a, 0x54, 0x0f, 0x27}}

#define NS_NATIVEKEYBINDINGSTEXTAREA_CONTRACTID \
NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX "textarea"

#define NS_NATIVEKEYBINDINGSEDITOR_CID \
{0xf916ebfb, 0x78ef, 0x464b, {0x94, 0xd0, 0xa6, 0xf2, 0xca, 0x32, 0x00, 0xae}}

#define NS_NATIVEKEYBINDINGSEDITOR_CONTRACTID \
NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX "editor"

class nsNativeKeyBindings : public nsINativeKeyBindings
{
public:
  NS_HIDDEN_(void) Init(NativeKeyBindingsType aType);

  NS_DECL_ISUPPORTS

  
  virtual NS_HIDDEN_(PRBool) KeyDown(const nsNativeKeyEvent& aEvent,
                                     DoCommandCallback aCallback,
                                     void *aCallbackData);

  virtual NS_HIDDEN_(PRBool) KeyPress(const nsNativeKeyEvent& aEvent,
                                      DoCommandCallback aCallback,
                                      void *aCallbackData);

  virtual NS_HIDDEN_(PRBool) KeyUp(const nsNativeKeyEvent& aEvent,
                                   DoCommandCallback aCallback,
                                   void *aCallbackData);

private:
  ~nsNativeKeyBindings() NS_HIDDEN;

  GtkWidget *mNativeTarget;
};

#endif

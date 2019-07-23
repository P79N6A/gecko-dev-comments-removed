




































#ifndef nsIFocusEventSuppressor_h___
#define nsIFocusEventSuppressor_h___
#include "nsISupports.h"

typedef void (* nsFocusEventSuppressorCallback)(PRBool aSuppress);

#define NS_NSIFOCUSEVENTSUPPRESSORSERVICE_IID \
  { 0x8aae5cee, 0x59ab, 0x42d4, \
    { 0xa3, 0x76, 0xbf, 0x63, 0x54, 0x04, 0xc7, 0x98 } }

class nsIFocusEventSuppressorService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NSIFOCUSEVENTSUPPRESSORSERVICE_IID)
  virtual void AddObserverCallback(nsFocusEventSuppressorCallback aCallback) = 0;
  virtual void Suppress() = 0;
  virtual void Unsuppress() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFocusEventSuppressorService,
                              NS_NSIFOCUSEVENTSUPPRESSORSERVICE_IID)

#if defined(_IMPL_NS_LAYOUT) || defined(NS_STATIC_FOCUS_SUPPRESSOR)
void NS_SuppressFocusEvent();
void NS_UnsuppressFocusEvent();
void NS_AddFocusSuppressorCallback(nsFocusEventSuppressorCallback aCallback);
void NS_ShutdownFocusSuppressor();
#endif

#define NS_NSIFOCUSEVENTSUPPRESSORSERVICE_CID \
  { 0x35b2656c, 0x4102, 0x4bc1, \
    { 0x87, 0x6a, 0xfd, 0x6c, 0xb8, 0x30, 0x78, 0x7b } }

#define NS_NSIFOCUSEVENTSUPPRESSORSERVICE_CONTRACTID \
  "@mozilla.org/focus-event-suppressor-service;1"

#endif

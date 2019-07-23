






































#ifndef nsICSSLoaderObserver_h___
#define nsICSSLoaderObserver_h___

#include "nsISupports.h"

#define NS_ICSSLOADEROBSERVER_IID     \
{ 0xf5e8eb0f, 0x4c44, 0x49d5,       \
{0xb1, 0xe9, 0xab, 0x39, 0x23, 0x93, 0xc0, 0xf8} }

class nsICSSStyleSheet;

class nsICSSLoaderObserver : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSLOADEROBSERVER_IID)

  














  NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate,
                              nsresult aStatus) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSLoaderObserver, NS_ICSSLOADEROBSERVER_IID)

#endif 

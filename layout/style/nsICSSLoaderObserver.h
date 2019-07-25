






































#ifndef nsICSSLoaderObserver_h___
#define nsICSSLoaderObserver_h___

#include "nsISupports.h"

#define NS_ICSSLOADEROBSERVER_IID     \
{ 0x7eb90c74, 0xea0c, 0x4df5,       \
{0xa1, 0x5f, 0x95, 0xf0, 0x6a, 0x98, 0xb9, 0x40} }

class nsCSSStyleSheet;

class nsICSSLoaderObserver : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSLOADEROBSERVER_IID)

  














  NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet* aSheet, bool aWasAlternate,
                              nsresult aStatus) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSLoaderObserver, NS_ICSSLOADEROBSERVER_IID)

#endif 





































#ifndef nsSimpleChromeRegistry_h___
#define nsSimpleChromeRegistry_h___

#include "nsIChromeRegistry.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSLoader.h"

class nsSimpleChromeRegistry : public nsIChromeRegistry
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHROMEREGISTRY

  nsSimpleChromeRegistry();
  virtual ~nsSimpleChromeRegistry();
};

#endif

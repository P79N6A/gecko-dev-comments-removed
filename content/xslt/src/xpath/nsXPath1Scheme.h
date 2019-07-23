









































#ifndef __nsXPath1Scheme_h__
#define __nsXPath1Scheme_h__

#include "nsIXPointer.h"

class nsXPath1SchemeProcessor : public nsIXPointerSchemeProcessor
{
public:
  nsXPath1SchemeProcessor();
  virtual ~nsXPath1SchemeProcessor();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIXPOINTERSCHEMEPROCESSOR
};

#endif

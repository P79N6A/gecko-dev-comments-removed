




































#ifndef __nsXULMenuAccessibleWrap_h__
#define __nsXULMenuAccessibleWrap_h__

#include "nsXULMenuAccessible.h"

class nsXULMenuitemAccessibleWrap : public nsXULMenuitemAccessible
{
public:
  nsXULMenuitemAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsXULMenuitemAccessibleWrap() {}

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
};

#endif






































#ifndef __nsXULMenuAccessibleWrap_h__
#define __nsXULMenuAccessibleWrap_h__

#include "nsXULMenuAccessible.h"

class nsXULMenuitemAccessibleWrap : public nsXULMenuitemAccessible
{
public:
  nsXULMenuitemAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell);
  virtual ~nsXULMenuitemAccessibleWrap() {}
  NS_IMETHOD GetName(nsAString& aName);
};

#endif











































#ifndef _nsAccessibleWrap_H_
#define _nsAccessibleWrap_H_

#include "nsCOMPtr.h"
#include "nsAccessible.h"

class nsAccessibleWrap : public nsAccessible
{
  public: 
    nsAccessibleWrap(nsIDOMNode*, nsIWeakReference *aShell);
    virtual ~nsAccessibleWrap();

  protected:
    virtual nsresult FirePlatformEvent(nsIAccessibleEvent *aEvent) {
      return NS_OK;
    }
};

#endif










#ifndef _nsAccessibleWrap_H_
#define _nsAccessibleWrap_H_

#include "nsCOMPtr.h"
#include "nsAccessible.h"

class nsAccessibleWrap : public nsAccessible
{
public: 
  nsAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsAccessibleWrap();

  protected:
    virtual nsresult FirePlatformEvent(AccEvent* aEvent)
    {
      return NS_OK;
    }
};

#endif

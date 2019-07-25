








#ifndef _AccessibleWrap_H_
#define _AccessibleWrap_H_

#include "nsCOMPtr.h"
#include "Accessible.h"

class AccessibleWrap : public Accessible
{
public: 
  AccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~AccessibleWrap();

  protected:
    virtual nsresult FirePlatformEvent(AccEvent* aEvent)
    {
      return NS_OK;
    }
};

#endif

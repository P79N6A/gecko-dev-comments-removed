








#ifndef mozilla_a11y_AccessibleWrap_h_
#define mozilla_a11y_AccessibleWrap_h_

#include "nsCOMPtr.h"
#include "Accessible.h"

namespace mozilla {
namespace a11y {

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

} 
} 

#endif

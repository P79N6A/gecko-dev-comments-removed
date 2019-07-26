





#ifndef mozilla_dom_NonRefcountedDOMObject_h__
#define mozilla_dom_NonRefcountedDOMObject_h__

#include "nsTraceRefcnt.h"

namespace mozilla {
namespace dom {







class NonRefcountedDOMObject
{
protected:
  NonRefcountedDOMObject()
  {
    MOZ_COUNT_CTOR(NonRefcountedDOMObject);
  }
  ~NonRefcountedDOMObject()
  {
    MOZ_COUNT_DTOR(NonRefcountedDOMObject);
  }
};

} 
} 

#endif 

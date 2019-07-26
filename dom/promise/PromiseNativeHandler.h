





#ifndef mozilla_dom_PromiseNativeHandler_h
#define mozilla_dom_PromiseNativeHandler_h

#include "nsISupports.h"

namespace mozilla {
namespace dom {






class PromiseNativeHandler : public nsISupports
{
public:
  
  NS_DECL_ISUPPORTS

  virtual ~PromiseNativeHandler()
  { }

  virtual void
  ResolvedCallback(JS::Handle<JS::Value> aValue) = 0;

  virtual void
  RejectedCallback(JS::Handle<JS::Value> aValue) = 0;
};

} 
} 

#endif 

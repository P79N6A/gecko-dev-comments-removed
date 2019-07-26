





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
  ResolvedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) = 0;

  virtual void
  RejectedCallback(JSContext* aCx, JS::Handle<JS::Value> aValue) = 0;
};

} 
} 

#endif 

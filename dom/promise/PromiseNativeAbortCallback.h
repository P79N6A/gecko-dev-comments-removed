





#ifndef mozilla_dom_PromiseNativeAbortCallback_h
#define mozilla_dom_PromiseNativeAbortCallback_h

#include "nsISupports.h"

namespace mozilla {
namespace dom {





class PromiseNativeAbortCallback : public nsISupports
{
protected:
  virtual ~PromiseNativeAbortCallback()
  { }

public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(PromiseNativeAbortCallback)

  virtual void Call() = 0;
};

} 
} 

#endif 

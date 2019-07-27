





#ifndef mozilla_dom_AbortablePromise_h__
#define mozilla_dom_AbortablePromise_h__

#include "js/TypeDecls.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/CallbackObject.h"

namespace mozilla {
namespace dom {

class AbortCallback;
class PromiseNativeAbortCallback;

class AbortablePromise
  : public Promise
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AbortablePromise, Promise)

public:
  
  
  static already_AddRefed<AbortablePromise>
  Create(nsIGlobalObject* aGlobal, PromiseNativeAbortCallback& aAbortCallback,
         ErrorResult& aRv);

protected:
  
  AbortablePromise(nsIGlobalObject* aGlobal,
                   PromiseNativeAbortCallback& aAbortCallback);

  
  
  AbortablePromise(nsIGlobalObject* aGlobal);

  virtual ~AbortablePromise();

public:
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<AbortablePromise>
  Constructor(const GlobalObject& aGlobal, PromiseInit& aInit,
              AbortCallback& aAbortCallback, ErrorResult& aRv);

  void Abort();

private:
  void DoAbort();

  
  CallbackObjectHolder<AbortCallback,
                       PromiseNativeAbortCallback> mAbortCallback;
};

} 
} 

#endif 







#ifndef mozilla_dom_PromiseCallback_h
#define mozilla_dom_PromiseCallback_h

#include "mozilla/dom/Promise.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {



class PromiseCallback : public nsISupports
{
protected:
  virtual ~PromiseCallback();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(PromiseCallback)

  PromiseCallback();

  virtual nsresult Call(JSContext* aCx,
                        JS::Handle<JS::Value> aValue) = 0;

  
  
  virtual Promise* GetDependentPromise() = 0;

  enum Task {
    Resolve,
    Reject
  };

  
  static PromiseCallback*
  Factory(Promise* aNextPromise, JS::Handle<JSObject*> aObject,
          AnyCallback* aCallback, Task aTask);
};




class WrapperPromiseCallback final : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(WrapperPromiseCallback,
                                                         PromiseCallback)

  nsresult Call(JSContext* aCx,
                JS::Handle<JS::Value> aValue) override;

  Promise* GetDependentPromise() override
  {
    return mNextPromise;
  }

  WrapperPromiseCallback(Promise* aNextPromise, JS::Handle<JSObject*> aGlobal,
                         AnyCallback* aCallback);

private:
  ~WrapperPromiseCallback();

  nsRefPtr<Promise> mNextPromise;
  JS::Heap<JSObject*> mGlobal;
  nsRefPtr<AnyCallback> mCallback;
};



class ResolvePromiseCallback final : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(ResolvePromiseCallback,
                                                         PromiseCallback)

  nsresult Call(JSContext* aCx,
                JS::Handle<JS::Value> aValue) override;

  Promise* GetDependentPromise() override
  {
    return mPromise;
  }

  ResolvePromiseCallback(Promise* aPromise, JS::Handle<JSObject*> aGlobal);

private:
  ~ResolvePromiseCallback();

  nsRefPtr<Promise> mPromise;
  JS::Heap<JSObject*> mGlobal;
};



class RejectPromiseCallback final : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(RejectPromiseCallback,
                                                         PromiseCallback)

  nsresult Call(JSContext* aCx,
                JS::Handle<JS::Value> aValue) override;

  Promise* GetDependentPromise() override
  {
    return mPromise;
  }

  RejectPromiseCallback(Promise* aPromise, JS::Handle<JSObject*> aGlobal);

private:
  ~RejectPromiseCallback();

  nsRefPtr<Promise> mPromise;
  JS::Heap<JSObject*> mGlobal;
};


class NativePromiseCallback final : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(NativePromiseCallback,
                                           PromiseCallback)

  nsresult Call(JSContext* aCx,
                JS::Handle<JS::Value> aValue) override;

  Promise* GetDependentPromise() override
  {
    return nullptr;
  }

  NativePromiseCallback(PromiseNativeHandler* aHandler,
                        Promise::PromiseState aState);

private:
  ~NativePromiseCallback();

  nsRefPtr<PromiseNativeHandler> mHandler;
  Promise::PromiseState mState;
};

} 
} 

#endif 

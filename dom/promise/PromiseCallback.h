





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

  virtual void Call(JSContext* aCx,
                    JS::Handle<JS::Value> aValue) = 0;

  enum Task {
    Resolve,
    Reject
  };

  
  static PromiseCallback*
  Factory(Promise* aNextPromise, JS::Handle<JSObject*> aObject,
          AnyCallback* aCallback, Task aTask);
};




class WrapperPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(WrapperPromiseCallback,
                                                         PromiseCallback)

  void Call(JSContext* aCx,
            JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  WrapperPromiseCallback(Promise* aNextPromise, JS::Handle<JSObject*> aGlobal,
                         AnyCallback* aCallback);

private:
  ~WrapperPromiseCallback();

  nsRefPtr<Promise> mNextPromise;
  JS::Heap<JSObject*> mGlobal;
  nsRefPtr<AnyCallback> mCallback;
};



class ResolvePromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(ResolvePromiseCallback,
                                                         PromiseCallback)

  void Call(JSContext* aCx,
            JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  ResolvePromiseCallback(Promise* aPromise, JS::Handle<JSObject*> aGlobal);

private:
  ~ResolvePromiseCallback();

  nsRefPtr<Promise> mPromise;
  JS::Heap<JSObject*> mGlobal;
};



class RejectPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(RejectPromiseCallback,
                                                         PromiseCallback)

  void Call(JSContext* aCx,
            JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  RejectPromiseCallback(Promise* aPromise, JS::Handle<JSObject*> aGlobal);

private:
  ~RejectPromiseCallback();

  nsRefPtr<Promise> mPromise;
  JS::Heap<JSObject*> mGlobal;
};


class NativePromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(NativePromiseCallback,
                                           PromiseCallback)

  void Call(JSContext* aCx,
            JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

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

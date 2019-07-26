





#ifndef mozilla_dom_PromiseCallback_h
#define mozilla_dom_PromiseCallback_h

#include "mozilla/dom/Promise.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {



class PromiseCallback : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(PromiseCallback)

  PromiseCallback();
  virtual ~PromiseCallback();

  virtual void Call(JS::Handle<JS::Value> aValue) = 0;

  enum Task {
    Resolve,
    Reject
  };

  
  static PromiseCallback*
  Factory(Promise* aNextPromise, AnyCallback* aCallback, Task aTask);
};




class WrapperPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(WrapperPromiseCallback,
                                           PromiseCallback)

  void Call(JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  WrapperPromiseCallback(Promise* aNextPromise, AnyCallback* aCallback);
  ~WrapperPromiseCallback();

private:
  nsRefPtr<Promise> mNextPromise;
  nsRefPtr<AnyCallback> mCallback;
};


class SimpleWrapperPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SimpleWrapperPromiseCallback,
                                           PromiseCallback)

  void Call(JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  SimpleWrapperPromiseCallback(Promise* aPromise,
                               AnyCallback* aCallback);
  ~SimpleWrapperPromiseCallback();

private:
  nsRefPtr<Promise> mPromise;
  nsRefPtr<AnyCallback> mCallback;
};



class ResolvePromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ResolvePromiseCallback,
                                           PromiseCallback)

  void Call(JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  ResolvePromiseCallback(Promise* aPromise);
  ~ResolvePromiseCallback();

private:
  nsRefPtr<Promise> mPromise;
};



class RejectPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(RejectPromiseCallback,
                                           PromiseCallback)

  void Call(JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  RejectPromiseCallback(Promise* aPromise);
  ~RejectPromiseCallback();

private:
  nsRefPtr<Promise> mPromise;
};


class NativePromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(NativePromiseCallback,
                                           PromiseCallback)

  void Call(JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  NativePromiseCallback(PromiseNativeHandler* aHandler,
                        Promise::PromiseState aState);
  ~NativePromiseCallback();

private:
  nsRefPtr<PromiseNativeHandler> mHandler;
  Promise::PromiseState mState;
};

} 
} 

#endif 

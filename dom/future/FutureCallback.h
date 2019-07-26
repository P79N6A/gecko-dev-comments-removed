





#ifndef mozilla_dom_FutureCallback_h
#define mozilla_dom_FutureCallback_h

#include "mozilla/dom/Future.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {

class FutureResolver;



class FutureCallback : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(FutureCallback)

  FutureCallback();
  virtual ~FutureCallback();

  virtual void Call(const Optional<JS::Handle<JS::Value> >& aValue) = 0;

  enum Task {
    Resolve,
    Reject
  };

  
  static FutureCallback*
  Factory(FutureResolver* aNextResolver, AnyCallback* aCallback,
          Task aTask);
};




class WrapperFutureCallback MOZ_FINAL : public FutureCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(WrapperFutureCallback,
                                           FutureCallback)

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

  WrapperFutureCallback(FutureResolver* aNextResolver,
                        AnyCallback* aCallback);
  ~WrapperFutureCallback();

private:
  nsRefPtr<FutureResolver> mNextResolver;
  nsRefPtr<AnyCallback> mCallback;
};


class SimpleWrapperFutureCallback MOZ_FINAL : public FutureCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SimpleWrapperFutureCallback,
                                           FutureCallback)

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

  SimpleWrapperFutureCallback(Future* aFuture,
                              AnyCallback* aCallback);
  ~SimpleWrapperFutureCallback();

private:
  nsRefPtr<Future> mFuture;
  nsRefPtr<AnyCallback> mCallback;
};



class ResolveFutureCallback MOZ_FINAL : public FutureCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ResolveFutureCallback,
                                           FutureCallback)

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

  ResolveFutureCallback(FutureResolver* aResolver);
  ~ResolveFutureCallback();

private:
  nsRefPtr<FutureResolver> mResolver;
};



class RejectFutureCallback MOZ_FINAL : public FutureCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(RejectFutureCallback,
                                           FutureCallback)

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

  RejectFutureCallback(FutureResolver* aResolver);
  ~RejectFutureCallback();

private:
  nsRefPtr<FutureResolver> mResolver;
};

} 
} 

#endif 

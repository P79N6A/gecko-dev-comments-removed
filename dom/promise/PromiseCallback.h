





#ifndef mozilla_dom_PromiseCallback_h
#define mozilla_dom_PromiseCallback_h

#include "mozilla/dom/Promise.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {

class PromiseResolver;



class PromiseCallback : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(PromiseCallback)

  PromiseCallback();
  virtual ~PromiseCallback();

  virtual void Call(const Optional<JS::Handle<JS::Value> >& aValue) = 0;

  enum Task {
    Resolve,
    Reject
  };

  
  static PromiseCallback*
  Factory(PromiseResolver* aNextResolver, AnyCallback* aCallback,
          Task aTask);
};




class WrapperPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(WrapperPromiseCallback,
                                           PromiseCallback)

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

  WrapperPromiseCallback(PromiseResolver* aNextResolver,
                         AnyCallback* aCallback);
  ~WrapperPromiseCallback();

private:
  nsRefPtr<PromiseResolver> mNextResolver;
  nsRefPtr<AnyCallback> mCallback;
};


class SimpleWrapperPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SimpleWrapperPromiseCallback,
                                           PromiseCallback)

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

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

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

  ResolvePromiseCallback(PromiseResolver* aResolver);
  ~ResolvePromiseCallback();

private:
  nsRefPtr<PromiseResolver> mResolver;
};



class RejectPromiseCallback MOZ_FINAL : public PromiseCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(RejectPromiseCallback,
                                           PromiseCallback)

  void Call(const Optional<JS::Handle<JS::Value> >& aValue) MOZ_OVERRIDE;

  RejectPromiseCallback(PromiseResolver* aResolver);
  ~RejectPromiseCallback();

private:
  nsRefPtr<PromiseResolver> mResolver;
};

} 
} 

#endif 

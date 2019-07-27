





#ifndef __DetailedPromise_h__
#define __DetailedPromise_h__

#include "mozilla/dom/Promise.h"

namespace mozilla {
namespace dom {






class DetailedPromise : public Promise
{
public:
  static already_AddRefed<DetailedPromise>
  Create(nsIGlobalObject* aGlobal, ErrorResult& aRv);

  template <typename T>
  void MaybeResolve(const T& aArg)
  {
    mResponded = true;
    Promise::MaybeResolve<T>(aArg);
  }

  void MaybeReject(nsresult aArg) = delete;
  void MaybeReject(nsresult aArg, const nsACString& aReason);

  void MaybeReject(ErrorResult& aArg) = delete;
  void MaybeReject(ErrorResult&, const nsACString& aReason);

private:
  explicit DetailedPromise(nsIGlobalObject* aGlobal);
  virtual ~DetailedPromise();

  bool mResponded;
};

}
}

#endif 

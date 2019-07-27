



#ifndef mozilla_dom_IccCardLockError_h
#define mozilla_dom_IccCardLockError_h

#include "mozilla/dom/DOMError.h"

namespace mozilla {
namespace dom {

class IccCardLockError MOZ_FINAL : public DOMError
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  IccCardLockError(nsPIDOMWindow* aWindow, const nsAString& aName,
                   int16_t aRetryCount);

  static already_AddRefed<IccCardLockError>
  Constructor(const GlobalObject& aGlobal, const nsAString& aName,
              int16_t aRetryCount, ErrorResult& aRv);

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  

  int16_t
  RetryCount() const
  {
    return mRetryCount;
  }

private:
  ~IccCardLockError() {}

private:
  int16_t mRetryCount;
};

} 
} 

#endif 

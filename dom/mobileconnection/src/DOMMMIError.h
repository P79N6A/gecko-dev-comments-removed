





#ifndef mozilla_dom_MmiError_h
#define mozilla_dom_MmiError_h

#include "mozilla/dom/DOMError.h"

namespace mozilla {
namespace dom {

class DOMMMIError MOZ_FINAL : public DOMError
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMMMIError, DOMError)

  DOMMMIError(nsPIDOMWindow* aWindow, const nsAString& aName,
              const nsAString& aMessage, const nsAString& aServiceCode,
              const Nullable<int16_t>& aInfo);

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  static already_AddRefed<DOMMMIError>
  Constructor(const GlobalObject& aGlobal, const nsAString& aServiceCode,
              const nsAString& aName, const nsAString& aMessage,
              const Nullable<int16_t>& aInfo, ErrorResult& aRv);

  void
  GetServiceCode(nsString& aServiceCode) const
  {
    aServiceCode = mServiceCode;
  }

  Nullable<int16_t>
  GetAdditionalInformation() const
  {
    return mInfo;
  }

private:
  ~DOMMMIError() {}

private:
  nsString mServiceCode;
  Nullable<int16_t> mInfo;
};

} 
} 

#endif 

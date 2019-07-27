





#ifndef mozilla_dom_TelephonyCallId_h
#define mozilla_dom_TelephonyCallId_h

#include "mozilla/dom/TelephonyCallIdBinding.h"
#include "mozilla/dom/telephony/TelephonyCommon.h"

#include "nsWrapperCache.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class TelephonyCallId MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TelephonyCallId)

  TelephonyCallId(nsPIDOMWindow* aWindow, const nsAString& aNumber,
                  uint16_t aNumberPresentation, const nsAString& aName,
                  uint16_t aNamePresentation);

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  

  void
  GetNumber(nsString& aNumber) const
  {
    aNumber.Assign(mNumber);
  }

  CallIdPresentation
  NumberPresentation() const;

  void
  GetName(nsString& aName) const
  {
    aName.Assign(mName);
  }

  CallIdPresentation
  NamePresentation() const;

  void
  UpdateNumber(const nsAString& aNumber)
  {
    mNumber = aNumber;
  }

private:
  ~TelephonyCallId();

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsString mNumber;
  uint16_t mNumberPresentation;
  nsString mName;
  uint16_t mNamePresentation;

  CallIdPresentation
  GetPresentationStr(uint16_t aPresentation) const;
};

} 
} 

#endif 

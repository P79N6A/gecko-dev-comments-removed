





#ifndef mozilla_dom_MobileNetworkInfo_h
#define mozilla_dom_MobileNetworkInfo_h

#include "mozilla/dom/MozMobileNetworkInfoBinding.h"
#include "nsIMobileNetworkInfo.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class MobileNetworkInfo final : public nsIMobileNetworkInfo
                              , public nsWrapperCache
{
public:
  NS_DECL_NSIMOBILENETWORKINFO
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MobileNetworkInfo)

  explicit MobileNetworkInfo(nsPIDOMWindow* aWindow);

  MobileNetworkInfo(const nsAString& aShortName, const nsAString& aLongName,
                    const nsAString& aMcc, const nsAString& aMnc,
                    const nsAString& aState);

  void
  Update(nsIMobileNetworkInfo* aInfo);

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  Nullable<MobileNetworkState>
  GetState() const
  {
    uint32_t i = 0;
    for (const EnumEntry* entry = MobileNetworkStateValues::strings;
         entry->value;
         ++entry, ++i) {
      if (mState.EqualsASCII(entry->value)) {
        return Nullable<MobileNetworkState>(static_cast<MobileNetworkState>(i));
      }
    }

    return Nullable<MobileNetworkState>();
  }

private:
  ~MobileNetworkInfo() {}

private:
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsString mShortName;
  nsString mLongName;
  nsString mMcc;
  nsString mMnc;
  nsString mState;
};

} 
} 

#endif 

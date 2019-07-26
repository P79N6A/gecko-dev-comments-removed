



#ifndef mozilla_dom_gamepad_Gamepad_h
#define mozilla_dom_gamepad_Gamepad_h

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/GamepadButton.h"
#include <stdint.h>
#include "nsCOMPtr.h"
#include "nsIVariant.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

enum GamepadMappingType
{
  NoMapping = 0,
  StandardMapping = 1
};

class Gamepad : public nsISupports,
                public nsWrapperCache
{
public:
  Gamepad(nsISupports* aParent,
          const nsAString& aID, uint32_t aIndex,
          GamepadMappingType aMapping,
          uint32_t aNumButtons, uint32_t aNumAxes);
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Gamepad)

  void SetConnected(bool aConnected);
  void SetButton(uint32_t aButton, bool aPressed, double aValue);
  void SetAxis(uint32_t aAxis, double aValue);
  void SetIndex(uint32_t aIndex);

  
  void SyncState(Gamepad* aOther);

  
  
  already_AddRefed<Gamepad> Clone(nsISupports* aParent);

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
			       JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void GetId(nsAString& aID) const
  {
    aID = mID;
  }

  void GetMapping(nsAString& aMapping) const
  {
    if (mMapping == StandardMapping) {
      aMapping = NS_LITERAL_STRING("standard");
    } else {
      aMapping = NS_LITERAL_STRING("");
    }
  }

  bool Connected() const
  {
    return mConnected;
  }

  uint32_t Index() const
  {
    return mIndex;
  }

  already_AddRefed<nsIVariant> GetButtons(mozilla::ErrorResult& aRv)
  {
    nsCOMPtr<nsIVariant> buttons;
    aRv = GetButtons(getter_AddRefs(buttons));
    return buttons.forget();
  }

  already_AddRefed<nsIVariant> GetAxes(mozilla::ErrorResult& aRv)
  {
    nsCOMPtr<nsIVariant> axes;
    aRv = GetAxes(getter_AddRefs(axes));
    return axes.forget();
  }

private:
  virtual ~Gamepad() {}

  nsresult GetButtons(nsIVariant** aButtons);
  nsresult GetAxes(nsIVariant** aAxes);

protected:
  nsCOMPtr<nsISupports> mParent;
  nsString mID;
  uint32_t mIndex;

  
  GamepadMappingType mMapping;

  
  bool mConnected;

  
  nsTArray<nsRefPtr<GamepadButton>> mButtons;
  nsTArray<double> mAxes;

  
  nsCOMPtr<nsIVariant> mButtonsVariant;
};

} 
} 

#endif 

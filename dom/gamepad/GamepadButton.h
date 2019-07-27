



#ifndef mozilla_dom_gamepad_GamepadButton_h
#define mozilla_dom_gamepad_GamepadButton_h

#include <stdint.h>
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class GamepadButton : public nsISupports,
                      public nsWrapperCache
{
public:
  GamepadButton(nsISupports* aParent) : mParent(aParent),
                                        mPressed(false),
                                        mValue(0)
  {
    SetIsDOMBinding();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(GamepadButton)

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void SetPressed(bool aPressed)
  {
    mPressed = aPressed;
  }

  void SetValue(double aValue)
  {
    mValue = aValue;
  }

  bool Pressed() const
  {
    return mPressed;
  }

  double Value() const
  {
    return mValue;
  }

private:
  virtual ~GamepadButton() {}

protected:
  nsCOMPtr<nsISupports> mParent;
  bool mPressed;
  double mValue;
};

} 
} 

#endif 





#include "Gamepad.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsVariant.h"
#include "mozilla/dom/GamepadBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(Gamepad)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Gamepad)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Gamepad)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(Gamepad, mParent, mButtonsVariant)

Gamepad::Gamepad(nsISupports* aParent,
                 const nsAString& aID, uint32_t aIndex,
                 GamepadMappingType aMapping,
                 uint32_t aNumButtons, uint32_t aNumAxes)
  : mParent(aParent),
    mID(aID),
    mIndex(aIndex),
    mMapping(aMapping),
    mConnected(true),
    mButtons(aNumButtons),
    mAxes(aNumAxes)
{
  SetIsDOMBinding();
  for (unsigned i = 0; i < aNumButtons; i++) {
    mButtons.InsertElementAt(i, new GamepadButton(mParent));
  }
  mAxes.InsertElementsAt(0, aNumAxes, 0.0f);
}

void
Gamepad::SetIndex(uint32_t aIndex)
{
  mIndex = aIndex;
}

void
Gamepad::SetConnected(bool aConnected)
{
  mConnected = aConnected;
}

void
Gamepad::SetButton(uint32_t aButton, bool aPressed, double aValue)
{
  MOZ_ASSERT(aButton < mButtons.Length());
  mButtons[aButton]->SetPressed(aPressed);
  mButtons[aButton]->SetValue(aValue);
}

void
Gamepad::SetAxis(uint32_t aAxis, double aValue)
{
  MOZ_ASSERT(aAxis < mAxes.Length());
  mAxes[aAxis] = aValue;
}

nsresult
Gamepad::GetButtons(nsIVariant** aButtons)
{
  if (mButtonsVariant) {
    NS_ADDREF(*aButtons = mButtonsVariant);
    return NS_OK;
  }

  nsRefPtr<nsVariant> out = new nsVariant();
  NS_ENSURE_STATE(out);

  if (mButtons.Length() == 0) {
    nsresult rv = out->SetAsEmptyArray();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    GamepadButton** array = reinterpret_cast<GamepadButton**>
                      (NS_Alloc(mButtons.Length() * sizeof(GamepadButton*)));
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    for (uint32_t i = 0; i < mButtons.Length(); ++i) {
      array[i] = mButtons[i].get();
    }

    nsresult rv = out->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                                  nullptr,
                                  mButtons.Length(),
                                  reinterpret_cast<void*>(array));
    NS_Free(array);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mButtonsVariant = out;
  *aButtons = out.forget().get();
  return NS_OK;
}

nsresult
Gamepad::GetAxes(nsIVariant** aAxes)
{
  nsRefPtr<nsVariant> out = new nsVariant();
  NS_ENSURE_STATE(out);

  if (mAxes.Length() == 0) {
    nsresult rv = out->SetAsEmptyArray();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    double* array = reinterpret_cast<double*>
                              (NS_Alloc(mAxes.Length() * sizeof(double)));
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    for (uint32_t i = 0; i < mAxes.Length(); ++i) {
      array[i] = mAxes[i];
    }

    nsresult rv = out->SetAsArray(nsIDataType::VTYPE_DOUBLE,
                                  nullptr,
                                  mAxes.Length(),
                                  reinterpret_cast<void*>(array));
    NS_Free(array);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aAxes = out.forget().get();
  return NS_OK;
}

void
Gamepad::SyncState(Gamepad* aOther)
{
  if (mButtons.Length() != aOther->mButtons.Length() ||
      mAxes.Length() != aOther->mAxes.Length()) {
    return;
  }

  mConnected = aOther->mConnected;
  for (uint32_t i = 0; i < mButtons.Length(); ++i) {
    mButtons[i]->SetPressed(aOther->mButtons[i]->Pressed());
    mButtons[i]->SetValue(aOther->mButtons[i]->Value());
  }
  for (uint32_t i = 0; i < mAxes.Length(); ++i) {
    mAxes[i] = aOther->mAxes[i];
  }
}

already_AddRefed<Gamepad>
Gamepad::Clone(nsISupports* aParent)
{
  nsRefPtr<Gamepad> out =
    new Gamepad(aParent, mID, mIndex, mMapping,
                mButtons.Length(), mAxes.Length());
  out->SyncState(this);
  return out.forget();
}

 JSObject*
Gamepad::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return GamepadBinding::Wrap(aCx, aScope, this);
}

} 
} 

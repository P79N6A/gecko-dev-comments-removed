





#include "ColorPickerParent.h"
#include "nsComponentManagerUtils.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "mozilla/unused.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/TabParent.h"

using mozilla::unused;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(ColorPickerParent::ColorPickerShownCallback,
                  nsIColorPickerShownCallback);

NS_IMETHODIMP
ColorPickerParent::ColorPickerShownCallback::Update(const nsAString& aColor)
{
  if (mColorPickerParent) {
    unused << mColorPickerParent->SendUpdate(nsString(aColor));
  }
  return NS_OK;
}

NS_IMETHODIMP
ColorPickerParent::ColorPickerShownCallback::Done(const nsAString& aColor)
{
  if (mColorPickerParent) {
    unused << mColorPickerParent->Send__delete__(mColorPickerParent,
                                                 nsString(aColor));
  }
  return NS_OK;
}

void
ColorPickerParent::ColorPickerShownCallback::Destroy()
{
  mColorPickerParent = nullptr;
}

bool
ColorPickerParent::CreateColorPicker()
{
  mPicker = do_CreateInstance("@mozilla.org/colorpicker;1");
  if (!mPicker) {
    return false;
  }

  Element* ownerElement = static_cast<TabParent*>(Manager())->GetOwnerElement();
  if (!ownerElement) {
    return false;
  }

  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(ownerElement->OwnerDoc()->GetWindow());
  if (!window) {
    return false;
  }

  return NS_SUCCEEDED(mPicker->Init(window, mTitle, mInitialColor));
}

bool
ColorPickerParent::RecvOpen()
{
  if (!CreateColorPicker()) {
    unused << Send__delete__(this, mInitialColor);
    return true;
  }

  mCallback = new ColorPickerShownCallback(this);

  mPicker->Open(mCallback);
  return true;
};

void
ColorPickerParent::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mCallback) {
    mCallback->Destroy();
  }
}

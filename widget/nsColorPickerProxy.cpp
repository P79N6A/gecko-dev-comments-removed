





#include "nsColorPickerProxy.h"

#include "mozilla/dom/TabChild.h"

using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(nsColorPickerProxy, nsIColorPicker)


NS_IMETHODIMP
nsColorPickerProxy::Init(nsIDOMWindow* aParent, const nsAString& aTitle,
                         const nsAString& aInitialColor)
{
  TabChild* tabChild = TabChild::GetFrom(aParent);
  if (!tabChild) {
    return NS_ERROR_FAILURE;
  }

  tabChild->SendPColorPickerConstructor(this,
                                        nsString(aTitle),
                                        nsString(aInitialColor));
  NS_ADDREF_THIS();
  return NS_OK;
}


NS_IMETHODIMP
nsColorPickerProxy::Open(nsIColorPickerShownCallback* aColorPickerShownCallback)
{
  NS_ENSURE_STATE(!mCallback);
  mCallback = aColorPickerShownCallback;

  SendOpen();
  return NS_OK;
}

bool
nsColorPickerProxy::RecvUpdate(const nsString& aColor)
{
  if (mCallback) {
    mCallback->Update(aColor);
  }
  return true;
}

bool
nsColorPickerProxy::Recv__delete__(const nsString& aColor)
{
  if (mCallback) {
    mCallback->Done(aColor);
    mCallback = nullptr;
  }
  return true;
}
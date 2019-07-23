




































#include "nsWidgetSupport.h"
#include "nsRect.h"
#include "nsIWidget.h"

#ifdef XP_MAC
#define WIDGET_SUPPORT_EXPORT(returnType) \
        PR_PUBLIC_API(returnType)
#else
#define WIDGET_SUPPORT_EXPORT(returnType) \
        returnType
#endif

WIDGET_SUPPORT_EXPORT(nsresult)
NS_ShowWidget(nsISupports* aWidget, PRBool aShow)
{
  nsCOMPtr<nsIWidget> widget = do_QueryInterface(aWidget);
  if (widget) {
    widget->Show(aShow);
  }

  return NS_OK;
}

WIDGET_SUPPORT_EXPORT(nsresult)
NS_MoveWidget(nsISupports* aWidget, PRUint32 aX, PRUint32 aY)
{
  nsCOMPtr<nsIWidget> widget = do_QueryInterface(aWidget);
  if (widget) {
    widget->Move(aX, aY);
  }

  return NS_OK;
}

WIDGET_SUPPORT_EXPORT(nsresult)
NS_EnableWidget(nsISupports* aWidget, PRBool aEnable)
{
  nsCOMPtr<nsIWidget> widget = do_QueryInterface(aWidget);
  if (widget) {
    widget->Enable(aEnable);
  }

  return NS_OK;
}

WIDGET_SUPPORT_EXPORT(nsresult)
NS_SetFocusToWidget(nsISupports* aWidget)
{
  nsCOMPtr<nsIWidget> widget = do_QueryInterface(aWidget);
  if (widget) {
    widget->SetFocus();
  }

  return NS_OK;
}

WIDGET_SUPPORT_EXPORT(nsresult)
NS_GetWidgetNativeData(nsISupports* aWidget, void** aNativeData)
{
  void *result = nsnull;
  nsCOMPtr<nsIWidget> widget = do_QueryInterface(aWidget);
  if (widget) {
    result = widget->GetNativeData(NS_NATIVE_WIDGET);
  }

  *aNativeData = result;

  return NS_OK;
}

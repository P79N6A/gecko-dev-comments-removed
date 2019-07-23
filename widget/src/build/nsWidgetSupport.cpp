





































#include "widgetCore.h"
#include "nsWidgetSupport.h"
#include "nsRect.h"
#include "nsIAppShell.h"
#include "nsIEventListener.h"
#include "nsILookAndFeel.h"
#include "nsIMouseListener.h"
#include "nsIToolkit.h"
#include "nsIWidget.h"


static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);


extern NS_WIDGET nsresult 
NS_ShowWidget(nsISupports* aWidget, PRBool aShow)
{

  nsIWidget* 	widget = nsnull;
	if (NS_OK == aWidget->QueryInterface(kIWidgetIID,(void**)&widget)) {
	  widget->Show(aShow);
	  NS_IF_RELEASE(widget);
	}

  return NS_OK;
}

extern NS_WIDGET nsresult 
NS_MoveWidget(nsISupports* aWidget, PRUint32 aX, PRUint32 aY)
{

  nsIWidget* 	widget = nsnull;
	if (NS_OK == aWidget->QueryInterface(kIWidgetIID,(void**)&widget)) {
	  widget->Move(aX,aY);
	  NS_IF_RELEASE(widget);
	}
  return NS_OK;
}

extern NS_WIDGET nsresult 
NS_EnableWidget(nsISupports* aWidget, PRBool aEnable)
{
	nsIWidget* 	widget;
	if (NS_OK == aWidget->QueryInterface(kIWidgetIID,(void**)&widget))
	{
		widget->Enable(aEnable);
		NS_RELEASE(widget);
	}
  return NS_OK;
}


extern NS_WIDGET nsresult 
NS_SetFocusToWidget(nsISupports* aWidget)
{

  nsIWidget* 	widget = nsnull;
	if (NS_OK == aWidget->QueryInterface(kIWidgetIID,(void**)&widget)) {
	  widget->SetFocus();
	  NS_IF_RELEASE(widget);
	}
  return NS_OK;
}


extern NS_WIDGET nsresult 
NS_GetWidgetNativeData(nsISupports* aWidget, void** aNativeData)
{
	void* 			result = nsnull;
	nsIWidget* 	widget;
	if (NS_OK == aWidget->QueryInterface(kIWidgetIID,(void**)&widget))
	{
		result = widget->GetNativeData(NS_NATIVE_WIDGET);
		NS_RELEASE(widget);
	}
	*aNativeData = result;
  return NS_OK;

}

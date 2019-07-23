





































#include "nscore.h"
#include "nsScreen.h"
#include "nsPIDOMWindow.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDeviceContext.h"
#include "nsIWidget.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "nsIInterfaceRequestorUtils.h"




nsScreen::nsScreen(nsIDocShell* aDocShell)
  : mDocShell(aDocShell)
{
}

nsScreen::~nsScreen()
{
}



NS_INTERFACE_MAP_BEGIN(nsScreen)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScreen)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Screen)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsScreen)
NS_IMPL_RELEASE(nsScreen)


NS_IMETHODIMP
nsScreen::SetDocShell(nsIDocShell* aDocShell)
{
   mDocShell = aDocShell; 
   return NS_OK;
}

NS_IMETHODIMP
nsScreen::GetTop(PRInt32* aTop)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aTop = rect.y;

  return rv;
}


NS_IMETHODIMP
nsScreen::GetLeft(PRInt32* aLeft)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aLeft = rect.x;

  return rv;
}


NS_IMETHODIMP
nsScreen::GetWidth(PRInt32* aWidth)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aWidth = rect.width;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetHeight(PRInt32* aHeight)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aHeight = rect.height;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetPixelDepth(PRInt32* aPixelDepth)
{
  nsIDeviceContext* context = GetDeviceContext();

  if (!context) {
    *aPixelDepth = -1;

    return NS_ERROR_FAILURE;
  }

  PRUint32 depth;
  context->GetDepth(depth);

  *aPixelDepth = depth;

  return NS_OK;
}

NS_IMETHODIMP
nsScreen::GetColorDepth(PRInt32* aColorDepth)
{
  return GetPixelDepth(aColorDepth);
}

NS_IMETHODIMP
nsScreen::GetAvailWidth(PRInt32* aAvailWidth)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailWidth = rect.width;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetAvailHeight(PRInt32* aAvailHeight)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailHeight = rect.height;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetAvailLeft(PRInt32* aAvailLeft)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailLeft = rect.x;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetAvailTop(PRInt32* aAvailTop)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailTop = rect.y;

  return rv;
}

nsIDeviceContext*
nsScreen::GetDeviceContext()
{
  nsCOMPtr<nsIDocShell> docShell = mDocShell;
  while (docShell) {
    
    
    
    
    nsCOMPtr<nsPIDOMWindow> win = do_GetInterface(docShell);
    if (!win) {
      
      return nsnull;
    }

    win->EnsureSizeUpToDate();

    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(docShell);
    NS_ENSURE_TRUE(baseWindow, nsnull);

    nsCOMPtr<nsIWidget> mainWidget;
    baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
    if (mainWidget) {
      return mainWidget->GetDeviceContext();
    }

    nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(docShell);
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    curItem->GetParent(getter_AddRefs(parentItem));
    docShell = do_QueryInterface(parentItem);
  }

  return nsnull;
}

nsresult
nsScreen::GetRect(nsRect& aRect)
{
  nsIDeviceContext *context = GetDeviceContext();

  if (!context) {
    return NS_ERROR_FAILURE;
  }

  context->GetRect(aRect);

  aRect.x = nsPresContext::AppUnitsToIntCSSPixels(aRect.x);
  aRect.y = nsPresContext::AppUnitsToIntCSSPixels(aRect.y);
  aRect.height = nsPresContext::AppUnitsToIntCSSPixels(aRect.height);
  aRect.width = nsPresContext::AppUnitsToIntCSSPixels(aRect.width);

  return NS_OK;
}

nsresult
nsScreen::GetAvailRect(nsRect& aRect)
{
  nsIDeviceContext *context = GetDeviceContext();

  if (!context) {
    return NS_ERROR_FAILURE;
  }

  context->GetClientRect(aRect);

  aRect.x = nsPresContext::AppUnitsToIntCSSPixels(aRect.x);
  aRect.y = nsPresContext::AppUnitsToIntCSSPixels(aRect.y);
  aRect.height = nsPresContext::AppUnitsToIntCSSPixels(aRect.height);
  aRect.width = nsPresContext::AppUnitsToIntCSSPixels(aRect.width);

  return NS_OK;
}

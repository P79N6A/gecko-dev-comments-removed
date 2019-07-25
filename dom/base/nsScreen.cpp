





































#include "mozilla/Hal.h"
#include "nsScreen.h"
#include "nsIDocShell.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfoID.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsLayoutUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

 bool nsScreen::sInitialized = false;
 bool nsScreen::sAllowScreenEnabledProperty = false;
 bool nsScreen::sAllowScreenBrightnessProperty = false;

 void
nsScreen::Initialize()
{
  MOZ_ASSERT(!sInitialized);
  sInitialized = true;
  Preferences::AddBoolVarCache(&sAllowScreenEnabledProperty,
                               "dom.screenEnabledProperty.enabled");
  Preferences::AddBoolVarCache(&sAllowScreenBrightnessProperty,
                               "dom.screenBrightnessProperty.enabled");
}




nsScreen::nsScreen(nsIDocShell* aDocShell)
  : mDocShell(aDocShell)
{
  if (!sInitialized) {
    Initialize();
  }
}

nsScreen::~nsScreen()
{
}


DOMCI_DATA(Screen, nsScreen)


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
  nsDeviceContext* context = GetDeviceContext();

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

nsDeviceContext*
nsScreen::GetDeviceContext()
{
  return nsLayoutUtils::GetDeviceContextForScreenInfo(mDocShell);
}

nsresult
nsScreen::GetRect(nsRect& aRect)
{
  nsDeviceContext *context = GetDeviceContext();

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
  nsDeviceContext *context = GetDeviceContext();

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

namespace {

bool
IsChromeType(nsIDocShell *aDocShell)
{
  nsCOMPtr<nsIDocShellTreeItem> ds = do_QueryInterface(aDocShell);
  if (!ds) {
    return false;
  }

  PRInt32 itemType;
  ds->GetItemType(&itemType);
  return itemType == nsIDocShellTreeItem::typeChrome;
}

} 

nsresult
nsScreen::GetMozEnabled(bool *aEnabled)
{
  if (!sAllowScreenEnabledProperty || !IsChromeType(mDocShell)) {
    *aEnabled = true;
    return NS_OK;
  }

  *aEnabled = hal::GetScreenEnabled();
  return NS_OK;
}

nsresult
nsScreen::SetMozEnabled(bool aEnabled)
{
  if (!sAllowScreenEnabledProperty || !IsChromeType(mDocShell)) {
    return NS_OK;
  }

  
  
  hal::SetScreenEnabled(aEnabled);
  return NS_OK;
}

nsresult
nsScreen::GetMozBrightness(double *aBrightness)
{
  if (!sAllowScreenBrightnessProperty || !IsChromeType(mDocShell)) {
    *aBrightness = 1;
    return NS_OK;
  }

  *aBrightness = hal::GetScreenBrightness();
  return NS_OK;
}

nsresult
nsScreen::SetMozBrightness(double aBrightness)
{
  if (!sAllowScreenBrightnessProperty || !IsChromeType(mDocShell)) {
    return NS_OK;
  }

  NS_ENSURE_TRUE(0 <= aBrightness && aBrightness <= 1, NS_ERROR_INVALID_ARG);
  hal::SetScreenBrightness(aBrightness);
  return NS_OK;
}

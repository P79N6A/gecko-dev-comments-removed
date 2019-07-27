





#include "mozilla/unused.h"
#include "nsIAppShell.h"
#include "nsScreenManagerProxy.h"
#include "nsServiceManagerUtils.h"
#include "nsWidgetsCID.h"
#include "ScreenProxy.h"

namespace mozilla {
namespace widget {

using namespace mozilla::dom;

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

ScreenProxy::ScreenProxy(nsScreenManagerProxy* aScreenManager, ScreenDetails aDetails)
  : mScreenManager(aScreenManager)
  , mCacheValid(false)
  , mCacheWillInvalidate(false)
{
  PopulateByDetails(aDetails);
}

NS_IMETHODIMP
ScreenProxy::GetId(uint32_t *outId)
{
  *outId = mId;
  return NS_OK;
}

NS_IMETHODIMP
ScreenProxy::GetRect(int32_t *outLeft,
                     int32_t *outTop,
                     int32_t *outWidth,
                     int32_t *outHeight)
{
  if (!EnsureCacheIsValid()) {
    return NS_ERROR_FAILURE;
  }

  *outLeft = mRect.x;
  *outTop = mRect.y;
  *outWidth = mRect.width;
  *outHeight = mRect.height;
  return NS_OK;
}

NS_IMETHODIMP
ScreenProxy::GetAvailRect(int32_t *outLeft,
                          int32_t *outTop,
                          int32_t *outWidth,
                          int32_t *outHeight)
{
  if (!EnsureCacheIsValid()) {
    return NS_ERROR_FAILURE;
  }

  *outLeft = mAvailRect.x;
  *outTop = mAvailRect.y;
  *outWidth = mAvailRect.width;
  *outHeight = mAvailRect.height;
  return NS_OK;
}

NS_IMETHODIMP
ScreenProxy::GetPixelDepth(int32_t *aPixelDepth)
{
  if (!EnsureCacheIsValid()) {
    return NS_ERROR_FAILURE;
  }

  *aPixelDepth = mPixelDepth;
  return NS_OK;
}

NS_IMETHODIMP
ScreenProxy::GetColorDepth(int32_t *aColorDepth)
{
  if (!EnsureCacheIsValid()) {
    return NS_ERROR_FAILURE;
  }

  *aColorDepth = mColorDepth;
  return NS_OK;
}

void
ScreenProxy::PopulateByDetails(ScreenDetails aDetails)
{
  mId = aDetails.id();
  mRect = nsIntRect(aDetails.rect());
  mAvailRect = nsIntRect(aDetails.availRect());
  mPixelDepth = aDetails.pixelDepth();
  mColorDepth = aDetails.colorDepth();
  mContentsScaleFactor = aDetails.contentsScaleFactor();
}

bool
ScreenProxy::EnsureCacheIsValid()
{
  if (mCacheValid) {
    return true;
  }

  bool success = false;
  
  
  ScreenDetails details;
  unused << mScreenManager->CallScreenRefresh(mId, &details, &success);
  if (!success) {
    NS_WARNING("Updating a ScreenProxy in the child process failed on parent side.");
    return false;
  }

  PopulateByDetails(details);
  mCacheValid = true;

  InvalidateCacheOnNextTick();
  return true;
}

void
ScreenProxy::InvalidateCacheOnNextTick()
{
  if (mCacheWillInvalidate) {
    return;
  }

  mCacheWillInvalidate = true;

  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->RunInStableState(
      NS_NewRunnableMethod(this, &ScreenProxy::InvalidateCache)
    );
  } else {
    
    
    InvalidateCache();
  }
}

void
ScreenProxy::InvalidateCache()
{
  mCacheValid = false;
  mCacheWillInvalidate = false;
}

} 
} 


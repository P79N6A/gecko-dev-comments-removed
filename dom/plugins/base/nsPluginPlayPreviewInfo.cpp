




#include "nsPluginPlayPreviewInfo.h"

using namespace mozilla;

nsPluginPlayPreviewInfo::nsPluginPlayPreviewInfo(const char* aMimeType,
                                                 bool aIgnoreCTP,
                                                 const char* aRedirectURL)
  : mMimeType(aMimeType), mIgnoreCTP(aIgnoreCTP), mRedirectURL(aRedirectURL) {}

nsPluginPlayPreviewInfo::nsPluginPlayPreviewInfo(
  const nsPluginPlayPreviewInfo* aSource)
{
  MOZ_ASSERT(aSource);

  mMimeType = aSource->mMimeType;
  mIgnoreCTP = aSource->mIgnoreCTP;
  mRedirectURL = aSource->mRedirectURL;
}

nsPluginPlayPreviewInfo::~nsPluginPlayPreviewInfo()
{
}

NS_IMPL_ISUPPORTS1(nsPluginPlayPreviewInfo, nsIPluginPlayPreviewInfo)

NS_IMETHODIMP
nsPluginPlayPreviewInfo::GetMimeType(nsACString& aMimeType)
{
  aMimeType = mMimeType;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginPlayPreviewInfo::GetIgnoreCTP(bool* aIgnoreCTP)
{
  *aIgnoreCTP = mIgnoreCTP;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginPlayPreviewInfo::GetRedirectURL(nsACString& aRedirectURL)
{
  aRedirectURL = mRedirectURL;
  return NS_OK;
}

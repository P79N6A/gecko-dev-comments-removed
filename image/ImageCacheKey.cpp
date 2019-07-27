




#include "ImageCacheKey.h"

#include "mozilla/Move.h"
#include "File.h"
#include "ImageURL.h"
#include "nsHostObjectProtocolHandler.h"
#include "nsString.h"

namespace mozilla {

using namespace dom;

namespace image {

ImageCacheKey::ImageCacheKey(nsIURI* aURI)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aURI);

  bool isChrome;
  mIsChrome = NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome;

  aURI->GetSpec(mSpec);
  mHash = ComputeHash(mSpec);
}

ImageCacheKey::ImageCacheKey(ImageURL* aURI)
{
  MOZ_ASSERT(aURI);

  bool isChrome;
  mIsChrome = NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome;

  aURI->GetSpec(mSpec);
  mHash = ComputeHash(mSpec);
}

ImageCacheKey::ImageCacheKey(const ImageCacheKey& aOther)
  : mSpec(aOther.mSpec)
  , mHash(aOther.mHash)
  , mIsChrome(aOther.mIsChrome)
{ }

ImageCacheKey::ImageCacheKey(ImageCacheKey&& aOther)
  : mSpec(Move(aOther.mSpec))
  , mHash(aOther.mHash)
  , mIsChrome(aOther.mIsChrome)
{ }

bool
ImageCacheKey::operator==(const ImageCacheKey& aOther) const
{
  return mSpec == aOther.mSpec;
}

 uint32_t
ImageCacheKey::ComputeHash(const nsACString& aSpec)
{
  
  
  return HashString(aSpec);
}

} 
} 

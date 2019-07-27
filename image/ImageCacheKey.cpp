




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
  : mURI(new ImageURL(aURI))
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mURI);

  bool isChrome;
  mIsChrome = NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome;

  mHash = ComputeHash(mURI);
}

ImageCacheKey::ImageCacheKey(ImageURL* aURI)
  : mURI(aURI)
{
  MOZ_ASSERT(mURI);

  bool isChrome;
  mIsChrome = NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome;

  mHash = ComputeHash(mURI);
}

ImageCacheKey::ImageCacheKey(const ImageCacheKey& aOther)
  : mURI(aOther.mURI)
  , mHash(aOther.mHash)
  , mIsChrome(aOther.mIsChrome)
{ }

ImageCacheKey::ImageCacheKey(ImageCacheKey&& aOther)
  : mURI(Move(aOther.mURI))
  , mHash(aOther.mHash)
  , mIsChrome(aOther.mIsChrome)
{ }

bool
ImageCacheKey::operator==(const ImageCacheKey& aOther) const
{
  return *mURI == *aOther.mURI;
}

const char*
ImageCacheKey::Spec() const
{
  return mURI->Spec();
}

 uint32_t
ImageCacheKey::ComputeHash(ImageURL* aURI)
{
  
  
  nsAutoCString spec;
  aURI->GetSpec(spec);
  return HashString(spec);
}

} 
} 

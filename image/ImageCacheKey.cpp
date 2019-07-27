




#include "ImageCacheKey.h"

#include "mozilla/Move.h"
#include "File.h"
#include "ImageURL.h"
#include "nsHostObjectProtocolHandler.h"
#include "nsString.h"

namespace mozilla {

using namespace dom;

namespace image {

bool
URISchemeIs(ImageURL* aURI, const char* aScheme)
{
  bool schemeMatches = false;
  if (NS_WARN_IF(NS_FAILED(aURI->SchemeIs(aScheme, &schemeMatches)))) {
    return false;
  }
  return schemeMatches;
}

static Maybe<uint64_t>
BlobSerial(ImageURL* aURI)
{
  nsAutoCString spec;
  aURI->GetSpec(spec);

  nsRefPtr<BlobImpl> blob;
  if (NS_SUCCEEDED(NS_GetBlobForBlobURISpec(spec, getter_AddRefs(blob))) &&
      blob) {
    return Some(blob->GetSerialNumber());
  }

  return Nothing();
}

ImageCacheKey::ImageCacheKey(nsIURI* aURI)
  : mURI(new ImageURL(aURI))
  , mIsChrome(URISchemeIs(mURI, "chrome"))
{
  MOZ_ASSERT(NS_IsMainThread());

  if (URISchemeIs(mURI, "blob")) {
    mBlobSerial = BlobSerial(mURI);
  }

  mHash = ComputeHash(mURI, mBlobSerial);
}

ImageCacheKey::ImageCacheKey(ImageURL* aURI)
  : mURI(aURI)
  , mIsChrome(URISchemeIs(mURI, "chrome"))
{
  MOZ_ASSERT(aURI);

  if (URISchemeIs(mURI, "blob")) {
    mBlobSerial = BlobSerial(mURI);
  }

  mHash = ComputeHash(mURI, mBlobSerial);
}

ImageCacheKey::ImageCacheKey(const ImageCacheKey& aOther)
  : mURI(aOther.mURI)
  , mBlobSerial(aOther.mBlobSerial)
  , mHash(aOther.mHash)
  , mIsChrome(aOther.mIsChrome)
{ }

ImageCacheKey::ImageCacheKey(ImageCacheKey&& aOther)
  : mURI(Move(aOther.mURI))
  , mBlobSerial(Move(aOther.mBlobSerial))
  , mHash(aOther.mHash)
  , mIsChrome(aOther.mIsChrome)
{ }

bool
ImageCacheKey::operator==(const ImageCacheKey& aOther) const
{
  if (mBlobSerial || aOther.mBlobSerial) {
    
    
    return mBlobSerial == aOther.mBlobSerial &&
           mURI->HasSameRef(*aOther.mURI);
  }

  
  return *mURI == *aOther.mURI;
}

const char*
ImageCacheKey::Spec() const
{
  return mURI->Spec();
}

 uint32_t
ImageCacheKey::ComputeHash(ImageURL* aURI,
                           const Maybe<uint64_t>& aBlobSerial)
{
  
  

  if (aBlobSerial) {
    
    
    
    
    
    nsAutoCString ref;
    aURI->GetRef(ref);
    return HashGeneric(*aBlobSerial, HashString(ref));
  }

  
  nsAutoCString spec;
  aURI->GetSpec(spec);
  return HashString(spec);
}

} 
} 

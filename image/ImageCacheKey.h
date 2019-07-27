








#ifndef mozilla_image_src_ImageCacheKey_h
#define mozilla_image_src_ImageCacheKey_h

class nsIURI;

namespace mozilla {
namespace image {

class ImageURL;







class ImageCacheKey final
{
public:
  explicit ImageCacheKey(nsIURI* aURI);
  explicit ImageCacheKey(ImageURL* aURI);

  ImageCacheKey(const ImageCacheKey& aOther);
  ImageCacheKey(ImageCacheKey&& aOther);

  bool operator==(const ImageCacheKey& aOther) const;
  uint32_t Hash() const { return mHash; }

  
  const char* Spec() const;

  
  bool IsChrome() const { return mIsChrome; }

private:
  static uint32_t ComputeHash(ImageURL* aURI);

  nsRefPtr<ImageURL> mURI;
  uint32_t mHash;
  bool mIsChrome;
};

} 
} 

#endif 

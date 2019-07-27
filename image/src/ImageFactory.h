





#ifndef mozilla_image_src_ImageFactory_h
#define mozilla_image_src_ImageFactory_h

#include "nsCOMPtr.h"
#include "nsProxyRelease.h"

class nsCString;
class nsIRequest;

namespace mozilla {
namespace image {

class Image;
class ImageURL;
class ProgressTracker;

class ImageFactory
{
public:
  


  static void Initialize();

  










  static already_AddRefed<Image> CreateImage(nsIRequest* aRequest,
                                             ProgressTracker* aProgressTracker,
                                             const nsCString& aMimeType,
                                             ImageURL* aURI,
                                             bool aIsMultiPart,
                                             uint32_t aInnerWindowId);
  





  static already_AddRefed<Image>
  CreateAnonymousImage(const nsCString& aMimeType);

private:
  
  static already_AddRefed<Image>
  CreateRasterImage(nsIRequest* aRequest,
                    ProgressTracker* aProgressTracker,
                    const nsCString& aMimeType,
                    ImageURL* aURI,
                    uint32_t aImageFlags,
                    uint32_t aInnerWindowId);

  static already_AddRefed<Image>
  CreateVectorImage(nsIRequest* aRequest,
                    ProgressTracker* aProgressTracker,
                    const nsCString& aMimeType,
                    ImageURL* aURI,
                    uint32_t aImageFlags,
                    uint32_t aInnerWindowId);

  
  virtual ~ImageFactory() = 0;
};

} 
} 

#endif 

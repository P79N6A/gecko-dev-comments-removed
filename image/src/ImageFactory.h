





#ifndef MOZILLA_IMAGELIB_IMAGEFACTORY_H_
#define MOZILLA_IMAGELIB_IMAGEFACTORY_H_

#include "nsCOMPtr.h"

class nsCString;
class nsIRequest;
class nsIURI;
class imgStatusTracker;

namespace mozilla {
namespace image {

class Image;

class ImageFactory
{
public:
  









  static already_AddRefed<Image> CreateImage(nsIRequest* aRequest,
                                             imgStatusTracker* aStatusTracker,
                                             const nsCString& aMimeType,
                                             nsIURI* aURI,
                                             bool aIsMultiPart,
                                             uint32_t aInnerWindowId);
  





  static already_AddRefed<Image> CreateAnonymousImage(const nsCString& aMimeType);

private:
  
  static already_AddRefed<Image> CreateRasterImage(nsIRequest* aRequest,
                                                   imgStatusTracker* aStatusTracker,
                                                   const nsCString& aMimeType,
                                                   nsIURI* aURI,
                                                   uint32_t aImageFlags,
                                                   uint32_t aInnerWindowId);

  static already_AddRefed<Image> CreateVectorImage(nsIRequest* aRequest,
                                                   imgStatusTracker* aStatusTracker,
                                                   const nsCString& aMimeType,
                                                   nsIURI* aURI,
                                                   uint32_t aImageFlags,
                                                   uint32_t aInnerWindowId);

  
  virtual ~ImageFactory() = 0;
};

} 
} 

#endif 

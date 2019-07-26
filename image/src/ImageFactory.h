





#include "nsIURI.h"
#include "nsIRequest.h"

#include "imgIContainer.h"
#include "imgStatusTracker.h"

#include "Image.h"

namespace mozilla {
namespace image {

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

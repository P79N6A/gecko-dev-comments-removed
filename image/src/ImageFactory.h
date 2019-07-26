





#include "nsIURI.h"
#include "nsIRequest.h"

#include "imgIContainer.h"
#include "imgStatusTracker.h"

#include "Image.h"

namespace mozilla {
namespace image {

extern const char* SVG_MIMETYPE;

struct ImageFactory
{
  









  static already_AddRefed<Image> CreateImage(nsIRequest* aRequest,
                                             imgStatusTracker* aStatusTracker,
                                             const nsCString& aMimeType,
                                             nsIURI* aURI,
                                             bool aIsMultiPart,
                                             uint32_t aInnerWindowId);

private:
  
  static already_AddRefed<Image> CreateRasterImage(nsIRequest* aRequest,
                                                   imgStatusTracker* aStatusTracker,
                                                   const nsCString& aMimeType,
                                                   const nsCString& aURIString,
                                                   uint32_t aImageFlags,
                                                   uint32_t aInnerWindowId);

  static already_AddRefed<Image> CreateVectorImage(nsIRequest* aRequest,
                                                   imgStatusTracker* aStatusTracker,
                                                   const nsCString& aMimeType,
                                                   const nsCString& aURIString,
                                                   uint32_t aImageFlags,
                                                   uint32_t aInnerWindowId);

  
  virtual ~ImageFactory() = 0;
};

} 
} 

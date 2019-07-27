




#ifndef mozilla_image_src_ImageURL_h
#define mozilla_image_src_ImageURL_h

#include "nsIURI.h"
#include "MainThreadUtils.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace image {













class ImageURL
{
public:
  explicit ImageURL(nsIURI* aURI)
  {
    MOZ_ASSERT(NS_IsMainThread(), "Cannot use nsIURI off main thread!");
    aURI->GetSpec(mSpec);
    aURI->GetScheme(mScheme);
    aURI->GetRef(mRef);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageURL)

  nsresult GetSpec(nsACString& result)
  {
    result = mSpec;
    return NS_OK;
  }

  nsresult GetScheme(nsACString& result)
  {
    result = mScheme;
    return NS_OK;
  }

  nsresult SchemeIs(const char* scheme, bool* result)
  {
    NS_PRECONDITION(scheme, "scheme is null");
    NS_PRECONDITION(result, "result is null");

    *result = mScheme.Equals(scheme);
    return NS_OK;
  }

  nsresult GetRef(nsACString& result)
  {
    result = mRef;
    return NS_OK;
  }

  already_AddRefed<nsIURI> ToIURI()
  {
    MOZ_ASSERT(NS_IsMainThread(),
               "Convert to nsIURI on main thread only; it is not threadsafe.");
    nsCOMPtr<nsIURI> newURI;
    NS_NewURI(getter_AddRefs(newURI), mSpec);
    return newURI.forget();
  }

private:
  
  
  
  
  
  nsAutoCString mSpec;
  nsAutoCString mScheme;
  nsAutoCString mRef;

  ~ImageURL() { }
};

} 
} 

#endif 

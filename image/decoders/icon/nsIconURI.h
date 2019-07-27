





#ifndef mozilla_image_decoders_icon_nsIconURI_h
#define mozilla_image_decoders_icon_nsIconURI_h

#include "nsIIconURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIIPCSerializableURI.h"

class nsMozIconURI final : public nsIMozIconURI
                         , public nsIIPCSerializableURI
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURI
  NS_DECL_NSIMOZICONURI
  NS_DECL_NSIIPCSERIALIZABLEURI

  
  nsMozIconURI();

protected:
  virtual ~nsMozIconURI();
  nsCOMPtr<nsIURL> mIconURL; 
  uint32_t mSize; 
                  
  nsCString mContentType; 
                          
  nsCString mFileName; 
                       
  nsCString mStockIcon;
  int32_t mIconSize;   
                       
  int32_t mIconState;  
                       
};

#endif 

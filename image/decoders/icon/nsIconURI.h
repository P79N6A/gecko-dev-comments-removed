





#ifndef nsMozIconURI_h__
#define nsMozIconURI_h__

#include "nsIIconURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsMozIconURI MOZ_FINAL : public nsIMozIconURI
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURI
  NS_DECL_NSIMOZICONURI

  
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

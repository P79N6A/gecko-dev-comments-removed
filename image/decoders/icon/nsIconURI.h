





#ifndef nsMozIconURI_h__
#define nsMozIconURI_h__

#include "nsIIconURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#define NS_MOZICONURI_CID                            \
{                                                    \
    0x43a88e0e,                                      \
    0x2d37,                                          \
    0x11d5,                                          \
    { 0x99, 0x7, 0x0, 0x10, 0x83, 0x1, 0xe, 0x9b }   \
}

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

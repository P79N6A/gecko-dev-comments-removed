






































#ifndef nsMozIconURI_h__
#define nsMozIconURI_h__

#include "nsIIconURI.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"

#define NS_MOZICONURI_CID                            \
{                                                    \
    0x43a88e0e,                                      \
    0x2d37,                                          \
    0x11d5,                                          \
    { 0x99, 0x7, 0x0, 0x10, 0x83, 0x1, 0xe, 0x9b }   \
}

class nsMozIconURI : public nsIMozIconURI
{
public:    
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURI
  NS_DECL_NSIMOZICONURI

  
  nsMozIconURI();
  virtual ~nsMozIconURI();

protected:
  nsCOMPtr<nsIURL> mIconURL; 
  PRUint32 mSize; 
  nsCString mContentType; 
  nsCString mFileName; 
  nsCString mStockIcon;
  PRInt32 mIconSize;     
  PRInt32 mIconState;    
};

#endif 

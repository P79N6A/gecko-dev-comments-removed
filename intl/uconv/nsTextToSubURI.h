



#ifndef nsTextToSubURI_h__
#define nsTextToSubURI_h__

#include "nsITextToSubURI.h"
#include "nsString.h"
#include "nsTArray.h"


class nsTextToSubURI: public nsITextToSubURI {
  NS_DECL_ISUPPORTS
  NS_DECL_NSITEXTTOSUBURI

private:
  virtual ~nsTextToSubURI();

  
  
  
  
  
  
  
  
  
  nsresult convertURItoUnicode(const nsAFlatCString &aCharset,
                               const nsAFlatCString &aURI, 
                               bool aIRI, 
                               nsAString &_retval);

  
  
  nsTArray<char16_t> mUnsafeChars;
};

#endif 


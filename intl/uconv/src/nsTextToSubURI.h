



#ifndef nsTextToSubURI_h__
#define nsTextToSubURI_h__

#include "nsITextToSubURI.h"
#include "nsString.h"


class nsTextToSubURI: public nsITextToSubURI {
  NS_DECL_ISUPPORTS
  NS_DECL_NSITEXTTOSUBURI

public:
  nsTextToSubURI();
  virtual ~nsTextToSubURI();

private:
  
  
  
  
  
  
  
  
  
  nsresult convertURItoUnicode(const nsAFlatCString &aCharset,
                               const nsAFlatCString &aURI, 
                               bool aIRI, 
                               nsAString &_retval);
};

#endif 


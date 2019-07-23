



































#ifndef nsCharsOrderIdFormater_h__
#define nsCharsOrderIdFormater_h__


#include "nsIOrderIdFormater.h"
#include "nsCharsList.h"

class nsCharsOrderIdFormater : public nsIOrderIdFormater {
  NS_DECL_ISUPPORTS

public: 
  
  nsCharsOrderIdFormater( nsCharsList* aList);

  virtual nsCharsOrderIdFormater();
  

  
  NS_IMETHOD ToString( PRUint32 aOrder, nsString& aResult) = 0;

private:

  nsCharsList *mList;
  PRUint32     mBase;
};

#endif  

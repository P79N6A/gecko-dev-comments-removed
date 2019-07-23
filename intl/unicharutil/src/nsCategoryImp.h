



































#ifndef nsCategoryImp_h__
#define nsCategoryImp_h__

#include "nscore.h"
#include "nsISupports.h"
#include "nsIUGenCategory.h"

class nsCategoryImp : public nsIUGenCategory {
   NS_DECL_ISUPPORTS
   
public: 
   nsCategoryImp();
   virtual ~nsCategoryImp();


   


   NS_IMETHOD Get( PRUnichar aChar, nsUGenCategory* oResult);
    
   




   NS_IMETHOD Is( PRUnichar aChar, nsUGenCategory aCategory, PRBool* oResult);
};

#endif  

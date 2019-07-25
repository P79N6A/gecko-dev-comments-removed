



#ifndef nsCategoryImp_h__
#define nsCategoryImp_h__

#include "nsIUGenCategory.h"

class nsCategoryImp : public nsIUGenCategory {
   NS_DECL_ISUPPORTS
   
public:
   static nsCategoryImp* GetInstance();
    
   


   virtual nsUGenCategory Get(PRUint32 aChar);
};

#endif  





#ifndef nsCategoryImp_h__
#define nsCategoryImp_h__

#include "nsIUGenCategory.h"

class nsCategoryImp : public nsIUGenCategory {
   NS_DECL_THREADSAFE_ISUPPORTS
   
public:
   static nsCategoryImp* GetInstance();
    
   


   virtual nsUGenCategory Get(uint32_t aChar) MOZ_OVERRIDE;
};

#endif  





































#ifndef nsIUGenCategory_h__
#define nsIUGenCategory_h__


#include "nsISupports.h"
#include "nscore.h"


#define NS_IUGENCATEGORY_IID \
{ 0xe86b3371, 0xbf89, 0x11d2, \
    { 0xb3, 0xaf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


class nsIUGenCategory : public nsISupports {

public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUGENCATEGORY_IID)

   




   typedef enum {
     kUGenCategory_Mark         = 1, 
     kUGenCategory_Number       = 2, 
     kUGenCategory_Separator    = 3, 
     kUGenCategory_Other        = 4, 
     kUGenCategory_Letter       = 5, 
     kUGenCategory_Punctuation  = 6, 
     kUGenCategory_Symbol       = 7  
   } nsUGenCategory;

   


   NS_IMETHOD Get( PRUnichar aChar, nsUGenCategory* oResult) = 0 ;
    
   




   NS_IMETHOD Is( PRUnichar aChar, nsUGenCategory aCategory, PRBool* oResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUGenCategory, NS_IUGENCATEGORY_IID)

#endif  

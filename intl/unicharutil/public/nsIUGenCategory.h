



#ifndef nsIUGenCategory_h__
#define nsIUGenCategory_h__


#include "nsISupports.h"
#include "nscore.h"


#define NS_IUGENCATEGORY_IID \
{ 0x671fea05, 0xfcee, 0x4b1c, \
    { 0x82, 0xa3, 0x6e, 0xb0, 0x3e, 0xda, 0x8d, 0xdc } }


class nsIUGenCategory : public nsISupports {

public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUGENCATEGORY_IID)

   



   typedef enum {
     kUndefined    = 0,
     kMark         = 1, 
     kNumber       = 2, 
     kSeparator    = 3, 
     kOther        = 4, 
     kLetter       = 5, 
     kPunctuation  = 6, 
     kSymbol       = 7  
   } nsUGenCategory;

   


   virtual nsUGenCategory Get(uint32_t aChar) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUGenCategory, NS_IUGENCATEGORY_IID)

#endif  

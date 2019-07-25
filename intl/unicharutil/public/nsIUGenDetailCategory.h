



































#ifndef nsIUGenDetailCategory_h__
#define nsIUGenDetailCategory_h__


#include "nsISupports.h"
#include "nscore.h"


#define NS_IUGENDETAILCATEGORY_IID \
{ 0xe86b3372, 0xbf89, 0x11d2, \
    { 0xb3, 0xaf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } };



class nsIUGenDetailCategory : public nsISupports {

public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUGENDETAILCATEGORY_IID)

   




   typedef enum {
              
     kUGDC_M  = (kUGenCategory_Mark << 4) | 0, 
     kUGDC_Mn = (kUGenCategory_Mark << 4) | 1, 
     kUGDC_Mc = (kUGenCategory_Mark << 4) | 2, 
     kUGDC_Me = (kUGenCategory_Mark << 4) | 3, 

     kUGDC_N  = (kUGenCategory_Number << 4) | 0, 
     kUGDC_Nd = (kUGenCategory_Number << 4) | 1, 
     kUGDC_Nl = (kUGenCategory_Number << 4) | 2, 
     kUGDC_No = (kUGenCategory_Number << 4) | 3, 

     kUGDC_Z  = (kUGenCategory_Separator << 4) | 0, 
     kUGDC_Zs = (kUGenCategory_Separator << 4) | 1, 
     kUGDC_Zl = (kUGenCategory_Separator << 4) | 2, 
     kUGDC_Zp = (kUGenCategory_Separator << 4) | 3, 

     kUGDC_C  = (kUGenCategory_Other << 4) | 0, 
     kUGDC_Cc = (kUGenCategory_Other << 4) | 1, 
     kUGDC_Cf = (kUGenCategory_Other << 4) | 2, 
     kUGDC_Cs = (kUGenCategory_Other << 4) | 3, 
     kUGDC_Co = (kUGenCategory_Other << 4) | 4, 
     kUGDC_Cn = (kUGenCategory_Other << 4) | 5, 

     kUGDC_L  = (kUGenCategory_Letter << 4) | 0, 
     kUGDC_Lu = (kUGenCategory_Letter << 4) | 1, 
     kUGDC_Ll = (kUGenCategory_Letter << 4) | 2, 
     kUGDC_Lt = (kUGenCategory_Letter << 4) | 3, 
     kUGDC_Lm = (kUGenCategory_Letter << 4) | 4, 
     kUGDC_Lo = (kUGenCategory_Letter << 4) | 5, 

     kUGDC_P  = (kUGenCategory_Punctuation << 4) | 0, 
     kUGDC_Pc = (kUGenCategory_Punctuation << 4) | 1, 
     kUGDC_Pd = (kUGenCategory_Punctuation << 4) | 2, 
     kUGDC_Ps = (kUGenCategory_Punctuation << 4) | 3, 
     kUGDC_Pe = (kUGenCategory_Punctuation << 4) | 4, 
     kUGDC_Pi = (kUGenCategory_Punctuation << 4) | 5, 
     kUGDC_Pf = (kUGenCategory_Punctuation << 4) | 6, 
     kUGDC_Po = (kUGenCategory_Punctuation << 4) | 7, 

     kUGDC_S  = (kUGenCategory_Symbol << 4) | 0, 
     kUGDC_Sm = (kUGenCategory_Symbol << 4) | 1, 
     kUGDC_Sc = (kUGenCategory_Symbol << 4) | 2, 
     kUGDC_Sk = (kUGenCategory_Symbol << 4) | 3, 
     kUGDC_So = (kUGenCategory_Symbol << 4) | 4, 
   } nsUGDC;

   



   typedef enum {
     kUGDC_DetailNone          = 0,
     kUGDC_DetailMark          = ( 1 << 0),
     kUGDC_DetailNumber        = ( 1 << 1),
     kUGDC_DetailSeparator     = ( 1 << 2),
     kUGDC_DetailOther         = ( 1 << 3),
     kUGDC_DetailLetter        = ( 1 << 4),
     kUGDC_DetailPunctuation   = ( 1 << 5),
     kUGDC_DetailSymbol        = ( 1 << 6),
     kUGDC_DetailNormative     = ( kUGDC_DetailMark | 
                                   kUGDC_DetailNumber | 
                                   kUGDC_DetailSeparator | 
                                   kUGDC_DetailOther  ),
     kUGDC_DetailInformative   = ( kUGDC_DetailLetter | 
                                   kUGDC_DetailPunctuation | 
                                   kUGDC_DetailSymbol);
     kUGDC_DetailAll           = ( kUGDC_DetailNormative | kUGDC_Informative );
   } nsUGDC_Detail;

   


   NS_IMETHOD Get( PRUnichar aChar, nsUGDC_Detail aDetailSpec, nsUGDC* oResult) = 0 ;
    
   




   NS_IMETHOD Is( PRUnichar aChar, nsUGDC aDetailCategory, PRBool* oResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUGenDetailCategory,
                              NS_IUGENDETAILCATEGORY_IID)

#endif  

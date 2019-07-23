





































#ifndef nsIImageMac_h__
#define nsIImageMac_h__


#include "nsISupports.h"
#include "nsRect.h"
#include <Quickdraw.h>




#define NS_IIMAGEMAC_IID \
 { 0xDE2628F9, 0x6023, 0x4443, \
   { 0xA4, 0xC9, 0xCE, 0x0C, 0xE6, 0xDA, 0x06, 0x28 } };





  


class nsIImageMac : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IIMAGEMAC_IID)

    
    
    
    
  NS_IMETHOD ConvertToPICT ( PicHandle* outPicture ) = 0;
  
    
    
  NS_IMETHOD ConvertFromPICT ( PicHandle inPicture ) = 0;

    
    
  NS_IMETHOD GetCGImageRef(CGImageRef* aCGImageRef) = 0;

}; 

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImageMac, NS_IIMAGEMAC_IID)

#endif

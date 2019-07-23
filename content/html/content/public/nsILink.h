




































#ifndef nsILink_h___
#define nsILink_h___

#include "nsISupports.h"
#include "nsILinkHandler.h" 

class nsIURI;


#define NS_ILINK_IID    \
{ 0x6f374a11, 0x212d, 0x47d6, \
  { 0x94, 0xd1, 0xe6, 0x7c, 0x23, 0x4d, 0x34, 0x99 } }








class nsILink : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINK_IID)

  




  




  NS_IMETHOD LinkAdded() = 0;

  





  NS_IMETHOD LinkRemoved() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILink, NS_ILINK_IID)

#endif 

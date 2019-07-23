




































#ifndef nsILink_h___
#define nsILink_h___

#include "nsISupports.h"
#include "nsILinkHandler.h" 

class nsIURI;


#define NS_ILINK_IID    \
{ 0x0c212bc4, 0xfcd7, 0x479d,  \
  { 0x8c, 0x3f, 0x3b, 0xe8, 0xe6, 0x78, 0x74, 0x50 } }








class nsILink : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINK_IID)

  






  NS_IMETHOD GetLinkState(nsLinkState &aState) = 0;

  





  NS_IMETHOD SetLinkState(nsLinkState aState) = 0;

  







  NS_IMETHOD GetHrefURI(nsIURI** aURI) = 0;

  




  NS_IMETHOD LinkAdded() = 0;

  





  NS_IMETHOD LinkRemoved() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILink, NS_ILINK_IID)

#endif 

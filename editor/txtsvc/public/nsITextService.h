




































#ifndef nsITextService_h__
#define nsITextService_h__

#include "nsISupports.h"

class nsITextServicesDocument;





#define NS_ITEXTSERVICE_IID                    \
{ /* 019718E0-CDB5-11d2-8D3C-000000000000 */    \
0x019718e0, 0xcdb5, 0x11d2,                     \
{ 0x8d, 0x3c, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }





class nsITextService  : public nsISupports{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEXTSERVICE_IID)

  


  NS_IMETHOD Init(nsITextServicesDocument *aDoc) = 0;
  NS_IMETHOD Execute() = 0;
  NS_IMETHOD GetMenuString(nsString &aString) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITextService, NS_ITEXTSERVICE_IID)

#endif 


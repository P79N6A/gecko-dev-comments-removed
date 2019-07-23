





































#ifndef nsIOptionElement_h___
#define nsIOptionElement_h___

#include "nsISupports.h"


#define NS_IOPTIONELEMENT_IID    \
{ 0xd49fe03a, 0x1dd1, 0x11b2,    \
  { 0xa0, 0xe0, 0x83, 0x66, 0x76, 0x19, 0xaf, 0x69 } }








class nsIOptionElement : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOPTIONELEMENT_IID)

  





  NS_IMETHOD SetSelectedInternal(PRBool aValue, PRBool aNotify) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIOptionElement, NS_IOPTIONELEMENT_IID)

#endif 


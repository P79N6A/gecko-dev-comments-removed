


































#ifndef nsISupportsBase_h__
#define nsISupportsBase_h__

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef nsID_h__
#include "nsID.h"
#endif










#define NS_ISUPPORTS_IID                                                      \
  { 0x00000000, 0x0000, 0x0000,                                               \
    {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} }







class NS_NO_VTABLE nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISUPPORTS_IID)

  



  
  








  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) = 0;
  






  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;

  






  NS_IMETHOD_(nsrefcnt) Release(void) = 0;

  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISupports, NS_ISUPPORTS_IID)



#endif

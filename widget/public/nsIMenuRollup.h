





































#ifndef nsIMenuRollup_h___
#define nsIMenuRollup_h___

#include "nsISupports.h"
#include "nsTArray.h"

class nsIWidget;

#define NS_IMENUROLLUP_IID \
  {0x2b65d177, 0xc3e4, 0x4564, \
    { 0x8d, 0xed, 0x86, 0xd2, 0xfa, 0x2f, 0x65, 0x9a }}

class nsIMenuRollup : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUROLLUP_IID)

  
  virtual void GetSubmenuWidgetChain(nsTArray<nsIWidget*> *_retval) = 0;

  
  virtual void AdjustPopupsOnWindowChange(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuRollup, NS_IMENUROLLUP_IID)

#endif

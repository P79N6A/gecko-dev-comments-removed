





































#ifndef nsIMenuRollup_h___
#define nsIMenuRollup_h___

#include "nsISupports.h"
#include "nsTArray.h"

class nsIWidget;
class nsIContent;

#define NS_IMENUROLLUP_IID \
  {0x61c9d01f, 0x8a4c, 0x4bb0, \
    { 0xa9, 0x90, 0xeb, 0xf6, 0x65, 0x4c, 0xda, 0x61 }}

class nsIMenuRollup : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUROLLUP_IID)

  






  virtual PRUint32 GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) = 0;

  

 
  virtual void AdjustPopupsOnWindowChange(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuRollup, NS_IMENUROLLUP_IID)

#endif

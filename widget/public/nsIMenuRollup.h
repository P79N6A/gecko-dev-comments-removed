





































#ifndef nsIMenuRollup_h___
#define nsIMenuRollup_h___

#include "nsISupports.h"
#include "nsTArray.h"

class nsIWidget;
class nsIContent;
class nsPIDOMWindow;

#define NS_IMENUROLLUP_IID \
  {0xa707b588, 0xa564, 0x488d, \
    { 0x87, 0xb6, 0xdb, 0x71, 0x2d, 0x78, 0x9d, 0x4c }}

class nsIMenuRollup : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUROLLUP_IID)

  






  virtual PRUint32 GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) = 0;

  

 
  virtual void AdjustPopupsOnWindowChange(nsPIDOMWindow* aWindow) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuRollup, NS_IMENUROLLUP_IID)

#endif

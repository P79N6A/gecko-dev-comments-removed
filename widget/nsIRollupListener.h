








































#ifndef __nsIRollupListener_h__
#define __nsIRollupListener_h__

#include "nsTArray.h"

class nsIContent;
class nsIWidget;

class nsIRollupListener {
 public: 

  








  virtual nsIContent* Rollup(PRUint32 aCount, bool aGetLastRolledUp = false) = 0;

  


  virtual bool ShouldRollupOnMouseWheelEvent() = 0;

  


  virtual bool ShouldRollupOnMouseActivate() = 0;

  







  virtual PRUint32 GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) = 0;
};

#endif 

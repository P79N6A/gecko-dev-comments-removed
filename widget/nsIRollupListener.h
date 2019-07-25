





#ifndef __nsIRollupListener_h__
#define __nsIRollupListener_h__

#include "nsTArray.h"

class nsIContent;
class nsIWidget;

class nsIRollupListener {
 public: 

  








  virtual nsIContent* Rollup(uint32_t aCount, bool aGetLastRolledUp = false) = 0;

  


  virtual bool ShouldRollupOnMouseWheelEvent() = 0;

  


  virtual bool ShouldRollupOnMouseActivate() = 0;

  







  virtual uint32_t GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) = 0;

  


  virtual void NotifyGeometryChange() = 0;
};

#endif 

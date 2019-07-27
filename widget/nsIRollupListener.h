





#ifndef __nsIRollupListener_h__
#define __nsIRollupListener_h__

#include "nsTArray.h"
#include "nsPoint.h"

class nsIContent;
class nsIWidget;

class nsIRollupListener {
 public: 

  















  virtual bool Rollup(uint32_t aCount, bool aFlush,
                      const nsIntPoint* aPoint, nsIContent** aLastRolledUp) = 0;

  


  virtual bool ShouldRollupOnMouseWheelEvent() = 0;

  


  virtual bool ShouldConsumeOnMouseWheelEvent() = 0;

  


  virtual bool ShouldRollupOnMouseActivate() = 0;

  







  virtual uint32_t GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) = 0;

  


  virtual void NotifyGeometryChange() = 0;

  virtual nsIWidget* GetRollupWidget() = 0;
};

#endif 

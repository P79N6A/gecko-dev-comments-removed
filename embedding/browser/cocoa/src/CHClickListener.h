




































#ifndef __CHClickListener_h__
#define __CHClickListener_h__


#include <Cocoa/Cocoa.h>

#include "nsIDOMMouseListener.h"

class CHClickListener :  public nsIDOMMouseListener
{
public:
  CHClickListener();
  virtual ~CHClickListener();

  NS_DECL_ISUPPORTS
  
  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; };

private:

};


#endif

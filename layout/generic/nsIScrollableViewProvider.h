





































#ifndef _nsIScrollableViewProvider_h
#define _nsIScrollableViewProvider_h

#include "nsQueryFrame.h"

class nsIScrollableView;

class nsIScrollableViewProvider : public nsQueryFrame
{
 public: 
  NS_DECL_QUERYFRAME_TARGET(nsIScrollableViewProvider)

  virtual nsIScrollableView* GetScrollableView() = 0;
};

#endif 

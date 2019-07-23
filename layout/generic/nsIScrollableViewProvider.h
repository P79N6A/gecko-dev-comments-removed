





































#ifndef _nsIScrollableViewProvider_h
#define _nsIScrollableViewProvider_h

#include "nsQueryFrame.h"

class nsIScrollableView;

class nsIScrollableViewProvider : public nsQueryFrame
{
 public: 
  NS_DECLARE_FRAME_ACCESSOR(nsIScrollableViewProvider)

  virtual nsIScrollableView* GetScrollableView() = 0;
};

#endif 

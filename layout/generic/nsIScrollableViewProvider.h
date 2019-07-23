





































#ifndef _nsIScrollableViewProvider_h
#define _nsIScrollableViewProvider_h

#include "nsISupports.h"

#define NS_ISCROLLABLEVIEWPROVIDER_IID \
{0xdafcbf5f, 0x701f, 0x4697, \
{0xa5, 0x13, 0x81, 0xd8, 0x0e, 0x01, 0x41, 0x2c}}

class nsIScrollableView;

class nsIScrollableViewProvider : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLABLEVIEWPROVIDER_IID)

  virtual nsIScrollableView* GetScrollableView() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollableViewProvider,
                              NS_ISCROLLABLEVIEWPROVIDER_IID)

#endif 









































#ifndef nsIDragSessionXLIB_h_
#define nsIDragSessionXLIB_h_

#include "nsISupports.h"
#include "nsIWidget.h"

#define NS_IDRAGSESSIONXLIB_IID \
{ 0xa6b49c42, 0x1dd1, 0x11b2, { 0xb2, 0xdf, 0xc1, 0xd6, 0x1d, 0x67, 0x45, 0xcf } };

class nsIDragSessionXlib : public nsISupports {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAGSESSIONXLIB_IID)

  NS_IMETHOD IsDragging(PRBool *result) = 0;
  NS_IMETHOD UpdatePosition(PRInt32 x, PRInt32 y) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDragSessionXlib, NS_IDRAGSESSIONXLIB_IID)

#endif 

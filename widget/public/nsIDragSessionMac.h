




































#ifndef nsIDragSessionMac_h__
#define nsIDragSessionMac_h__

#include "nsISupports.h"
#include <Drag.h>


#define NS_IDRAGSESSIONMAC_IID      \
{ 0x36c4c380, 0x09e2, 0x11d3, { 0xb0, 0x33, 0xa4, 0x20, 0xf4, 0x2c, 0xfd, 0x7c } };


class nsIDragSessionMac : public nsISupports {

  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAGSESSIONMAC_IID)

  







   NS_IMETHOD SetDragReference ( DragReference aDragRef ) = 0; 

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDragSessionMac, NS_IDRAGSESSIONMAC_IID)

#endif

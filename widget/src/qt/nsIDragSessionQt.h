




































#ifndef nsIDragSessionQt_h__
#define nsIDragSessionQt_h__

#include "nsISupports.h"
#include <qdragobject.h> 

#define NS_IDRAGSESSIONQT_IID      \
{ 0x36c4c381, 0x09e3, 0x11d4, { 0xb0, 0x33, 0xa4, 0x20, 0xf4, 0x2c, 0xfd, 0x7c } };

class nsIDragSessionQt : public nsISupports
{
  public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDRAGSESSIONQT_IID)

  





   NS_IMETHOD SetDragReference(QMimeSource* aDragRef) = 0; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDragSessionQt, NS_IDRAGSESSIONQT_IID)

#endif

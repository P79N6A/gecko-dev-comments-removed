





































#ifndef nsIDragSessionBeOS_h__
#define nsIDragSessionBeOS_h__


#include "nsISupports.h"

#include <Message.h>


#define NS_IDRAGSESSIONBEOS_IID      \
{ 0x36c4c381, 0x09e3, 0x11d4, { 0xb0, 0x33, 0xa4, 0x20, 0xf4, 0x2c, 0xfd, 0x7c } };

class nsIDragSessionBeOS : public nsISupports
{
  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAGSESSIONBEOS_IID)
    NS_IMETHOD TransmitData( BMessage *aNegotiationReply) = 0;  
    NS_IMETHOD UpdateDragMessageIfNeeded(BMessage *aDragMessage) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDragSessionBeOS, NS_IDRAGSESSIONBEOS_IID)

#endif

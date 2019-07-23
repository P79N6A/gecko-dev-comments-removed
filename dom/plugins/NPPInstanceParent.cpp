





































#include "NPPInstanceParent.h"

namespace mozilla {
namespace plugins {


NPError
NPPInstanceParent::NPP_SetWindow(NPWindow* aWindow)
{
    _MOZ_LOG(__FUNCTION__);

#ifdef OS_LINUX

    
    
    

    
    XID window = (XID) aWindow->window;

    return mNpp.NPP_SetWindow((XID) aWindow->window,
                              aWindow->width,
                              aWindow->height);
    
    

#else
#  error Implement me for your OS
#endif
}

NPError
NPPInstanceParent::NPP_GetValue(NPPVariable variable, void *ret_value)
{
    _MOZ_LOG(__FUNCTION__);

    
#ifdef OS_LINUX

    switch(variable) {
    case NPPVpluginNeedsXEmbed:
        (*(PRBool*)ret_value) = PR_TRUE;
        return NPERR_NO_ERROR;
            
    default:
        return NPERR_GENERIC_ERROR;
    }

#else
#  error Implement me for your system
#endif
}


} 
} 

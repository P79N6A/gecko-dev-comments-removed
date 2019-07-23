




































#ifndef __nsIThebesRenderingContext_h
#define __nsIThebesRenderingContext_h

#include "nsIRenderingContext.h"


#define NSI_THEBES_RENDERING_CONTEXT_IID \
{ 0x8591c4c6, 0x41d4, 0x485a, \
{ 0xb2, 0x4f, 0x9d, 0xe1, 0x9b, 0x69, 0xce, 0x02 } }

class nsIThebesRenderingContext : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NSI_THEBES_RENDERING_CONTEXT_IID)

    NS_IMETHOD CreateDrawingSurface(nsNativeWidget aWidget, nsIDrawingSurface* &aSurface) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIThebesRenderingContext,
                              NSI_THEBES_RENDERING_CONTEXT_IID)

#endif 

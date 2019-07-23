



































#ifndef GLCONTEXTPROVIDER_H_
#define GLCONTEXTPROVIDER_H_

#include "GLContext.h"
#include "gfxTypes.h"
#include "gfxPoint.h"
#include "nsAutoPtr.h"

class nsIWidget;

namespace mozilla {
namespace gl {

class THEBES_API GLContextProvider 
{
public:
    





    already_AddRefed<GLContext> CreatePbuffer(const gfxSize &aSize);

    






    already_AddRefed<GLContext> CreateForWindow(nsIWidget *aWidget);
};

extern GLContextProvider THEBES_API sGLContextProvider;

}
}

#endif

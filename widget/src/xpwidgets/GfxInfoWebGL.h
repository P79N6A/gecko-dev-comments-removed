






































#ifndef __mozilla_widget_GfxInfoWebGL_h__
#define __mozilla_widget_GfxInfoWebGL_h__

#include "nsString.h"

namespace mozilla {
namespace widget {

class GfxInfoWebGL {
public:
    static nsresult GetWebGLParameter(const nsAString& aParam, nsAString& aResult);

protected:
    GfxInfoWebGL() { }
};

}
}

#endif

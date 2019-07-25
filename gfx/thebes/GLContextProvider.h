



































#ifndef GLCONTEXTPROVIDER_H_
#define GLCONTEXTPROVIDER_H_

#include "GLContext.h"
#include "gfxTypes.h"
#include "gfxPoint.h"
#include "nsAutoPtr.h"

class nsIWidget;
class gfxASurface;

namespace mozilla {
namespace gl {

struct THEBES_API ContextFormat {
    static const ContextFormat BasicRGBA32Format;

    enum StandardContextFormat {
        Empty,
        BasicRGBA32,
        StrictBasicRGBA32,
        BasicRGBX32,
        StrictBasicRGBX32
    };

    ContextFormat(const StandardContextFormat cf) {
        memset(this, 0, sizeof(ContextFormat));

        switch (cf) {
        case BasicRGBA32:
            red = green = blue = alpha = 8;
            minRed = minGreen = minBlue = minAlpha = 1;
            break;

        case StrictBasicRGBA32:
            red = green = blue = alpha = 8;
            minRed = minGreen = minBlue = minAlpha = 8;
            break;

        case BasicRGBX32:
            red = green = blue = 8;
            minRed = minGreen = minBlue = 1;
            break;

        case StrictBasicRGBX32:
            red = green = blue = alpha = 8;
            minRed = minGreen = minBlue = 8;
            break;

        default:
            break;
        }
    }

    int depth, minDepth;
    int stencil, minStencil;
    int red, minRed;
    int green, minGreen;
    int blue, minBlue;
    int alpha, minAlpha;

    int colorBits() const { return red + green + blue; }
};

#define IN_GL_CONTEXT_PROVIDER_H


#define GL_CONTEXT_PROVIDER_NAME GLContextProviderNull
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME

#define GL_CONTEXT_PROVIDER_NAME GLContextProviderOSMesa
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME

#ifdef XP_WIN
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderWGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderWGL
#define DEFAULT_IMPL WGL
#endif

#ifdef XP_MACOSX
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderCGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderCGL
#endif

#if defined(ANDROID) || defined(MOZ_PLATFORM_MAEMO)
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderEGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderEGL
#endif


#if defined(MOZ_X11) && !defined(GL_CONTEXT_PROVIDER_DEFAULT)
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderGLX
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderGLX
#endif

#ifdef GL_CONTEXT_PROVIDER_DEFAULT
typedef GL_CONTEXT_PROVIDER_DEFAULT GLContextProvider;
#else
typedef GLContextProviderNull GLContextProvider;
#endif

#undef IN_GL_CONTEXT_PROVIDER_H

}
}

#endif

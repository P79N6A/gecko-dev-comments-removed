



































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

class THEBES_API GLContextProvider 
{
public:
    struct ContextFormat {
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

    







    already_AddRefed<GLContext> CreatePBuffer(const gfxIntSize &aSize,
                                              const ContextFormat& aFormat = ContextFormat::BasicRGBA32Format);

    






    already_AddRefed<GLContext> CreateForWindow(nsIWidget *aWidget);

    






    already_AddRefed<GLContext> CreateForNativePixmapSurface(gfxASurface *aSurface);
};


class THEBES_API GLContextProviderOSMesa
{
public:
    typedef GLContextProvider::ContextFormat ContextFormat;

    







    static already_AddRefed<GLContext> CreatePBuffer(const gfxIntSize &aSize,
                                              const ContextFormat& aFormat = ContextFormat::BasicRGBA32Format);

    






    static already_AddRefed<GLContext> CreateForWindow(nsIWidget *aWidget);

    






    static already_AddRefed<GLContext> CreateForNativePixmapSurface(gfxASurface *aSurface);
};

extern GLContextProvider THEBES_API sGLContextProvider;

}
}

#endif

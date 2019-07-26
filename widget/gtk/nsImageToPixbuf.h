




#ifndef NSIMAGETOPIXBUF_H_
#define NSIMAGETOPIXBUF_H_

#include "nsIImageToPixbuf.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace gfx {
class SourceSurface;
}
}

class nsImageToPixbuf MOZ_FINAL : public nsIImageToPixbuf {
    typedef mozilla::gfx::SourceSurface SourceSurface;

    public:
        NS_DECL_ISUPPORTS
        NS_IMETHOD_(GdkPixbuf*) ConvertImageToPixbuf(imgIContainer* aImage);

        
        
        



        static GdkPixbuf* ImageToPixbuf(imgIContainer * aImage);
        static GdkPixbuf* SourceSurfaceToPixbuf(SourceSurface* aSurface,
                                                int32_t aWidth,
                                                int32_t aHeight);

    private:
        ~nsImageToPixbuf() {}
};



#define NS_IMAGE_TO_PIXBUF_CID \
{ 0xfc2389b8, 0xc650, 0x4093, \
  { 0x9e, 0x42, 0xb0, 0x5e, 0x5f, 0x06, 0x85, 0xb7 } }

#endif

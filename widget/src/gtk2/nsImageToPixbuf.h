




































#ifndef NSIMAGETOPIXBUF_H_
#define NSIMAGETOPIXBUF_H_

#include "nsIImageToPixbuf.h"

class gfxASurface;
class gfxPattern;
class gfxImageSurface;

class nsImageToPixbuf : public nsIImageToPixbuf {
    public:
        NS_DECL_ISUPPORTS
        NS_IMETHOD_(GdkPixbuf*) ConvertImageToPixbuf(imgIContainer* aImage);

        
        
        static GdkPixbuf* ImageToPixbuf(imgIContainer * aImage);
        static GdkPixbuf* SurfaceToPixbuf(gfxASurface* aSurface,
                                          PRInt32 aWidth, PRInt32 aHeight);
    private:
        static GdkPixbuf* ImgSurfaceToPixbuf(gfxImageSurface* aImgSurface,
                                             PRInt32 aWidth, PRInt32 aHeight);
        ~nsImageToPixbuf() {}
};



#define NS_IMAGE_TO_PIXBUF_CID \
{ 0xfc2389b8, 0xc650, 0x4093, \
  { 0x9e, 0x42, 0xb0, 0x5e, 0x5f, 0x06, 0x85, 0xb7 } }

#endif

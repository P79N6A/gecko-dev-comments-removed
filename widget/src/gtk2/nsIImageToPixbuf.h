



































#ifndef NSIIMAGETOPIXBUF_H_
#define NSIIMAGETOPIXBUF_H_

#include "nsISupports.h"


#define NSIIMAGETOPIXBUF_IID \
{ 0xdfa4ac93, 0x83f2, 0x4ab8, \
  { 0x9b, 0x2a, 0x0f, 0xf7, 0x02, 0x2a, 0xeb, 0xe2 } }

class nsIImage;
typedef struct _GdkPixbuf GdkPixbuf;






class nsIImageToPixbuf : public nsISupports {
    public:
        NS_DECLARE_STATIC_IID_ACCESSOR(NSIIMAGETOPIXBUF_IID)

        NS_IMETHOD_(GdkPixbuf*) ConvertImageToPixbuf(nsIImage* aImage) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImageToPixbuf, NSIIMAGETOPIXBUF_IID)

#endif

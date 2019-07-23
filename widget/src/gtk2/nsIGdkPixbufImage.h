



































#ifndef NSIGDKPIXBUFIMAGE_H_
#define NSIGDKPIXBUFIMAGE_H_

#include "nsIImage.h"


#define NSIGDKPIXBUFIMAGE_IID \
{ 0x348a4834, 0x9d5b, 0xd911, { 0x87, 0xc6, 0x0, 0x2, 0x44, 0x21, 0x2b, 0xcb } }

typedef struct _GdkPixbuf GdkPixbuf;




class nsIGdkPixbufImage : public nsIImage {
  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NSIGDKPIXBUFIMAGE_IID)

    



    NS_IMETHOD_(GdkPixbuf*) GetGdkPixbuf() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGdkPixbufImage, NSIGDKPIXBUFIMAGE_IID)

#endif

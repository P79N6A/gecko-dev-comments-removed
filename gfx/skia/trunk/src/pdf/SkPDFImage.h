








#ifndef SkPDFImage_DEFINED
#define SkPDFImage_DEFINED

#include "SkPicture.h"
#include "SkPDFDevice.h"
#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkRefCnt.h"

class SkBitmap;
class SkPDFCatalog;
struct SkIRect;









class SkPDFImage : public SkPDFStream {
public:
    






    static SkPDFImage* CreateImage(const SkBitmap& bitmap,
                                   const SkIRect& srcRect,
                                   SkPicture::EncodeBitmap encoder);

    virtual ~SkPDFImage();

    



    SkPDFImage* addSMask(SkPDFImage* mask);

    bool isEmpty() {
        return fSrcRect.isEmpty();
    }

    
    virtual void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                              SkTSet<SkPDFObject*>* newResourceObjects);

private:
    SkBitmap fBitmap;
    bool fIsAlpha;
    SkIRect fSrcRect;
    SkPicture::EncodeBitmap fEncoder;
    bool fStreamValid;

    SkTDArray<SkPDFObject*> fResources;

    













    SkPDFImage(SkStream* stream, const SkBitmap& bitmap, bool isAlpha,
               const SkIRect& srcRect, SkPicture::EncodeBitmap encoder);

    


    SkPDFImage(SkPDFImage& pdfImage);

    
    
    virtual bool populate(SkPDFCatalog* catalog);

    typedef SkPDFStream INHERITED;
};

#endif

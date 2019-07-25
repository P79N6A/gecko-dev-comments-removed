








#ifndef SkPDFImage_DEFINED
#define SkPDFImage_DEFINED

#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkRefCnt.h"

class SkBitmap;
class SkPaint;
class SkPDFCatalog;
struct SkIRect;









class SkPDFImage : public SkPDFStream {
public:
    






    static SkPDFImage* CreateImage(const SkBitmap& bitmap,
                                   const SkIRect& srcRect,
                                   const SkPaint& paint);

    virtual ~SkPDFImage();

    



    SkPDFImage* addSMask(SkPDFImage* mask);

    
    virtual void getResources(SkTDArray<SkPDFObject*>* resourceList);

private:
    SkTDArray<SkPDFObject*> fResources;

    








    SkPDFImage(SkStream* imageData, const SkBitmap& bitmap,
               const SkIRect& srcRect, bool alpha, const SkPaint& paint);
};

#endif

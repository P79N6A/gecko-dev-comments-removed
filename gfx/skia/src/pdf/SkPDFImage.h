








#ifndef SkPDFImage_DEFINED
#define SkPDFImage_DEFINED

#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkRefCnt.h"

class SkBitmap;
class SkPDFCatalog;
struct SkIRect;









class SkPDFImage : public SkPDFStream {
public:
    






    static SkPDFImage* CreateImage(const SkBitmap& bitmap,
                                   const SkIRect& srcRect);

    virtual ~SkPDFImage();

    



    SkPDFImage* addSMask(SkPDFImage* mask);

    
    virtual void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                              SkTSet<SkPDFObject*>* newResourceObjects);

private:
    SkTDArray<SkPDFObject*> fResources;

    








    SkPDFImage(SkStream* imageData, const SkBitmap& bitmap,
               const SkIRect& srcRect, bool alpha);
};

#endif

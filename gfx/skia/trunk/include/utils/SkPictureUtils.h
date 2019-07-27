






#ifndef SkPictureUtils_DEFINED
#define SkPictureUtils_DEFINED

#include "SkPicture.h"
#include "SkTDArray.h"

class SkData;
struct SkRect;

class SK_API SkPictureUtils {
public:
    









    static SkData* GatherPixelRefs(const SkPicture* pict, const SkRect& area);

    



    class SkPixelRefContainer : public SkRefCnt {
    public:
        virtual void add(SkPixelRef* pr, const SkRect& rect) = 0;

        
        virtual void query(const SkRect& queryRect, SkTDArray<SkPixelRef*> *result) = 0;

    private:
        typedef SkRefCnt INHERITED;
    };

    
    
    class SkPixelRefsAndRectsList : public SkPixelRefContainer {
    public:
        virtual void add(SkPixelRef* pr, const SkRect& rect) SK_OVERRIDE {
            PixelRefAndRect *dst = fArray.append();

            dst->fPixelRef = pr;
            dst->fRect = rect;
        }

        virtual void query(const SkRect& queryRect, SkTDArray<SkPixelRef*> *result) SK_OVERRIDE {
            for (int i = 0; i < fArray.count(); ++i) {
                if (SkRect::Intersects(fArray[i].fRect, queryRect)) {
                    *result->append() = fArray[i].fPixelRef;
                }
            }
        }

    private:
        struct PixelRefAndRect {
            SkPixelRef* fPixelRef;
            SkRect      fRect;
        };

        SkTDArray<PixelRefAndRect> fArray;

        typedef SkPixelRefContainer INHERITED;
    };

    



    static void GatherPixelRefsAndRects(SkPicture* pict, SkPixelRefContainer* prCont);
};

#endif

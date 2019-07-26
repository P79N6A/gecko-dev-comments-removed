






#ifndef SkImageFilter_DEFINED
#define SkImageFilter_DEFINED

#include "SkFlattenable.h"

class SkBitmap;
class SkDevice;
class SkMatrix;
struct SkIPoint;
struct SkIRect;
struct SkRect;
class GrCustomStage;
class GrTexture;






















class SK_API SkImageFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkImageFilter)

    class Proxy {
    public:
        virtual ~Proxy() {};

        virtual SkDevice* createDevice(int width, int height) = 0;
        
        virtual bool canHandleImageFilter(SkImageFilter*) = 0;
        
        
        virtual bool filterImage(SkImageFilter*, const SkBitmap& src,
                                 const SkMatrix& ctm,
                                 SkBitmap* result, SkIPoint* offset) = 0;
    };

    












    bool filterImage(Proxy*, const SkBitmap& src, const SkMatrix& ctm,
                     SkBitmap* result, SkIPoint* offset);

    



    bool filterBounds(const SkIRect& src, const SkMatrix& ctm, SkIRect* dst);

    








    virtual bool asNewCustomStage(GrCustomStage** stage, GrTexture*) const;

    





    virtual bool canFilterImageGPU() const;

    






    virtual GrTexture* onFilterImageGPU(GrTexture* texture, const SkRect& rect);

protected:
    SkImageFilter() {}
    explicit SkImageFilter(SkFlattenableReadBuffer& rb) : INHERITED(rb) {}

    
    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const SkMatrix&,
                               SkBitmap* result, SkIPoint* offset);
    
    virtual bool onFilterBounds(const SkIRect&, const SkMatrix&, SkIRect*);

private:
    typedef SkFlattenable INHERITED;
};

#endif








#ifndef SkImageFilter_DEFINED
#define SkImageFilter_DEFINED

#include "SkFlattenable.h"

class SkBitmap;
class SkDevice;
class SkMatrix;
struct SkPoint;






















class SK_API SkImageFilter : public SkFlattenable {
public:
    class Proxy {
    public:
        virtual SkDevice* createDevice(int width, int height) = 0;
        
        
        
        virtual bool filterImage(SkImageFilter*, const SkBitmap& src,
                                 const SkMatrix& ctm,
                                 SkBitmap* result, SkIPoint* offset) = 0;
    };

    












    bool filterImage(Proxy*, const SkBitmap& src, const SkMatrix& ctm,
                     SkBitmap* result, SkIPoint* offset);

    



    bool filterBounds(const SkIRect& src, const SkMatrix& ctm, SkIRect* dst);

    





    virtual bool asABlur(SkSize* sigma) const;

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

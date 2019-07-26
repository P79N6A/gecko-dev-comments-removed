






#ifndef SkImageFilter_DEFINED
#define SkImageFilter_DEFINED

#include "SkFlattenable.h"
#include "SkMatrix.h"
#include "SkRect.h"

class SkBitmap;
class SkColorFilter;
class SkBaseDevice;
struct SkIPoint;
class SkShader;
class GrEffectRef;
class GrTexture;








class SK_API SkImageFilter : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkImageFilter)

    class CropRect {
    public:
        enum CropEdge {
            kHasLeft_CropEdge   = 0x01,
            kHasTop_CropEdge    = 0x02,
            kHasRight_CropEdge  = 0x04,
            kHasBottom_CropEdge = 0x08,
            kHasAll_CropEdge    = 0x0F,
        };
        CropRect() {}
        explicit CropRect(const SkRect& rect, uint32_t flags = kHasAll_CropEdge) : fRect(rect), fFlags(flags) {}
        uint32_t flags() const { return fFlags; }
        const SkRect& rect() const { return fRect; }
    private:
        SkRect fRect;
        uint32_t fFlags;
    };

    class Context {
    public:
        Context(const SkMatrix& ctm, const SkIRect& clipBounds) :
            fCTM(ctm), fClipBounds(clipBounds) {
        }
        const SkMatrix& ctm() const { return fCTM; }
        const SkIRect& clipBounds() const { return fClipBounds; }
    private:
        SkMatrix fCTM;
        SkIRect  fClipBounds;
    };

    class Proxy {
    public:
        virtual ~Proxy() {};

        virtual SkBaseDevice* createDevice(int width, int height) = 0;
        
        virtual bool canHandleImageFilter(const SkImageFilter*) = 0;
        
        
        virtual bool filterImage(const SkImageFilter*, const SkBitmap& src,
                                 const Context&,
                                 SkBitmap* result, SkIPoint* offset) = 0;
    };

    












    bool filterImage(Proxy*, const SkBitmap& src, const Context&,
                     SkBitmap* result, SkIPoint* offset) const;

    



    bool filterBounds(const SkIRect& src, const SkMatrix& ctm, SkIRect* dst) const;

    






    virtual bool canFilterImageGPU() const;

    









    virtual bool filterImageGPU(Proxy*, const SkBitmap& src, const Context&,
                                SkBitmap* result, SkIPoint* offset) const;

    






    virtual bool asColorFilter(SkColorFilter** filterPtr) const;

    



    int countInputs() const { return fInputCount; }

    



    SkImageFilter* getInput(int i) const {
        SkASSERT(i < fInputCount);
        return fInputs[i];
    }

    









    bool cropRectIsSet() const { return fCropRect.flags() != 0x0; }

    
    virtual void computeFastBounds(const SkRect&, SkRect*) const;

#ifdef SK_SUPPORT_GPU
    


    static void WrapTexture(GrTexture* texture, int width, int height, SkBitmap* result);

    



    bool getInputResultGPU(SkImageFilter::Proxy* proxy, const SkBitmap& src, const Context&,
                           SkBitmap* result, SkIPoint* offset) const;
#endif

    SK_DEFINE_FLATTENABLE_TYPE(SkImageFilter)

protected:
    SkImageFilter(int inputCount, SkImageFilter** inputs, const CropRect* cropRect = NULL);

    
    explicit SkImageFilter(SkImageFilter* input, const CropRect* cropRect = NULL);

    
    SkImageFilter(SkImageFilter* input1, SkImageFilter* input2, const CropRect* cropRect = NULL);

    virtual ~SkImageFilter();

    






    explicit SkImageFilter(int inputCount, SkReadBuffer& rb);

    virtual void flatten(SkWriteBuffer& wb) const SK_OVERRIDE;

    















    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const Context&,
                               SkBitmap* result, SkIPoint* offset) const;
    
    
    
    
    
    
    
    virtual bool onFilterBounds(const SkIRect&, const SkMatrix&, SkIRect*) const;

    





    bool applyCropRect(const Context&, const SkBitmap& src, const SkIPoint& srcOffset,
                       SkIRect* bounds) const;

    







    bool applyCropRect(const Context&, Proxy* proxy, const SkBitmap& src, SkIPoint* srcOffset,
                       SkIRect* bounds, SkBitmap* result) const;

    














    virtual bool asNewEffect(GrEffectRef** effect,
                             GrTexture*,
                             const SkMatrix& matrix,
                             const SkIRect& bounds) const;


private:
    typedef SkFlattenable INHERITED;
    int fInputCount;
    SkImageFilter** fInputs;
    CropRect fCropRect;
};

#endif

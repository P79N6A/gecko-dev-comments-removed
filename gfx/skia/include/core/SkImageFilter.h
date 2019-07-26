






#ifndef SkImageFilter_DEFINED
#define SkImageFilter_DEFINED

#include "SkFlattenable.h"

class SkBitmap;
class SkColorFilter;
class SkDevice;
class SkMatrix;
struct SkIPoint;
struct SkIRect;
class SkShader;
class GrEffectRef;
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

    











    virtual bool asNewEffect(GrEffectRef** effect, GrTexture*) const;

    





    virtual bool canFilterImageGPU() const;

    






    virtual bool filterImageGPU(Proxy*, const SkBitmap& src, SkBitmap* result);

    






    virtual bool asColorFilter(SkColorFilter** filterPtr) const;

    



    int countInputs() const { return fInputCount; }

    



    SkImageFilter* getInput(int i) const {
        SkASSERT(i < fInputCount);
        return fInputs[i];
    }

protected:
    SkImageFilter(int inputCount, SkImageFilter** inputs);

    
    explicit SkImageFilter(SkImageFilter* input);

    
    SkImageFilter(SkImageFilter* input1, SkImageFilter* input2);

    virtual ~SkImageFilter();

    explicit SkImageFilter(SkFlattenableReadBuffer& rb);

    virtual void flatten(SkFlattenableWriteBuffer& wb) const SK_OVERRIDE;

    
    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const SkMatrix&,
                               SkBitmap* result, SkIPoint* offset);
    
    virtual bool onFilterBounds(const SkIRect&, const SkMatrix&, SkIRect*);

    
    
    SkBitmap getInputResult(int index, Proxy*, const SkBitmap& src, const SkMatrix&,
                            SkIPoint*);

private:
    typedef SkFlattenable INHERITED;
    int fInputCount;
    SkImageFilter** fInputs;
};

#endif

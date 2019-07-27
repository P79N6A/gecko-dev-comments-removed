






#ifndef SkImageFilter_DEFINED
#define SkImageFilter_DEFINED

#include "SkFlattenable.h"
#include "SkMatrix.h"
#include "SkRect.h"
#include "SkTemplates.h"

class SkBitmap;
class SkColorFilter;
class SkBaseDevice;
struct SkIPoint;
class GrEffect;
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

    class SK_API Cache : public SkRefCnt {
    public:
        
        
        static Cache* Create(int minChildren = 2);
        virtual ~Cache() {}
        virtual bool get(const SkImageFilter* key, SkBitmap* result, SkIPoint* offset) = 0;
        virtual void set(const SkImageFilter* key,
                         const SkBitmap& result, const SkIPoint& offset) = 0;
        virtual void remove(const SkImageFilter* key) = 0;
    };

    class Context {
    public:
        Context(const SkMatrix& ctm, const SkIRect& clipBounds, Cache* cache) :
            fCTM(ctm), fClipBounds(clipBounds), fCache(cache) {
        }
        const SkMatrix& ctm() const { return fCTM; }
        const SkIRect& clipBounds() const { return fClipBounds; }
        Cache* cache() const { return fCache; }
    private:
        SkMatrix fCTM;
        SkIRect  fClipBounds;
        Cache*   fCache;
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

    



    static void SetExternalCache(Cache* cache);

    


    static Cache* GetExternalCache();

    SK_DEFINE_FLATTENABLE_TYPE(SkImageFilter)

protected:
    class Common {
    public:
        Common() {}
        ~Common();

        bool unflatten(SkReadBuffer&, int expectedInputs = -1);

        CropRect        cropRect() const { return fCropRect; }
        int             inputCount() const { return fInputs.count(); }
        SkImageFilter** inputs() const { return fInputs.get(); }

        
        
        
        
        void detachInputs(SkImageFilter** inputs);

    private:
        CropRect fCropRect;
        
        SkAutoSTArray<2, SkImageFilter*> fInputs;

        void allocInputs(int count);
    };

    SkImageFilter(int inputCount, SkImageFilter** inputs, const CropRect* cropRect = NULL);

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

    














    virtual bool asNewEffect(GrEffect** effect,
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

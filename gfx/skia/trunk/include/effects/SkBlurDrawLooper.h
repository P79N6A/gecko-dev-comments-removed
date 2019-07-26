







#ifndef SkBlurDrawLooper_DEFINED
#define SkBlurDrawLooper_DEFINED

#include "SkDrawLooper.h"
#include "SkColor.h"

class SkMaskFilter;
class SkColorFilter;






class SK_API SkBlurDrawLooper : public SkDrawLooper {
public:
    enum BlurFlags {
        kNone_BlurFlag = 0x00,
        



        kIgnoreTransform_BlurFlag   = 0x01,
        kOverrideColor_BlurFlag     = 0x02,
        kHighQuality_BlurFlag       = 0x04,
        
        kAll_BlurFlag               = 0x07
    };

    SkBlurDrawLooper(SkColor color, SkScalar sigma, SkScalar dx, SkScalar dy,
                     uint32_t flags = kNone_BlurFlag);


    SkBlurDrawLooper(SkScalar radius, SkScalar dx, SkScalar dy, SkColor color,
                     uint32_t flags = kNone_BlurFlag);
    virtual ~SkBlurDrawLooper();

    virtual SkDrawLooper::Context* createContext(SkCanvas*, void* storage) const SK_OVERRIDE;

    virtual size_t contextSize() const SK_OVERRIDE { return sizeof(BlurDrawLooperContext); }

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkBlurDrawLooper)

protected:
    SkBlurDrawLooper(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkMaskFilter*   fBlur;
    SkColorFilter*  fColorFilter;
    SkScalar        fDx, fDy;
    SkColor         fBlurColor;
    uint32_t        fBlurFlags;

    enum State {
        kBeforeEdge,
        kAfterEdge,
        kDone
    };

    class BlurDrawLooperContext : public SkDrawLooper::Context {
    public:
        explicit BlurDrawLooperContext(const SkBlurDrawLooper* looper);

        virtual bool next(SkCanvas* canvas, SkPaint* paint) SK_OVERRIDE;

    private:
        const SkBlurDrawLooper* fLooper;
        State fState;
    };

    void init(SkScalar sigma, SkScalar dx, SkScalar dy, SkColor color, uint32_t flags);

    typedef SkDrawLooper INHERITED;
};

#endif










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
        
        kAll_BlurFlag = 0x07
    };

    SkBlurDrawLooper(SkScalar radius, SkScalar dx, SkScalar dy, SkColor color, 
                     uint32_t flags = kNone_BlurFlag);
    virtual ~SkBlurDrawLooper();

    
    virtual void init(SkCanvas*);
    virtual bool next(SkCanvas*, SkPaint* paint);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkBlurDrawLooper, (buffer));
    }


protected:
    SkBlurDrawLooper(SkFlattenableReadBuffer&);
    
    virtual void flatten(SkFlattenableWriteBuffer& );
    virtual Factory getFactory() { return CreateProc; }

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
    State   fState;
    
    typedef SkDrawLooper INHERITED;
};

#endif

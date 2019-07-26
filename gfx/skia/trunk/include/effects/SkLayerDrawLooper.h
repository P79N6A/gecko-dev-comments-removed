






#ifndef SkLayerDrawLooper_DEFINED
#define SkLayerDrawLooper_DEFINED

#include "SkDrawLooper.h"
#include "SkPaint.h"
#include "SkPoint.h"
#include "SkXfermode.h"

class SK_API SkLayerDrawLooper : public SkDrawLooper {
public:
    SK_DECLARE_INST_COUNT(SkLayerDrawLooper)

            SkLayerDrawLooper();
    virtual ~SkLayerDrawLooper();

    






    enum Bits {
        kStyle_Bit      = 1 << 0,   
        kTextSkewX_Bit  = 1 << 1,   
        kPathEffect_Bit = 1 << 2,   
        kMaskFilter_Bit = 1 << 3,   
        kShader_Bit     = 1 << 4,   
        kColorFilter_Bit = 1 << 5,  
        kXfermode_Bit   = 1 << 6,   

        






        kEntirePaint_Bits = -1

    };
    typedef int32_t BitFlags;

    
















    struct SK_API LayerInfo {
        uint32_t            fFlagsMask; 
        BitFlags            fPaintBits;
        SkXfermode::Mode    fColorMode;
        SkVector            fOffset;
        bool                fPostTranslate; 

        






        LayerInfo();
    };

    




    SkPaint* addLayer(const LayerInfo&);

    


    void addLayer(SkScalar dx, SkScalar dy);

    


    void addLayer() { this->addLayer(0, 0); }

    
    SkPaint* addLayerOnTop(const LayerInfo&);

    
    virtual void init(SkCanvas*);
    virtual bool next(SkCanvas*, SkPaint* paint);

    SK_DEVELOPER_TO_STRING()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLayerDrawLooper)

protected:
    SkLayerDrawLooper(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    struct Rec {
        Rec*    fNext;
        SkPaint fPaint;
        LayerInfo fInfo;
    };
    Rec*    fRecs;
    Rec*    fTopRec;
    int     fCount;

    
    Rec* fCurrRec;

    static void ApplyInfo(SkPaint* dst, const SkPaint& src, const LayerInfo&);

    class MyRegistrar : public SkFlattenable::Registrar {
    public:
        MyRegistrar();
    };

    typedef SkDrawLooper INHERITED;
};

#endif

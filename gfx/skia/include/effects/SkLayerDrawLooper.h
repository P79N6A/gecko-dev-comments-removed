






#ifndef SkLayerDrawLooper_DEFINED
#define SkLayerDrawLooper_DEFINED

#include "SkDrawLooper.h"
#include "SkXfermode.h"

struct SkPoint;

class SK_API SkLayerDrawLooper : public SkDrawLooper {
public:
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
        
        kEntirePaint_Bits = -1,      
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
    
    
    virtual void init(SkCanvas*);
    virtual bool next(SkCanvas*, SkPaint* paint);

    
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkLayerDrawLooper, (buffer));
    }
    
protected:
    SkLayerDrawLooper(SkFlattenableReadBuffer&);

    
    virtual void flatten(SkFlattenableWriteBuffer& );
    virtual Factory getFactory() { return CreateProc; }
    
private:
    struct Rec {
        Rec*    fNext;
        SkPaint fPaint;
        LayerInfo fInfo;

        static Rec* Reverse(Rec*);
    };
    Rec*    fRecs;
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

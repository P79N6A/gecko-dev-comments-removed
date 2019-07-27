






#ifndef SkLayerDrawLooper_DEFINED
#define SkLayerDrawLooper_DEFINED

#include "SkDrawLooper.h"
#include "SkPaint.h"
#include "SkPoint.h"
#include "SkXfermode.h"

class SK_API SkLayerDrawLooper : public SkDrawLooper {
public:
    SK_DECLARE_INST_COUNT(SkLayerDrawLooper)

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
        BitFlags            fPaintBits;
        SkXfermode::Mode    fColorMode;
        SkVector            fOffset;
        bool                fPostTranslate; 

        






        LayerInfo();
    };

    virtual SkDrawLooper::Context* createContext(SkCanvas*, void* storage) const SK_OVERRIDE;

    virtual size_t contextSize() const SK_OVERRIDE { return sizeof(LayerDrawLooperContext); }

    virtual bool asABlurShadow(BlurShadowRec* rec) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()

    
    virtual Factory getFactory() const SK_OVERRIDE { return CreateProc; }
    static SkFlattenable* CreateProc(SkReadBuffer& buffer);

protected:
    SkLayerDrawLooper();

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

    
    class LayerDrawLooperContext : public SkDrawLooper::Context {
    public:
        explicit LayerDrawLooperContext(const SkLayerDrawLooper* looper);

    protected:
        virtual bool next(SkCanvas*, SkPaint* paint) SK_OVERRIDE;

    private:
        Rec* fCurrRec;

        static void ApplyInfo(SkPaint* dst, const SkPaint& src, const LayerInfo&);
    };

    class MyRegistrar : public SkFlattenable::Registrar {
    public:
        MyRegistrar();
    };

    typedef SkDrawLooper INHERITED;

public:
    class SK_API Builder {
    public:
        Builder();
        ~Builder();

        




        SkPaint* addLayer(const LayerInfo&);

        


        void addLayer(SkScalar dx, SkScalar dy);

        


        void addLayer() { this->addLayer(0, 0); }

        
        SkPaint* addLayerOnTop(const LayerInfo&);

        



        SkLayerDrawLooper* detachLooper();

    private:
        Rec* fRecs;
        Rec* fTopRec;
        int  fCount;
    };
};

#endif

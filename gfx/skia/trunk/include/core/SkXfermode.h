








#ifndef SkXfermode_DEFINED
#define SkXfermode_DEFINED

#include "SkFlattenable.h"
#include "SkColor.h"

class GrEffectRef;
class GrTexture;
class SkString;












class SK_API SkXfermode : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkXfermode)

    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const;
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const;
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const;

    

    enum Coeff {
        kZero_Coeff,    
        kOne_Coeff,     
        kSC_Coeff,      
        kISC_Coeff,     
        kDC_Coeff,      
        kIDC_Coeff,     
        kSA_Coeff,      
        kISA_Coeff,     
        kDA_Coeff,      
        kIDA_Coeff,     

        kCoeffCount
    };

    














    virtual bool asCoeff(Coeff* src, Coeff* dst) const;

    



    static bool AsCoeff(const SkXfermode*, Coeff* src, Coeff* dst);

    










    enum Mode {
        kClear_Mode,    
        kSrc_Mode,      
        kDst_Mode,      
        kSrcOver_Mode,  
        kDstOver_Mode,  
        kSrcIn_Mode,    
        kDstIn_Mode,    
        kSrcOut_Mode,   
        kDstOut_Mode,   
        kSrcATop_Mode,  
        kDstATop_Mode,  
        kXor_Mode,      
        kPlus_Mode,     
        kModulate_Mode, 

        
        
        kScreen_Mode,
        kLastCoeffMode = kScreen_Mode,

        kOverlay_Mode,
        kDarken_Mode,
        kLighten_Mode,
        kColorDodge_Mode,
        kColorBurn_Mode,
        kHardLight_Mode,
        kSoftLight_Mode,
        kDifference_Mode,
        kExclusion_Mode,
        kMultiply_Mode,
        kLastSeparableMode = kMultiply_Mode,

        kHue_Mode,
        kSaturation_Mode,
        kColor_Mode,
        kLuminosity_Mode,
        kLastMode = kLuminosity_Mode
    };

    


    static const char* ModeName(Mode);

    




    virtual bool asMode(Mode* mode) const;

    



    static bool AsMode(const SkXfermode*, Mode* mode);

    









    static bool IsMode(const SkXfermode* xfer, Mode mode);

    

    static SkXfermode* Create(Mode mode);

    


    static SkXfermodeProc GetProc(Mode mode);

    




    static SkXfermodeProc16 GetProc16(Mode mode, SkColor srcColor);

    





    static bool ModeAsCoeff(Mode mode, Coeff* src, Coeff* dst);

    SK_ATTR_DEPRECATED("use AsMode(...)")
    static bool IsMode(const SkXfermode* xfer, Mode* mode) {
        return AsMode(xfer, mode);
    }

    








    virtual bool asNewEffect(GrEffectRef** effect, GrTexture* background = NULL) const;

    




    static bool AsNewEffectOrCoeff(SkXfermode*,
                                   GrEffectRef** effect,
                                   Coeff* src,
                                   Coeff* dst,
                                   GrTexture* background = NULL);

    SK_TO_STRING_PUREVIRT()
    SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()
    SK_DEFINE_FLATTENABLE_TYPE(SkXfermode)

protected:
    SkXfermode(SkReadBuffer& rb) : SkFlattenable(rb) {}

    







    virtual SkPMColor xferColor(SkPMColor src, SkPMColor dst) const;

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkXfermode() {}

private:
    enum {
        kModeCount = kLastMode + 1
    };

    friend class SkGraphics;
    static void Term();

    typedef SkFlattenable INHERITED;
};








class SkProcXfermode : public SkXfermode {
public:
    static SkProcXfermode* Create(SkXfermodeProc proc) {
        return SkNEW_ARGS(SkProcXfermode, (proc));
    }

    
    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkProcXfermode)

protected:
    SkProcXfermode(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    void setProc(SkXfermodeProc proc) {
        fProc = proc;
    }

    SkXfermodeProc getProc() const {
        return fProc;
    }

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkProcXfermode(SkXfermodeProc proc) : fProc(proc) {}

private:
    SkXfermodeProc  fProc;

    typedef SkXfermode INHERITED;
};

#endif

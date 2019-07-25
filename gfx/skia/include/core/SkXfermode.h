








#ifndef SkXfermode_DEFINED
#define SkXfermode_DEFINED

#include "SkFlattenable.h"
#include "SkColor.h"









class SK_API SkXfermode : public SkFlattenable {
public:
    SkXfermode() {}

    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]);
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]);
    virtual void xfer4444(uint16_t dst[], const SkPMColor src[], int count,
                          const SkAlpha aa[]);
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]);

    

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

    














    virtual bool asCoeff(Coeff* src, Coeff* dst);

    



    static bool AsCoeff(SkXfermode*, Coeff* src, Coeff* dst);

    







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
        kMultiply_Mode, 
        
        
        kCoeffModesCnt, 
        
        kScreen_Mode = kCoeffModesCnt,
        kOverlay_Mode,
        kDarken_Mode,
        kLighten_Mode,
        kColorDodge_Mode,
        kColorBurn_Mode,
        kHardLight_Mode,
        kSoftLight_Mode,
        kDifference_Mode,
        kExclusion_Mode,

        kLastMode = kExclusion_Mode
    };

    




    virtual bool asMode(Mode* mode);

    



    static bool AsMode(SkXfermode*, Mode* mode);

    

    static SkXfermode* Create(Mode mode);

    


    static SkXfermodeProc GetProc(Mode mode);

    




    static SkXfermodeProc16 GetProc16(Mode mode, SkColor srcColor);

    





    static bool ModeAsCoeff(Mode mode, Coeff* src, Coeff* dst);

    
    static bool IsMode(SkXfermode* xfer, Mode* mode) {
        return AsMode(xfer, mode);
    }

protected:
    SkXfermode(SkFlattenableReadBuffer& rb) : SkFlattenable(rb) {}

    







    virtual SkPMColor xferColor(SkPMColor src, SkPMColor dst);

private:
    enum {
        kModeCount = kLastMode + 1
    };
    typedef SkFlattenable INHERITED;
};








class SkProcXfermode : public SkXfermode {
public:
    SkProcXfermode(SkXfermodeProc proc) : fProc(proc) {}

    
    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) SK_OVERRIDE;
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) SK_OVERRIDE;
    virtual void xfer4444(uint16_t dst[], const SkPMColor src[], int count,
                          const SkAlpha aa[]) SK_OVERRIDE;
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) SK_OVERRIDE;

    
    virtual Factory getFactory() SK_OVERRIDE { return CreateProc; }
    virtual void    flatten(SkFlattenableWriteBuffer&) SK_OVERRIDE;

protected:
    SkProcXfermode(SkFlattenableReadBuffer&);

    
    void setProc(SkXfermodeProc proc) {
        fProc = proc;
    }

private:
    SkXfermodeProc  fProc;

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkProcXfermode, (buffer)); }

    typedef SkXfermode INHERITED;
};

#endif

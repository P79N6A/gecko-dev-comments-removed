








#ifndef GrStencil_DEFINED
#define GrStencil_DEFINED

#include "GrTypes.h"

























enum GrStencilFunc {
    kAlways_StencilFunc = 0,
    kNever_StencilFunc,
    kGreater_StencilFunc,
    kGEqual_StencilFunc,
    kLess_StencilFunc,
    kLEqual_StencilFunc,
    kEqual_StencilFunc,
    kNotEqual_StencilFunc,

    
    
    
    
    
    
    

    
    kAlwaysIfInClip_StencilFunc,
    kEqualIfInClip_StencilFunc,
    kLessIfInClip_StencilFunc,
    kLEqualIfInClip_StencilFunc,
    kNonZeroIfInClip_StencilFunc, 

    
    kStencilFuncCount,
    kClipStencilFuncCount = kNonZeroIfInClip_StencilFunc -
                            kAlwaysIfInClip_StencilFunc + 1,
    kBasicStencilFuncCount = kStencilFuncCount - kClipStencilFuncCount
};




enum GrStencilOp {
    kKeep_StencilOp = 0,    
    kReplace_StencilOp,     
    kIncWrap_StencilOp,     
    kIncClamp_StencilOp,    
    kDecWrap_StencilOp,     
    kDecClamp_StencilOp,    
    kZero_StencilOp,        
    kInvert_StencilOp,      

    kStencilOpCount
};




struct GrStencilSettings {
    GrStencilOp   fFrontPassOp;     
    GrStencilOp   fBackPassOp;      
    GrStencilOp   fFrontFailOp;     
    GrStencilOp   fBackFailOp;      
    GrStencilFunc fFrontFunc;       
    GrStencilFunc fBackFunc;        
    unsigned int fFrontFuncMask;    
    unsigned int fBackFuncMask;     
    unsigned int fFrontFuncRef;     
    unsigned int fBackFuncRef;      
    unsigned int fFrontWriteMask;   
    unsigned int fBackWriteMask;    

    bool operator == (const GrStencilSettings& s) const {
        
        GR_STATIC_ASSERT(0 == sizeof(GrStencilOp)%4);
        GR_STATIC_ASSERT(0 == sizeof(GrStencilFunc)%4);
        GR_STATIC_ASSERT(sizeof(GrStencilSettings) ==
                        4*sizeof(GrStencilOp) +
                        2*sizeof(GrStencilFunc) +
                        6*sizeof(unsigned int));
        return 0 == memcmp(this, &s, sizeof(GrStencilSettings));
    }

    bool operator != (const GrStencilSettings& s) const {
        return !(*this == s);
    }

    GrStencilSettings& operator =(const GrStencilSettings& s) {
        memcpy(this, &s, sizeof(GrStencilSettings));
        return *this;
    }

    void setSame(GrStencilOp passOp,
                 GrStencilOp failOp,
                 GrStencilFunc func,
                 unsigned int funcMask,
                 unsigned int funcRef,
                 unsigned int writeMask) {
        fFrontPassOp        = passOp;
        fBackPassOp         = passOp;
        fFrontFailOp        = failOp;
        fBackFailOp         = failOp;
        fFrontFunc          = func;
        fBackFunc           = func;
        fFrontFuncMask      = funcMask;
        fBackFuncMask       = funcMask;
        fFrontFuncRef       = funcRef;
        fBackFuncRef        = funcRef;
        fFrontWriteMask     = writeMask;
        fBackWriteMask      = writeMask;
    }

    
    static const GrStencilSettings gDisabled;
    void setDisabled() {
        *this = gDisabled;
    }
    bool isDisabled() const {
        return kKeep_StencilOp == fFrontPassOp   &&
               kKeep_StencilOp == fBackPassOp    &&
               kKeep_StencilOp == fFrontFailOp   &&
               kKeep_StencilOp == fBackFailOp   &&
               kAlways_StencilFunc == fFrontFunc &&
               kAlways_StencilFunc == fBackFunc;
    }
    bool doesWrite() const {
        return !((kNever_StencilFunc == fFrontFunc ||
                  kKeep_StencilOp == fFrontPassOp)  &&
                 (kNever_StencilFunc == fBackFunc ||
                  kKeep_StencilOp == fBackPassOp)    &&
                 (kAlways_StencilFunc == fFrontFunc ||
                  kKeep_StencilOp == fFrontFailOp)  &&
                 (kAlways_StencilFunc == fBackFunc ||
                  kKeep_StencilOp == fBackFailOp));
    }
    void invalidate()  {
        
        fFrontPassOp = (GrStencilOp)-1;
    }

private:
    friend class GrGpu;

    enum {
        kMaxStencilClipPasses = 2  
                                   
    };

    






















    static bool GetClipPasses(GrSetOp op, 
                              bool canBeDirect,
                              unsigned int stencilClipMask,
                              bool invertedFill,
                              int* numPasses,
                              GrStencilSettings settings[kMaxStencilClipPasses]);
};

#endif










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










struct GrStencilSettingsStruct {
    GrStencilOp fFrontPassOp : 8;    
    GrStencilOp fBackPassOp : 8;     
    GrStencilOp fFrontFailOp : 8;    
    GrStencilOp fBackFailOp : 8;     
    GrStencilFunc fFrontFunc : 8;    
    GrStencilFunc fBackFunc : 8;     
    int fPad0 : 8;
    int fPad1 : 8;
    unsigned short fFrontFuncMask;   
    unsigned short fBackFuncMask;    
    unsigned short fFrontFuncRef;    
    unsigned short fBackFuncRef;     
    unsigned short fFrontWriteMask;  
    unsigned short fBackWriteMask;   
    mutable uint32_t fFlags;
};

GR_STATIC_ASSERT(sizeof(GrStencilSettingsStruct) % 4 == 0);
GR_STATIC_ASSERT(sizeof(GrStencilSettingsStruct) ==
                 4*sizeof(uint8_t) + 
                 2*sizeof(uint8_t) + 
                 2*sizeof(uint8_t) + 
                 2*sizeof(unsigned short) + 
                 2*sizeof(unsigned short) + 
                 2*sizeof(unsigned short) + 
                 sizeof(uint32_t)); 




class GrStencilSettings : private GrStencilSettingsStruct {

public:
    GrStencilSettings() {
        fPad0 = fPad1 = 0;
        this->setDisabled();
    }
    
    GrStencilOp frontPassOp() const { return fFrontPassOp; }
    GrStencilOp backPassOp() const { return fBackPassOp; }
    GrStencilOp frontFailOp() const { return fFrontFailOp; }
    GrStencilOp backFailOp() const { return fBackFailOp; }
    GrStencilFunc frontFunc() const { return fFrontFunc; }
    GrStencilFunc backFunc() const { return fBackFunc; }
    unsigned short frontFuncMask() const { return fFrontFuncMask; }
    unsigned short backFuncMask() const { return fBackFuncMask; }
    unsigned short frontFuncRef() const { return fFrontFuncRef; }
    unsigned short backFuncRef() const { return fBackFuncRef; }
    unsigned short frontWriteMask() const {return fFrontWriteMask; }
    unsigned short backWriteMask() const { return fBackWriteMask; }

    void setFrontPassOp(GrStencilOp op) { fFrontPassOp = op; fFlags = 0;}
    void setBackPassOp(GrStencilOp op) { fBackPassOp = op; fFlags = 0;}
    void setFrontFailOp(GrStencilOp op) {fFrontFailOp = op; fFlags = 0;}
    void setBackFailOp(GrStencilOp op) { fBackFailOp = op; fFlags = 0;}
    void setFrontFunc(GrStencilFunc func) { fFrontFunc = func; fFlags = 0;}
    void setBackFunc(GrStencilFunc func) { fBackFunc = func; fFlags = 0;}
    void setFrontFuncMask(unsigned short mask) { fFrontFuncMask = mask; }
    void setBackFuncMask(unsigned short mask) { fBackFuncMask = mask; }
    void setFrontFuncRef(unsigned short ref) { fFrontFuncRef = ref; }
    void setBackFuncRef(unsigned short ref) { fBackFuncRef = ref; }
    void setFrontWriteMask(unsigned short writeMask) { fFrontWriteMask = writeMask; }
    void setBackWriteMask(unsigned short writeMask) { fBackWriteMask = writeMask; }

    void setSame(GrStencilOp passOp,
                 GrStencilOp failOp,
                 GrStencilFunc func,
                 unsigned short funcMask,
                 unsigned short funcRef,
                 unsigned short writeMask) {
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
        fFlags = 0;
    }

    void setDisabled() {
        memset(this, 0, sizeof(*this));
        GR_STATIC_ASSERT(0 == kKeep_StencilOp);
        GR_STATIC_ASSERT(0 == kAlways_StencilFunc);
        fFlags = kIsDisabled_Flag | kDoesNotWrite_Flag;
    }

    bool isDisabled() const {
        if (fFlags & kIsDisabled_Flag) {
            return true;
        }
        if (fFlags & kNotDisabled_Flag) {
            return false;
        }
        bool disabled = kKeep_StencilOp == fFrontPassOp   &&
                        kKeep_StencilOp == fBackPassOp    &&
                        kKeep_StencilOp == fFrontFailOp   &&
                        kKeep_StencilOp == fBackFailOp   &&
                        kAlways_StencilFunc == fFrontFunc &&
                        kAlways_StencilFunc == fBackFunc;
        fFlags |= disabled ? kIsDisabled_Flag : kNotDisabled_Flag;
        return disabled;
    }

    bool doesWrite() const {
        if (fFlags & kDoesWrite_Flag) {
            return true;
        }
        if (fFlags & kDoesNotWrite_Flag) {
            return false;
        }
        bool writes = !((kNever_StencilFunc == fFrontFunc ||
                         kKeep_StencilOp == fFrontPassOp)  &&
                        (kNever_StencilFunc == fBackFunc ||
                         kKeep_StencilOp == fBackPassOp)    &&
                        (kAlways_StencilFunc == fFrontFunc ||
                         kKeep_StencilOp == fFrontFailOp)  &&
                        (kAlways_StencilFunc == fBackFunc ||
                         kKeep_StencilOp == fBackFailOp));
        fFlags |= writes ? kDoesWrite_Flag : kDoesNotWrite_Flag;
        return writes;
    }
    
    void invalidate()  {
        
        fFrontPassOp = (GrStencilOp)(uint8_t)-1;
        fFlags = 0;
    }

    bool operator == (const GrStencilSettings& s) const {
        static const size_t gCompareSize = sizeof(GrStencilSettings) -
                                           sizeof(fFlags);
        GrAssert((const char*)&fFlags + sizeof(fFlags) == 
                 (const char*)this + sizeof(GrStencilSettings));
        if (this->isDisabled() & s.isDisabled()) { 
            return true;
        }
        return 0 == memcmp(this, &s, gCompareSize);
    }
    
    bool operator != (const GrStencilSettings& s) const {
        return !(*this == s);
    }
    
    GrStencilSettings& operator =(const GrStencilSettings& s) {
        memcpy(this, &s, sizeof(GrStencilSettings));
        return *this;
    }

private:
    friend class GrGpu;
    enum {
        kIsDisabled_Flag      = 0x1,
        kNotDisabled_Flag     = 0x2,
        kDoesWrite_Flag       = 0x4,
        kDoesNotWrite_Flag    = 0x8,
    };

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

GR_STATIC_ASSERT(sizeof(GrStencilSettingsStruct) == sizeof(GrStencilSettings));

#define GR_STATIC_CONST_STENCIL(NAME,                                        \
    FRONT_PASS_OP,    BACK_PASS_OP,                                          \
    FRONT_FAIL_OP,    BACK_FAIL_OP,                                          \
    FRONT_FUNC,       BACK_FUNC,                                             \
    FRONT_MASK,       BACK_MASK,                                             \
    FRONT_REF,        BACK_REF,                                              \
    FRONT_WRITE_MASK, BACK_WRITE_MASK)                                       \
    static const GrStencilSettingsStruct NAME ## _STRUCT = {                 \
        (FRONT_PASS_OP),    (BACK_PASS_OP),                                  \
        (FRONT_FAIL_OP),    (BACK_FAIL_OP),                                  \
        (FRONT_FUNC),       (BACK_FUNC),                                     \
        (0),                (0),                                             \
        (FRONT_MASK),       (BACK_MASK),                                     \
        (FRONT_REF),        (BACK_REF),                                      \
        (FRONT_WRITE_MASK), (BACK_WRITE_MASK),                               \
        0                                                                    \
    };                                                                       \
    static const GrStencilSettings& NAME =                                   \
        *reinterpret_cast<const GrStencilSettings*>(&(NAME ## _STRUCT))
#endif

#define GR_STATIC_CONST_SAME_STENCIL(NAME,                                   \
    PASS_OP, FAIL_OP, FUNC, MASK, REF, WRITE_MASK)                           \
    GR_STATIC_CONST_STENCIL(NAME, (PASS_OP), (PASS_OP), (FAIL_OP),           \
    (FAIL_OP), (FUNC), (FUNC), (MASK), (MASK), (REF), (REF), (WRITE_MASK),   \
    (WRITE_MASK))

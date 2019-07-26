








#ifndef GrStencil_DEFINED
#define GrStencil_DEFINED

#include "GrTypes.h"
#include "SkRegion.h"


























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

enum GrStencilFlags {
    kIsDisabled_StencilFlag      = 0x1,
    kNotDisabled_StencilFlag     = 0x2,
    kDoesWrite_StencilFlag       = 0x4,
    kDoesNotWrite_StencilFlag    = 0x8,
};










struct GrStencilSettingsStruct {
    uint8_t fPassOps[2];     
    uint8_t fFailOps[2];     
    uint8_t fFuncs[2];       
    uint8_t fPad0;
    uint8_t fPad1;
    uint16_t fFuncMasks[2];  
    uint16_t fFuncRefs[2];   
    uint16_t fWriteMasks[2]; 
    mutable uint32_t fFlags;
};

GR_STATIC_ASSERT(sizeof(GrStencilSettingsStruct) % 4 == 0);
GR_STATIC_ASSERT(sizeof(GrStencilSettingsStruct) ==
                 4*sizeof(uint8_t) + 
                 2*sizeof(uint8_t) + 
                 2*sizeof(uint8_t) + 
                 2*sizeof(uint16_t) + 
                 2*sizeof(uint16_t) + 
                 2*sizeof(uint16_t) + 
                 sizeof(uint32_t)); 





#define GR_STENCIL_SETTINGS_IS_DISABLED(                                     \
    FRONT_PASS_OP,    BACK_PASS_OP,                                          \
    FRONT_FAIL_OP,    BACK_FAIL_OP,                                          \
    FRONT_FUNC,       BACK_FUNC)                                             \
    ((FRONT_PASS_OP) == kKeep_StencilOp &&                                   \
     (BACK_PASS_OP)  == kKeep_StencilOp &&                                   \
     (FRONT_FAIL_OP) == kKeep_StencilOp &&                                   \
     (BACK_FAIL_OP)  == kKeep_StencilOp &&                                   \
     (FRONT_FUNC)    == kAlways_StencilFunc &&                               \
     (BACK_FUNC)     == kAlways_StencilFunc)

#define GR_STENCIL_SETTINGS_DOES_WRITE(                                      \
    FRONT_PASS_OP,    BACK_PASS_OP,                                          \
    FRONT_FAIL_OP,    BACK_FAIL_OP,                                          \
    FRONT_FUNC,       BACK_FUNC)                                             \
    (!(((FRONT_FUNC) == kNever_StencilFunc  ||                               \
        (FRONT_PASS_OP) == kKeep_StencilOp)  &&                              \
       ((BACK_FUNC) == kNever_StencilFunc  ||                                \
        (BACK_PASS_OP)  == kKeep_StencilOp) &&                               \
       ((FRONT_FUNC) == kAlways_StencilFunc ||                               \
        (FRONT_FAIL_OP) == kKeep_StencilOp) &&                               \
       ((BACK_FUNC)  == kAlways_StencilFunc ||                               \
        (BACK_FAIL_OP)  == kKeep_StencilOp)))

#define GR_STENCIL_SETTINGS_DEFAULT_FLAGS(                                   \
    FRONT_PASS_OP,    BACK_PASS_OP,                                          \
    FRONT_FAIL_OP,    BACK_FAIL_OP,                                          \
    FRONT_FUNC,       BACK_FUNC)                                             \
  ((GR_STENCIL_SETTINGS_IS_DISABLED(FRONT_PASS_OP,BACK_PASS_OP,              \
      FRONT_FAIL_OP,BACK_FAIL_OP,FRONT_FUNC,BACK_FUNC) ?                     \
      kIsDisabled_StencilFlag : kNotDisabled_StencilFlag) |                  \
   (GR_STENCIL_SETTINGS_DOES_WRITE(FRONT_PASS_OP,BACK_PASS_OP,               \
      FRONT_FAIL_OP,BACK_FAIL_OP,FRONT_FUNC,BACK_FUNC) ?                     \
      kDoesWrite_StencilFlag : kDoesNotWrite_StencilFlag))




class GrStencilSettings : private GrStencilSettingsStruct {

public:
    enum Face {
        kFront_Face = 0,
        kBack_Face  = 1,
    };

    GrStencilSettings() {
        fPad0 = fPad1 = 0;
        this->setDisabled();
    }

    GrStencilOp passOp(Face f) const { return static_cast<GrStencilOp>(fPassOps[f]); }
    GrStencilOp failOp(Face f) const { return static_cast<GrStencilOp>(fFailOps[f]); }
    GrStencilFunc func(Face f) const { return static_cast<GrStencilFunc>(fFuncs[f]); }
    uint16_t funcMask(Face f) const  { return fFuncMasks[f]; }
    uint16_t funcRef(Face f) const   { return fFuncRefs[f]; }
    uint16_t writeMask(Face f) const { return fWriteMasks[f]; }

    void setPassOp(Face f, GrStencilOp op) { fPassOps[f] = op; fFlags = 0;}
    void setFailOp(Face f, GrStencilOp op) { fFailOps[f] = op; fFlags = 0;}
    void setFunc(Face f, GrStencilFunc func) { fFuncs[f] = func; fFlags = 0;}
    void setFuncMask(Face f, unsigned short mask) { fFuncMasks[f] = mask; }
    void setFuncRef(Face f, unsigned short ref) { fFuncRefs[f] = ref; }
    void setWriteMask(Face f, unsigned short writeMask) { fWriteMasks[f] = writeMask; }

    void copyFrontSettingsToBack() {
        fPassOps[kBack_Face]    = fPassOps[kFront_Face];
        fFailOps[kBack_Face]    = fFailOps[kFront_Face];
        fFuncs[kBack_Face]      = fFuncs[kFront_Face];
        fFuncMasks[kBack_Face]  = fFuncMasks[kFront_Face];
        fFuncRefs[kBack_Face]   = fFuncRefs[kFront_Face];
        fWriteMasks[kBack_Face] = fWriteMasks[kFront_Face];
        fFlags = 0;
    }

    void setSame(GrStencilOp passOp,
                 GrStencilOp failOp,
                 GrStencilFunc func,
                 unsigned short funcMask,
                 unsigned short funcRef,
                 unsigned short writeMask) {
        fPassOps[kFront_Face]    = fPassOps[kBack_Face]    = passOp;
        fFailOps[kFront_Face]    = fFailOps[kBack_Face]    = failOp;
        fFuncs[kFront_Face]      = fFuncs[kBack_Face]      = func;
        fFuncMasks[kFront_Face]  = fFuncMasks[kBack_Face]  = funcMask;
        fFuncRefs[kFront_Face]   = fFuncRefs[kBack_Face]   = funcRef;
        fWriteMasks[kFront_Face] = fWriteMasks[kBack_Face] = writeMask;
        fFlags = 0;
    }

    void setDisabled() {
        memset(this, 0, sizeof(*this));
        GR_STATIC_ASSERT(0 == kKeep_StencilOp);
        GR_STATIC_ASSERT(0 == kAlways_StencilFunc);
        fFlags = kIsDisabled_StencilFlag | kDoesNotWrite_StencilFlag;
    }

    bool isTwoSided() const {
        return fPassOps[kFront_Face]    != fPassOps[kBack_Face]   ||
               fFailOps[kFront_Face]    != fFailOps[kBack_Face]   ||
               fFuncs[kFront_Face]      != fFuncs[kBack_Face]     ||
               fFuncMasks[kFront_Face]  != fFuncMasks[kBack_Face] ||
               fFuncRefs[kFront_Face]   != fFuncRefs[kBack_Face]  ||
               fWriteMasks[kFront_Face] != fWriteMasks[kBack_Face];
    }

    bool usesWrapOp() const {
        return kIncWrap_StencilOp == fPassOps[kFront_Face] ||
               kDecWrap_StencilOp == fPassOps[kFront_Face] ||
               kIncWrap_StencilOp == fPassOps[kBack_Face]  ||
               kDecWrap_StencilOp == fPassOps[kBack_Face]  ||
               kIncWrap_StencilOp == fFailOps[kFront_Face] ||
               kDecWrap_StencilOp == fFailOps[kFront_Face] ||
               kIncWrap_StencilOp == fFailOps[kBack_Face]  ||
               kDecWrap_StencilOp == fFailOps[kBack_Face];
    }

    bool isDisabled() const {
        if (fFlags & kIsDisabled_StencilFlag) {
            return true;
        }
        if (fFlags & kNotDisabled_StencilFlag) {
            return false;
        }
        bool disabled = GR_STENCIL_SETTINGS_IS_DISABLED(
                            fPassOps[kFront_Face], fPassOps[kBack_Face],
                            fFailOps[kFront_Face], fFailOps[kBack_Face],
                            fFuncs[kFront_Face],   fFuncs[kBack_Face]);
        fFlags |= disabled ? kIsDisabled_StencilFlag : kNotDisabled_StencilFlag;
        return disabled;
    }

    bool doesWrite() const {
        if (fFlags & kDoesWrite_StencilFlag) {
            return true;
        }
        if (fFlags & kDoesNotWrite_StencilFlag) {
            return false;
        }
        bool writes = GR_STENCIL_SETTINGS_DOES_WRITE(
                            fPassOps[kFront_Face], fPassOps[kBack_Face],
                            fFailOps[kFront_Face], fFailOps[kBack_Face],
                            fFuncs[kFront_Face],   fFuncs[kBack_Face]);
        fFlags |= writes ? kDoesWrite_StencilFlag : kDoesNotWrite_StencilFlag;
        return writes;
    }

    void invalidate()  {
        
        fPassOps[0] = (GrStencilOp)(uint8_t)-1;
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
    friend class GrClipMaskManager;

    enum {
        kMaxStencilClipPasses = 2  
                                   
    };

    






















    static bool GetClipPasses(SkRegion::Op op,
                              bool canBeDirect,
                              unsigned int stencilClipMask,
                              bool invertedFill,
                              int* numPasses,
                              GrStencilSettings settings[kMaxStencilClipPasses]);
};

GR_STATIC_ASSERT(sizeof(GrStencilSettingsStruct) == sizeof(GrStencilSettings));

#define GR_STATIC_CONST_STENCIL_STRUCT(STRUCT_NAME,                          \
    FRONT_PASS_OP,    BACK_PASS_OP,                                          \
    FRONT_FAIL_OP,    BACK_FAIL_OP,                                          \
    FRONT_FUNC,       BACK_FUNC,                                             \
    FRONT_MASK,       BACK_MASK,                                             \
    FRONT_REF,        BACK_REF,                                              \
    FRONT_WRITE_MASK, BACK_WRITE_MASK)                                       \
    static const GrStencilSettingsStruct STRUCT_NAME = {                     \
       {(FRONT_PASS_OP),    (BACK_PASS_OP)   },                              \
       {(FRONT_FAIL_OP),    (BACK_FAIL_OP)   },                              \
       {(FRONT_FUNC),       (BACK_FUNC)      },                              \
        (0),                (0),                                             \
       {(FRONT_MASK),       (BACK_MASK)      },                              \
       {(FRONT_REF),        (BACK_REF)       },                              \
       {(FRONT_WRITE_MASK), (BACK_WRITE_MASK)},                              \
        GR_STENCIL_SETTINGS_DEFAULT_FLAGS(                                   \
            FRONT_PASS_OP, BACK_PASS_OP, FRONT_FAIL_OP, BACK_FAIL_OP,        \
            FRONT_FUNC, BACK_FUNC)                                           \
    };

#define GR_CONST_STENCIL_SETTINGS_PTR_FROM_STRUCT_PTR(STRUCT_PTR)            \
    reinterpret_cast<const GrStencilSettings*>(STRUCT_PTR)

#define GR_STATIC_CONST_SAME_STENCIL_STRUCT(STRUCT_NAME,                     \
    PASS_OP, FAIL_OP, FUNC, MASK, REF, WRITE_MASK)                           \
    GR_STATIC_CONST_STENCIL_STRUCT(STRUCT_NAME, (PASS_OP), (PASS_OP),        \
    (FAIL_OP),(FAIL_OP), (FUNC), (FUNC), (MASK), (MASK), (REF), (REF),       \
    (WRITE_MASK),(WRITE_MASK))

#define GR_STATIC_CONST_STENCIL(NAME,                                        \
    FRONT_PASS_OP,    BACK_PASS_OP,                                          \
    FRONT_FAIL_OP,    BACK_FAIL_OP,                                          \
    FRONT_FUNC,       BACK_FUNC,                                             \
    FRONT_MASK,       BACK_MASK,                                             \
    FRONT_REF,        BACK_REF,                                              \
    FRONT_WRITE_MASK, BACK_WRITE_MASK)                                       \
    GR_STATIC_CONST_STENCIL_STRUCT(NAME ## _STRUCT,                          \
    (FRONT_PASS_OP),(BACK_PASS_OP),(FRONT_FAIL_OP),(BACK_FAIL_OP),           \
    (FRONT_FUNC),(BACK_FUNC),(FRONT_MASK),(BACK_MASK),                       \
    (FRONT_REF),(BACK_REF),(FRONT_WRITE_MASK),(BACK_WRITE_MASK))             \
    static const GrStencilSettings& NAME =                                   \
        *GR_CONST_STENCIL_SETTINGS_PTR_FROM_STRUCT_PTR(&(NAME ## _STRUCT));


#define GR_STATIC_CONST_SAME_STENCIL(NAME,                                   \
    PASS_OP, FAIL_OP, FUNC, MASK, REF, WRITE_MASK)                           \
    GR_STATIC_CONST_STENCIL(NAME, (PASS_OP), (PASS_OP), (FAIL_OP),           \
    (FAIL_OP), (FUNC), (FUNC), (MASK), (MASK), (REF), (REF), (WRITE_MASK),   \
    (WRITE_MASK))

#endif

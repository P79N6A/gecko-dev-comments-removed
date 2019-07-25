








#include "GrStencil.h"

const GrStencilSettings GrStencilSettings::gDisabled = {
    kKeep_StencilOp,     kKeep_StencilOp,
    kKeep_StencilOp,     kKeep_StencilOp,
    kAlways_StencilFunc, kAlways_StencilFunc,
    0x0,                 0x0,
    0x0,                 0x0,
    0x0,                 0x0
};
GR_STATIC_ASSERT(0 == kKeep_StencilOp);
GR_STATIC_ASSERT(0 == kAlways_StencilFunc);













static const GrStencilSettings gUserToClipReplace = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kLess_StencilFunc,   kLess_StencilFunc,
    0xffffffff,          0xffffffff,    
    0x0,                 0x0,           
    0xffffffff,          0xffffffff
};
static const GrStencilSettings gInvUserToClipReplace = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kEqual_StencilFunc,  kEqual_StencilFunc,
    0xffffffff,          0xffffffff,    
    0x0,                 0x0,           
    0xffffffff,          0xffffffff
};



static const GrStencilSettings gUserToClipIsect = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kLess_StencilFunc,   kLess_StencilFunc,
    0xffffffff,          0xffffffff,
    0x0,                 0x0,           
    0xffffffff,          0xffffffff
};
static const GrStencilSettings gInvUserToClipIsect = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kEqual_StencilFunc,  kEqual_StencilFunc,
    0xffffffff,          0xffffffff,
    0x0,                 0x0,           
    0xffffffff,          0xffffffff
};



static const GrStencilSettings gUserToClipDiff = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kEqual_StencilFunc,  kEqual_StencilFunc,
    0xffffffff,          0xffffffff,
    0x0,                 0x0,           
    0xffffffff,          0xffffffff
};
static const GrStencilSettings gInvUserToClipDiff = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kLess_StencilFunc,   kLess_StencilFunc,
    0xffffffff,          0xffffffff,
    0x0,                 0x0,           
    0xffffffff,          0xffffffff
};





static const GrStencilSettings gUserToClipUnionPass0 = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kKeep_StencilOp,     kKeep_StencilOp,
    kLEqual_StencilFunc, kLEqual_StencilFunc,
    0xffffffff,          0xffffffff,    
    0x00000001,          0x00000001,    
    0xffffffff,          0xffffffff
};


static const GrStencilSettings gUserToClipUnionPass1 = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kLEqual_StencilFunc, kLEqual_StencilFunc,
    0xffffffff,          0xffffffff,
    0x00000000,          0x00000000,    
    0xffffffff,          0xffffffff
};



static const GrStencilSettings gInvUserToClipUnionPass0 = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kKeep_StencilOp,     kKeep_StencilOp,
    kLess_StencilFunc,   kLess_StencilFunc,
    0xffffffff,          0xffffffff,
    0x00000000,          0x00000000,    
    0xffffffff,          0xffffffff
};



static const GrStencilSettings gInvUserToClipUnionPass1 = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kLess_StencilFunc,   kLess_StencilFunc,
    0xffffffff,          0xffffffff,    
    0x00000000,          0x00000000,    
    0xffffffff,          0xffffffff
};



static const GrStencilSettings gUserToClipXorPass0 = {
    kInvert_StencilOp,   kInvert_StencilOp,
    kKeep_StencilOp,     kKeep_StencilOp,
    kEqual_StencilFunc,  kEqual_StencilFunc,
    0xffffffff,          0xffffffff,    
    0x00000000,          0x00000000,
    0xffffffff,          0xffffffff
};

static const GrStencilSettings gUserToClipXorPass1 = {
    kReplace_StencilOp,   kReplace_StencilOp,
    kZero_StencilOp,      kZero_StencilOp,
    kGreater_StencilFunc, kGreater_StencilFunc,
    0xffffffff,           0xffffffff,
    0x00000000,           0x00000000,   
    0xffffffff,           0xffffffff
};

static const GrStencilSettings gInvUserToClipXorPass0 = {
    kInvert_StencilOp,   kInvert_StencilOp,
    kKeep_StencilOp,     kKeep_StencilOp,
    kEqual_StencilFunc,  kEqual_StencilFunc,
    0xffffffff,          0xffffffff,    
    0x00000000,          0x00000000,
    0xffffffff,          0xffffffff
};

static const GrStencilSettings gInvUserToClipXorPass1 = {
    kReplace_StencilOp,   kReplace_StencilOp,
    kZero_StencilOp,      kZero_StencilOp,
    kLess_StencilFunc,    kLess_StencilFunc,
    0xffffffff,           0xffffffff,
    0x00000000,           0x00000000,   
    0xffffffff,           0xffffffff
};



static const GrStencilSettings gUserToClipRDiffPass0 = {
    kInvert_StencilOp,   kInvert_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kLess_StencilFunc,   kLess_StencilFunc,
    0xffffffff,          0xffffffff,  
    0x00000000,          0x00000000,  
    0xffffffff,          0xffffffff
};

static const GrStencilSettings gUserToClipRDiffPass1 = {
    kReplace_StencilOp,   kReplace_StencilOp,
    kZero_StencilOp,      kZero_StencilOp,
    kEqual_StencilFunc,   kEqual_StencilFunc,
    0x00000000,           0x00000000,   
    0x00000000,           0x00000000,   
    0xffffffff,           0xffffffff
};

static const GrStencilSettings gInvUserToClipRDiff = {
    kInvert_StencilOp,    kInvert_StencilOp,
    kZero_StencilOp,      kZero_StencilOp,
    kEqual_StencilFunc,   kEqual_StencilFunc,
    0xffffffff,           0xffffffff,
    0x00000000,           0x00000000, 
    0x00000000,           0x00000000    
};









static const GrStencilSettings gReplaceClip = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kReplace_StencilOp,  kReplace_StencilOp,
    kAlways_StencilFunc, kAlways_StencilFunc,
    0xffffffff,          0xffffffff,
    0x00000000,          0x00000000,    
    0x00000000,          0x00000000     
};

static const GrStencilSettings gUnionClip = {
    kReplace_StencilOp,  kReplace_StencilOp,
    kReplace_StencilOp,  kReplace_StencilOp,
    kAlways_StencilFunc, kAlways_StencilFunc,
    0xffffffff,          0xffffffff,
    0x00000000,          0x00000000,    
    0x00000000,          0x00000000     
};

static const GrStencilSettings gXorClip = {
    kInvert_StencilOp,   kInvert_StencilOp,
    kInvert_StencilOp,   kInvert_StencilOp,
    kAlways_StencilFunc, kAlways_StencilFunc,
    0xffffffff,          0xffffffff,
    0x00000000,          0x00000000,
    0x00000000,          0x00000000     
};

static const GrStencilSettings gDiffClip = {
    kZero_StencilOp,     kZero_StencilOp,
    kZero_StencilOp,     kZero_StencilOp,
    kAlways_StencilFunc, kAlways_StencilFunc,
    0xffffffff,          0xffffffff,
    0x00000000,          0x00000000,
    0x00000000,          0x00000000     
};

bool GrStencilSettings::GetClipPasses(GrSetOp op, 
                                      bool canBeDirect,
                                      unsigned int stencilClipMask,
                                      bool invertedFill,
                                      int* numPasses,
                                      GrStencilSettings settings[kMaxStencilClipPasses]) {
    if (canBeDirect && !invertedFill) {
        *numPasses = 0;
        switch (op) {
            case kReplace_SetOp:
                *numPasses = 1;
                settings[0] = gReplaceClip;
                break;
            case kUnion_SetOp:
                *numPasses = 1;
                settings[0] = gUnionClip;
                break;
            case kXor_SetOp:
                *numPasses = 1;
                settings[0] = gXorClip;
                break;
            case kDifference_SetOp:
                *numPasses = 1;
                settings[0] = gDiffClip;
                break;
            default: 
                break;
        }
        if (1 == *numPasses) {
            settings[0].fFrontFuncRef |= stencilClipMask;
            settings[0].fFrontWriteMask |= stencilClipMask;
            settings[0].fBackFuncRef = settings[0].fFrontFuncRef;
            settings[0].fBackWriteMask = settings[0].fFrontWriteMask;
            return true;
        }
    }
    switch (op) {
        
        
        
        case kReplace_SetOp:
            *numPasses= 1;
            settings[0] = invertedFill ? gInvUserToClipReplace : gUserToClipReplace;
            settings[0].fFrontFuncMask &= ~stencilClipMask;
            settings[0].fFrontFuncRef |= stencilClipMask;
            settings[0].fBackFuncMask = settings[0].fFrontFuncMask;
            settings[0].fBackFuncRef = settings[0].fFrontFuncRef;
            break;
        case kIntersect_SetOp:
            *numPasses = 1;
            settings[0] = invertedFill ? gInvUserToClipIsect : gUserToClipIsect;
            settings[0].fFrontFuncRef = stencilClipMask;
            settings[0].fBackFuncRef = settings[0].fFrontFuncRef;
            break;
        case kUnion_SetOp:
            *numPasses = 2;
            if (invertedFill) {
                settings[0] = gInvUserToClipUnionPass0;
                settings[0].fFrontFuncRef |= stencilClipMask;
                settings[0].fBackFuncRef = settings[0].fFrontFuncMask;

                settings[1] = gInvUserToClipUnionPass1;
                settings[1].fFrontFuncMask &= ~stencilClipMask;
                settings[1].fFrontFuncRef |= stencilClipMask;
                settings[1].fBackFuncMask = settings[1].fFrontFuncMask;
                settings[1].fBackFuncRef = settings[1].fFrontFuncRef;

            } else {
                settings[0] = gUserToClipUnionPass0;
                settings[0].fFrontFuncMask &= ~stencilClipMask;
                settings[0].fFrontFuncRef |= stencilClipMask;
                settings[0].fBackFuncMask = settings[0].fFrontFuncMask;
                settings[0].fBackFuncRef = settings[0].fFrontFuncRef;

                settings[1] = gUserToClipUnionPass1;
                settings[1].fFrontFuncRef |= stencilClipMask;
                settings[1].fBackFuncRef = settings[1].fFrontFuncRef;
            }
            break;
        case kXor_SetOp:
            *numPasses = 2;
            if (invertedFill) {
                settings[0] = gInvUserToClipXorPass0;
                settings[0].fFrontFuncMask &= ~stencilClipMask;
                settings[0].fBackFuncMask = settings[0].fFrontFuncMask;

                settings[1] = gInvUserToClipXorPass1;
                settings[1].fFrontFuncRef |= stencilClipMask;
                settings[1].fBackFuncRef = settings[1].fFrontFuncRef;
            } else {
                settings[0] = gUserToClipXorPass0;
                settings[0].fFrontFuncMask &= ~stencilClipMask;
                settings[0].fBackFuncMask = settings[0].fFrontFuncMask;

                settings[1] = gUserToClipXorPass1;
                settings[1].fFrontFuncRef |= stencilClipMask;
                settings[1].fBackFuncRef = settings[1].fFrontFuncRef;
            }
            break;
        case kDifference_SetOp:
            *numPasses = 1;
            settings[0] = invertedFill ? gInvUserToClipDiff : gUserToClipDiff;
            settings[0].fFrontFuncRef |= stencilClipMask;
            settings[0].fBackFuncRef = settings[0].fFrontFuncRef;
            break;
        case kReverseDifference_SetOp:
            if (invertedFill) {
                *numPasses = 1;
                settings[0] = gInvUserToClipRDiff;
                settings[0].fFrontWriteMask |= stencilClipMask;
                settings[0].fBackWriteMask = settings[0].fFrontWriteMask;
            } else {
                *numPasses = 2;
                settings[0] = gUserToClipRDiffPass0;
                settings[0].fFrontFuncMask &= ~stencilClipMask;
                settings[0].fBackFuncMask = settings[0].fFrontFuncMask;
                settings[0].fFrontFuncRef |= stencilClipMask;
                settings[0].fBackFuncRef = settings[0].fFrontFuncRef;

                settings[1] = gUserToClipRDiffPass1;
                settings[1].fFrontFuncMask |= stencilClipMask;
                settings[1].fFrontFuncRef |= stencilClipMask;
                settings[1].fBackFuncMask = settings[1].fFrontFuncMask;
                settings[1].fBackFuncRef = settings[1].fFrontFuncRef;
            }
            break;
        default:
            GrCrash("Unknown set op");
    }
    return false;
}










#include "GrStencil.h"













GR_STATIC_CONST_SAME_STENCIL(gUserToClipReplace,
    kReplace_StencilOp,
    kZero_StencilOp,
    kLess_StencilFunc,
    0xffff,           
    0x0000,           
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipReplace,
    kReplace_StencilOp,
    kZero_StencilOp,
    kEqual_StencilFunc,
    0xffff,           
    0x0000,           
    0xffff);



GR_STATIC_CONST_SAME_STENCIL(gUserToClipIsect,
    kReplace_StencilOp,
    kZero_StencilOp,
    kLess_StencilFunc,
    0xffff,
    0x0000,           
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipIsect,
    kReplace_StencilOp,
    kZero_StencilOp,
    kEqual_StencilFunc,
    0xffff,
    0x0000,           
    0xffff);



GR_STATIC_CONST_SAME_STENCIL(gUserToClipDiff,
    kReplace_StencilOp,
    kZero_StencilOp,
    kEqual_StencilFunc,
    0xffff,
    0x0000,           
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipDiff,
    kReplace_StencilOp,
    kZero_StencilOp,
    kLess_StencilFunc,
    0xffff,
    0x0000,           
    0xffff);





GR_STATIC_CONST_SAME_STENCIL(gUserToClipUnionPass0,
    kReplace_StencilOp,
    kKeep_StencilOp,
    kLEqual_StencilFunc,
    0xffff,
    0x0001,           
    0xffff);


GR_STATIC_CONST_SAME_STENCIL(gUserToClipUnionPass1,
    kReplace_StencilOp,
    kZero_StencilOp,
    kLEqual_StencilFunc,
    0xffff,
    0x0000,           
    0xffff);



GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipUnionPass0,
    kReplace_StencilOp,
    kKeep_StencilOp,
    kEqual_StencilFunc,
    0xffff,
    0x0000,           
    0x0000            
);


GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipUnionPass1,
    kZero_StencilOp,
    kZero_StencilOp,
    kLess_StencilFunc,
    0xffff,
    0x0000,
    0xffff            
);



GR_STATIC_CONST_SAME_STENCIL(gUserToClipXorPass0,
    kInvert_StencilOp,
    kKeep_StencilOp,
    kEqual_StencilFunc,
    0xffff,           
    0x0000,
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gUserToClipXorPass1,
    kReplace_StencilOp,
    kZero_StencilOp,
    kGreater_StencilFunc,
    0xffff,
    0x0000,          
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipXorPass0,
    kInvert_StencilOp,
    kKeep_StencilOp,
    kEqual_StencilFunc,
    0xffff,           
    0x0000,
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipXorPass1,
    kReplace_StencilOp,
    kZero_StencilOp,
    kLess_StencilFunc,
    0xffff,
    0x0000,          
    0xffff);



GR_STATIC_CONST_SAME_STENCIL(gUserToClipRDiffPass0,
    kInvert_StencilOp,
    kZero_StencilOp,
    kLess_StencilFunc,
    0xffff,         
    0x0000,         
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gUserToClipRDiffPass1,
    kReplace_StencilOp,
    kZero_StencilOp,
    kEqual_StencilFunc,
    0x0000,          
    0x0000,          
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipRDiff,
    kInvert_StencilOp,
    kZero_StencilOp,
    kEqual_StencilFunc,
    0xffff,
    0x0000,
    0x0000           
);









GR_STATIC_CONST_SAME_STENCIL(gReplaceClip,
    kReplace_StencilOp,
    kReplace_StencilOp,
    kAlways_StencilFunc,
    0xffff,
    0x0000,           
    0x0000            
);

GR_STATIC_CONST_SAME_STENCIL(gUnionClip,
    kReplace_StencilOp,
    kReplace_StencilOp,
    kAlways_StencilFunc,
    0xffff,
    0x0000,           
    0x0000            
);

GR_STATIC_CONST_SAME_STENCIL(gXorClip,
    kInvert_StencilOp,
    kInvert_StencilOp,
    kAlways_StencilFunc,
    0xffff,
    0x0000,
    0x0000            
);

GR_STATIC_CONST_SAME_STENCIL(gDiffClip,
    kZero_StencilOp,
    kZero_StencilOp,
    kAlways_StencilFunc,
    0xffff,
    0x0000,
    0x0000            
);

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
                settings[0].fFrontFuncMask &= ~stencilClipMask;
                settings[0].fBackFuncMask = settings[0].fFrontFuncMask;
                settings[0].fFrontFuncRef |= stencilClipMask;
                settings[0].fBackFuncRef = settings[0].fFrontFuncRef;
                settings[0].fFrontWriteMask |= stencilClipMask;
                settings[0].fBackWriteMask = settings[0].fFrontWriteMask;

                settings[1] = gInvUserToClipUnionPass1;
                settings[1].fFrontWriteMask &= ~stencilClipMask;
                settings[1].fBackWriteMask &= settings[1].fFrontWriteMask;

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

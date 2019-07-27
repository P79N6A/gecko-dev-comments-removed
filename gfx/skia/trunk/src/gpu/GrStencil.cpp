








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



GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipRDiffPass0,
    kInvert_StencilOp,
    kZero_StencilOp,
    kEqual_StencilFunc,
    0xffff,
    0x0000,
    0x0000           
);

GR_STATIC_CONST_SAME_STENCIL(gInvUserToClipRDiffPass1,
    kZero_StencilOp,
    kZero_StencilOp,
    kAlways_StencilFunc,
    0xffff,
    0x0000,
    0xffff           
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

bool GrStencilSettings::GetClipPasses(
                            SkRegion::Op op,
                            bool canBeDirect,
                            unsigned int stencilClipMask,
                            bool invertedFill,
                            int* numPasses,
                            GrStencilSettings settings[kMaxStencilClipPasses]) {
    if (canBeDirect && !invertedFill) {
        *numPasses = 0;
        switch (op) {
            case SkRegion::kReplace_Op:
                *numPasses = 1;
                settings[0] = gReplaceClip;
                break;
            case SkRegion::kUnion_Op:
                *numPasses = 1;
                settings[0] = gUnionClip;
                break;
            case SkRegion::kXOR_Op:
                *numPasses = 1;
                settings[0] = gXorClip;
                break;
            case SkRegion::kDifference_Op:
                *numPasses = 1;
                settings[0] = gDiffClip;
                break;
            default: 
                break;
        }
        if (1 == *numPasses) {
            settings[0].fFuncRefs[kFront_Face]   |= stencilClipMask;
            settings[0].fWriteMasks[kFront_Face] |= stencilClipMask;
            settings[0].fFuncRefs[kBack_Face] =
                settings[0].fFuncRefs[kFront_Face];
            settings[0].fWriteMasks[kBack_Face] =
                settings[0].fWriteMasks[kFront_Face];
            return true;
        }
    }
    switch (op) {
        
        
        
        case SkRegion::kReplace_Op:
            *numPasses= 1;
            settings[0] = invertedFill ? gInvUserToClipReplace :
                                         gUserToClipReplace;
            settings[0].fFuncMasks[kFront_Face] &= ~stencilClipMask;
            settings[0].fFuncRefs[kFront_Face] |= stencilClipMask;
            settings[0].fFuncMasks[kBack_Face] =
                settings[0].fFuncMasks[kFront_Face];
            settings[0].fFuncRefs[kBack_Face] =
                settings[0].fFuncRefs[kFront_Face];
            break;
        case SkRegion::kIntersect_Op:
            *numPasses = 1;
            settings[0] = invertedFill ? gInvUserToClipIsect : gUserToClipIsect;
            settings[0].fFuncRefs[kFront_Face] = stencilClipMask;
            settings[0].fFuncRefs[kBack_Face] =
                settings[0].fFuncRefs[kFront_Face];
            break;
        case SkRegion::kUnion_Op:
            *numPasses = 2;
            if (invertedFill) {
                settings[0] = gInvUserToClipUnionPass0;
                settings[0].fFuncMasks[kFront_Face] &= ~stencilClipMask;
                settings[0].fFuncMasks[kBack_Face] =
                    settings[0].fFuncMasks[kFront_Face];
                settings[0].fFuncRefs[kFront_Face] |= stencilClipMask;
                settings[0].fFuncRefs[kBack_Face] =
                    settings[0].fFuncRefs[kFront_Face];
                settings[0].fWriteMasks[kFront_Face] |= stencilClipMask;
                settings[0].fWriteMasks[kBack_Face] =
                    settings[0].fWriteMasks[kFront_Face];

                settings[1] = gInvUserToClipUnionPass1;
                settings[1].fWriteMasks[kFront_Face] &= ~stencilClipMask;
                settings[1].fWriteMasks[kBack_Face] &=
                    settings[1].fWriteMasks[kFront_Face];

            } else {
                settings[0] = gUserToClipUnionPass0;
                settings[0].fFuncMasks[kFront_Face] &= ~stencilClipMask;
                settings[0].fFuncRefs[kFront_Face] |= stencilClipMask;
                settings[0].fFuncMasks[kBack_Face] =
                    settings[0].fFuncMasks[kFront_Face];
                settings[0].fFuncRefs[kBack_Face] =
                    settings[0].fFuncRefs[kFront_Face];

                settings[1] = gUserToClipUnionPass1;
                settings[1].fFuncRefs[kFront_Face] |= stencilClipMask;
                settings[1].fFuncRefs[kBack_Face] =
                    settings[1].fFuncRefs[kFront_Face];
            }
            break;
        case SkRegion::kXOR_Op:
            *numPasses = 2;
            if (invertedFill) {
                settings[0] = gInvUserToClipXorPass0;
                settings[0].fFuncMasks[kFront_Face] &= ~stencilClipMask;
                settings[0].fFuncMasks[kBack_Face] =
                    settings[0].fFuncMasks[kFront_Face];

                settings[1] = gInvUserToClipXorPass1;
                settings[1].fFuncRefs[kFront_Face] |= stencilClipMask;
                settings[1].fFuncRefs[kBack_Face] =
                    settings[1].fFuncRefs[kFront_Face];
            } else {
                settings[0] = gUserToClipXorPass0;
                settings[0].fFuncMasks[kFront_Face] &= ~stencilClipMask;
                settings[0].fFuncMasks[kBack_Face] =
                    settings[0].fFuncMasks[kFront_Face];

                settings[1] = gUserToClipXorPass1;
                settings[1].fFuncRefs[kFront_Face] |= stencilClipMask;
                settings[1].fFuncRefs[kBack_Face] =
                    settings[1].fFuncRefs[kFront_Face];
            }
            break;
        case SkRegion::kDifference_Op:
            *numPasses = 1;
            settings[0] = invertedFill ? gInvUserToClipDiff : gUserToClipDiff;
            settings[0].fFuncRefs[kFront_Face] |= stencilClipMask;
            settings[0].fFuncRefs[kBack_Face] =
                settings[0].fFuncRefs[kFront_Face];
            break;
        case SkRegion::kReverseDifference_Op:
            if (invertedFill) {
                *numPasses = 2;
                settings[0] = gInvUserToClipRDiffPass0;
                settings[0].fWriteMasks[kFront_Face] |= stencilClipMask;
                settings[0].fWriteMasks[kBack_Face] =
                    settings[0].fWriteMasks[kFront_Face];
                settings[1] = gInvUserToClipRDiffPass1;
                settings[1].fWriteMasks[kFront_Face] &= ~stencilClipMask;
                settings[1].fWriteMasks[kBack_Face] =
                    settings[1].fWriteMasks[kFront_Face];
            } else {
                *numPasses = 2;
                settings[0] = gUserToClipRDiffPass0;
                settings[0].fFuncMasks[kFront_Face] &= ~stencilClipMask;
                settings[0].fFuncMasks[kBack_Face] =
                    settings[0].fFuncMasks[kFront_Face];
                settings[0].fFuncRefs[kFront_Face] |= stencilClipMask;
                settings[0].fFuncRefs[kBack_Face] =
                    settings[0].fFuncRefs[kFront_Face];

                settings[1] = gUserToClipRDiffPass1;
                settings[1].fFuncMasks[kFront_Face] |= stencilClipMask;
                settings[1].fFuncRefs[kFront_Face] |= stencilClipMask;
                settings[1].fFuncMasks[kBack_Face] =
                    settings[1].fFuncMasks[kFront_Face];
                settings[1].fFuncRefs[kBack_Face] =
                    settings[1].fFuncRefs[kFront_Face];
            }
            break;
        default:
            SkFAIL("Unknown set op");
    }
    return false;
}

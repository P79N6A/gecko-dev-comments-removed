






#ifndef GrTypesPriv_DEFINED
#define GrTypesPriv_DEFINED

#include "GrTypes.h"
#include "SkTArray.h"





enum GrSLType {
    kVoid_GrSLType,
    kFloat_GrSLType,
    kVec2f_GrSLType,
    kVec3f_GrSLType,
    kVec4f_GrSLType,
    kMat33f_GrSLType,
    kMat44f_GrSLType,
    kSampler2D_GrSLType,

    kLast_GrSLType = kSampler2D_GrSLType
};
static const int kGrSLTypeCount = kLast_GrSLType + 1;




static inline int GrSLTypeVectorCount(GrSLType type) {
    SkASSERT(type >= 0 && type < static_cast<GrSLType>(kGrSLTypeCount));
    static const int kCounts[] = { -1, 1, 2, 3, 4, -1, -1, -1 };
    return kCounts[type];

    GR_STATIC_ASSERT(0 == kVoid_GrSLType);
    GR_STATIC_ASSERT(1 == kFloat_GrSLType);
    GR_STATIC_ASSERT(2 == kVec2f_GrSLType);
    GR_STATIC_ASSERT(3 == kVec3f_GrSLType);
    GR_STATIC_ASSERT(4 == kVec4f_GrSLType);
    GR_STATIC_ASSERT(5 == kMat33f_GrSLType);
    GR_STATIC_ASSERT(6 == kMat44f_GrSLType);
    GR_STATIC_ASSERT(7 == kSampler2D_GrSLType);
    GR_STATIC_ASSERT(SK_ARRAY_COUNT(kCounts) == kGrSLTypeCount);
}



static inline GrSLType GrSLFloatVectorType(int count) {
    SkASSERT(count > 0 && count <= 4);
    return (GrSLType)(count);

    GR_STATIC_ASSERT(kFloat_GrSLType == 1);
    GR_STATIC_ASSERT(kVec2f_GrSLType == 2);
    GR_STATIC_ASSERT(kVec3f_GrSLType == 3);
    GR_STATIC_ASSERT(kVec4f_GrSLType == 4);
}






enum GrVertexAttribType {
    kFloat_GrVertexAttribType = 0,
    kVec2f_GrVertexAttribType,
    kVec3f_GrVertexAttribType,
    kVec4f_GrVertexAttribType,
    kVec4ub_GrVertexAttribType,   

    kLast_GrVertexAttribType = kVec4ub_GrVertexAttribType
};
static const int kGrVertexAttribTypeCount = kLast_GrVertexAttribType + 1;




static inline int GrVertexAttribTypeVectorCount(GrVertexAttribType type) {
    SkASSERT(type >= 0 && type < kGrVertexAttribTypeCount);
    static const int kCounts[] = { 1, 2, 3, 4, 4 };
    return kCounts[type];

    GR_STATIC_ASSERT(0 == kFloat_GrVertexAttribType);
    GR_STATIC_ASSERT(1 == kVec2f_GrVertexAttribType);
    GR_STATIC_ASSERT(2 == kVec3f_GrVertexAttribType);
    GR_STATIC_ASSERT(3 == kVec4f_GrVertexAttribType);
    GR_STATIC_ASSERT(4 == kVec4ub_GrVertexAttribType);
    GR_STATIC_ASSERT(SK_ARRAY_COUNT(kCounts) == kGrVertexAttribTypeCount);
}




static inline size_t GrVertexAttribTypeSize(GrVertexAttribType type) {
    SkASSERT(type >= 0 && type < kGrVertexAttribTypeCount);
    static const size_t kSizes[] = {
        sizeof(float),          
        2*sizeof(float),        
        3*sizeof(float),        
        4*sizeof(float),        
        4*sizeof(char)          
    };
    return kSizes[type];

    GR_STATIC_ASSERT(0 == kFloat_GrVertexAttribType);
    GR_STATIC_ASSERT(1 == kVec2f_GrVertexAttribType);
    GR_STATIC_ASSERT(2 == kVec3f_GrVertexAttribType);
    GR_STATIC_ASSERT(3 == kVec4f_GrVertexAttribType);
    GR_STATIC_ASSERT(4 == kVec4ub_GrVertexAttribType);
    GR_STATIC_ASSERT(SK_ARRAY_COUNT(kSizes) == kGrVertexAttribTypeCount);
}






enum GrVertexAttribBinding {
    kPosition_GrVertexAttribBinding,    
    kLocalCoord_GrVertexAttribBinding,  
    kColor_GrVertexAttribBinding,       
    kCoverage_GrVertexAttribBinding,    

    kLastFixedFunction_GrVertexAttribBinding = kCoverage_GrVertexAttribBinding,

    kEffect_GrVertexAttribBinding,      
                                        
                                        
    kLast_GrVertexAttribBinding = kEffect_GrVertexAttribBinding
};

static const int kGrVertexAttribBindingCnt = kLast_GrVertexAttribBinding + 1;
static const int kGrFixedFunctionVertexAttribBindingCnt =
    kLastFixedFunction_GrVertexAttribBinding + 1;

static inline int GrFixedFunctionVertexAttribVectorCount(GrVertexAttribBinding binding) {
    SkASSERT(binding >= 0 && binding < kGrFixedFunctionVertexAttribBindingCnt);
    static const int kVecCounts[] = { 2, 2, 4, 4 };

    return kVecCounts[binding];

    GR_STATIC_ASSERT(0 == kPosition_GrVertexAttribBinding);
    GR_STATIC_ASSERT(1 == kLocalCoord_GrVertexAttribBinding);
    GR_STATIC_ASSERT(2 == kColor_GrVertexAttribBinding);
    GR_STATIC_ASSERT(3 == kCoverage_GrVertexAttribBinding);
    GR_STATIC_ASSERT(kGrFixedFunctionVertexAttribBindingCnt == SK_ARRAY_COUNT(kVecCounts));
}

struct GrVertexAttrib {
    inline void set(GrVertexAttribType type, size_t offset, GrVertexAttribBinding binding) {
        fType = type;
        fOffset = offset;
        fBinding = binding;
    }
    bool operator==(const GrVertexAttrib& other) const {
        return fType == other.fType && fOffset == other.fOffset && fBinding == other.fBinding;
    };
    bool operator!=(const GrVertexAttrib& other) const { return !(*this == other); }

    GrVertexAttribType      fType;
    size_t                  fOffset;
    GrVertexAttribBinding   fBinding;
};

template <int N> class GrVertexAttribArray : public SkSTArray<N, GrVertexAttrib, true> {};









enum GrEffectEdgeType {
    kFillBW_GrEffectEdgeType,
    kFillAA_GrEffectEdgeType,
    kInverseFillBW_GrEffectEdgeType,
    kInverseFillAA_GrEffectEdgeType,
    kHairlineAA_GrEffectEdgeType,

    kLast_GrEffectEdgeType = kHairlineAA_GrEffectEdgeType
};

static const int kGrEffectEdgeTypeCnt = kLast_GrEffectEdgeType + 1;

static inline bool GrEffectEdgeTypeIsFill(const GrEffectEdgeType edgeType) {
    return (kFillAA_GrEffectEdgeType == edgeType || kFillBW_GrEffectEdgeType == edgeType);
}

static inline bool GrEffectEdgeTypeIsInverseFill(const GrEffectEdgeType edgeType) {
    return (kInverseFillAA_GrEffectEdgeType == edgeType ||
            kInverseFillBW_GrEffectEdgeType == edgeType);
}

static inline bool GrEffectEdgeTypeIsAA(const GrEffectEdgeType edgeType) {
    return (kFillBW_GrEffectEdgeType != edgeType && kInverseFillBW_GrEffectEdgeType != edgeType);
}

static inline GrEffectEdgeType GrInvertEffectEdgeType(const GrEffectEdgeType edgeType) {
    switch (edgeType) {
        case kFillBW_GrEffectEdgeType:
            return kInverseFillBW_GrEffectEdgeType;
        case kFillAA_GrEffectEdgeType:
            return kInverseFillAA_GrEffectEdgeType;
        case kInverseFillBW_GrEffectEdgeType:
            return kFillBW_GrEffectEdgeType;
        case kInverseFillAA_GrEffectEdgeType:
            return kFillAA_GrEffectEdgeType;
        case kHairlineAA_GrEffectEdgeType:
            SkFAIL("Hairline fill isn't invertible.");
    }
    return kFillAA_GrEffectEdgeType; 
}

#endif

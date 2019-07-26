






#ifndef GrGLSL_DEFINED
#define GrGLSL_DEFINED

#include "gl/GrGLInterface.h"

class GrGLShaderVar;
class SkString;



enum GrGLSLGeneration {
    


    k110_GrGLSLGeneration,
    


    k130_GrGLSLGeneration,
    


    k150_GrGLSLGeneration,
};






enum GrSLType {
    kVoid_GrSLType,
    kFloat_GrSLType,
    kVec2f_GrSLType,
    kVec3f_GrSLType,
    kVec4f_GrSLType,
    kMat33f_GrSLType,
    kMat44f_GrSLType,
    kSampler2D_GrSLType
};

enum GrSLConstantVec {
    kZeros_GrSLConstantVec,
    kOnes_GrSLConstantVec,
    kNone_GrSLConstantVec,
};

namespace {
inline int GrSLTypeToVecLength(GrSLType type) {
    static const int kVecLengths[] = {
        0, 
        1, 
        2, 
        3, 
        4, 
        1, 
        1, 
        1, 
    };
    GrAssert((size_t) type < GR_ARRAY_COUNT(kVecLengths));
    return kVecLengths[type];
}

const char* GrGLSLOnesVecf(int count) {
    static const char* kONESVEC[] = {"ERROR", "1.0", "vec2(1,1)",
                                     "vec3(1,1,1)", "vec4(1,1,1,1)"};
    GrAssert(count >= 1 && count < (int)GR_ARRAY_COUNT(kONESVEC));
    return kONESVEC[count];
}

const char* GrGLSLZerosVecf(int count) {
    static const char* kZEROSVEC[] = {"ERROR", "0.0", "vec2(0,0)",
                                      "vec3(0,0,0)", "vec4(0,0,0,0)"};
    GrAssert(count >= 1 && count < (int)GR_ARRAY_COUNT(kZEROSVEC));
    return kZEROSVEC[count];
}
}




GrGLSLGeneration GrGetGLSLGeneration(GrGLBinding binding,
                                     const GrGLInterface* gl);





const char* GrGetGLSLVersionDecl(GrGLBinding binding,
                                 GrGLSLGeneration v);
















bool GrGLSLSetupFSColorOuput(GrGLSLGeneration gen,
                             const char* nameIfDeclared,
                             GrGLShaderVar* var);



GrSLType GrSLFloatVectorType(int count);



const char* GrGLSLVectorHomogCoord(int count);
const char* GrGLSLVectorHomogCoord(GrSLType type);



const char* GrGLSLVectorNonhomogCoords(int count);
const char* GrGLSLVectorNonhomogCoords(GrSLType type);










GrSLConstantVec GrGLSLModulate4f(SkString* outAppend,
                                 const char* in0,
                                 const char* in1,
                                 GrSLConstantVec default0 = kOnes_GrSLConstantVec,
                                 GrSLConstantVec default1 = kOnes_GrSLConstantVec);









GrSLConstantVec GrGLSLMulVarBy4f(SkString* outAppend,
                                 int tabCnt,
                                 const char* vec4VarName,
                                 const char* mulFactor,
                                 GrSLConstantVec mulFactorDefault = kOnes_GrSLConstantVec);










GrSLConstantVec GrGLSLAdd4f(SkString* outAppend,
                            const char* in0,
                            const char* in1,
                            GrSLConstantVec default0 = kZeros_GrSLConstantVec,
                            GrSLConstantVec default1 = kZeros_GrSLConstantVec);

#endif








#ifndef GrGLSL_DEFINED
#define GrGLSL_DEFINED

#include "gl/GrGLInterface.h"
#include "GrColor.h"
#include "GrTypesPriv.h"
#include "SkString.h"

class GrGLContextInfo;
class GrGLShaderVar;



enum GrGLSLGeneration {
    


    k110_GrGLSLGeneration,
    


    k130_GrGLSLGeneration,
    


    k140_GrGLSLGeneration,
    


    k150_GrGLSLGeneration,
};




bool GrGetGLSLGeneration(const GrGLInterface* gl, GrGLSLGeneration* generation);





const char* GrGetGLSLVersionDecl(const GrGLContextInfo&);




static inline const char* GrGLSLTypeString(GrSLType t) {
    switch (t) {
        case kVoid_GrSLType:
            return "void";
        case kFloat_GrSLType:
            return "float";
        case kVec2f_GrSLType:
            return "vec2";
        case kVec3f_GrSLType:
            return "vec3";
        case kVec4f_GrSLType:
            return "vec4";
        case kMat33f_GrSLType:
            return "mat3";
        case kMat44f_GrSLType:
            return "mat4";
        case kSampler2D_GrSLType:
            return "sampler2D";
        default:
            SkFAIL("Unknown shader var type.");
            return ""; 
    }
}








template <typename Self>
class GrGLSLExpr {
public:
    bool isOnes() const { return kOnes_ExprType == fType; }
    bool isZeros() const { return kZeros_ExprType == fType; }

    const char* c_str() const {
        if (kZeros_ExprType == fType) {
            return Self::ZerosStr();
        } else if (kOnes_ExprType == fType) {
            return Self::OnesStr();
        }
        SkASSERT(!fExpr.isEmpty()); 
        return fExpr.c_str();
    }

protected:
    


    GrGLSLExpr()
        : fType(kFullExpr_ExprType) {
        
        SkASSERT(!this->isValid());
    }

    
    explicit GrGLSLExpr(int v) {
        if (v == 0) {
            fType = kZeros_ExprType;
        } else if (v == 1) {
            fType = kOnes_ExprType;
        } else {
            fType = kFullExpr_ExprType;
            fExpr.appendf(Self::CastIntStr(), v);
        }
    }

    

    
    GrGLSLExpr(const char expr[]) {
        if (NULL == expr) {  
            fType = kOnes_ExprType;
        } else {
            fType = kFullExpr_ExprType;
            fExpr = expr;
        }
        SkASSERT(this->isValid());
    }

    

    
    GrGLSLExpr(const SkString& expr) {
        if (expr.isEmpty()) {  
            fType = kOnes_ExprType;
        } else {
            fType = kFullExpr_ExprType;
            fExpr = expr;
        }
        SkASSERT(this->isValid());
    }

    
    GrGLSLExpr(const char format[], const char in0[])
        : fType(kFullExpr_ExprType) {
        fExpr.appendf(format, in0);
    }

    
    GrGLSLExpr(const char format[], const char in0[], const char in1[])
        : fType(kFullExpr_ExprType) {
        fExpr.appendf(format, in0, in1);
    }

    bool isValid() const {
        return kFullExpr_ExprType != fType || !fExpr.isEmpty();
    }

    

    template <typename T>
    static Self VectorCastImpl(const T& other);

    







    template <typename T0, typename T1>
    static Self Mul(T0 in0, T1 in1);

    



    template <typename T0, typename T1>
    static Self Add(T0 in0, T1 in1);

    



    template <typename T0, typename T1>
    static Self Sub(T0 in0, T1 in1);

    




    template <typename T>
    T extractComponents(const char format[]) const;

private:
    enum ExprType {
        kZeros_ExprType,
        kOnes_ExprType,
        kFullExpr_ExprType,
    };
    ExprType fType;
    SkString fExpr;
};

class GrGLSLExpr1;
class GrGLSLExpr4;


class GrGLSLExpr1 : public GrGLSLExpr<GrGLSLExpr1> {
public:
    GrGLSLExpr1()
        : INHERITED() {
    }
    explicit GrGLSLExpr1(int v)
        : INHERITED(v) {
    }
    GrGLSLExpr1(const char* expr)
        : INHERITED(expr) {
    }
    GrGLSLExpr1(const SkString& expr)
        : INHERITED(expr) {
    }

    static GrGLSLExpr1 VectorCast(const GrGLSLExpr1& expr);

private:
    GrGLSLExpr1(const char format[], const char in0[])
        : INHERITED(format, in0) {
    }
    GrGLSLExpr1(const char format[], const char in0[], const char in1[])
        : INHERITED(format, in0, in1) {
    }

    static const char* ZerosStr();
    static const char* OnesStr();
    static const char* CastStr();
    static const char* CastIntStr();

    friend GrGLSLExpr1 operator*(const GrGLSLExpr1& in0, const GrGLSLExpr1&in1);
    friend GrGLSLExpr1 operator+(const GrGLSLExpr1& in0, const GrGLSLExpr1&in1);
    friend GrGLSLExpr1 operator-(const GrGLSLExpr1& in0, const GrGLSLExpr1&in1);

    friend class GrGLSLExpr<GrGLSLExpr1>;
    friend class GrGLSLExpr<GrGLSLExpr4>;

    typedef GrGLSLExpr<GrGLSLExpr1> INHERITED;
};


class GrGLSLExpr4 : public GrGLSLExpr<GrGLSLExpr4> {
public:
    GrGLSLExpr4()
        : INHERITED() {
    }
    explicit GrGLSLExpr4(int v)
        : INHERITED(v) {
    }
    GrGLSLExpr4(const char* expr)
        : INHERITED(expr) {
    }
    GrGLSLExpr4(const SkString& expr)
        : INHERITED(expr) {
    }

    typedef GrGLSLExpr1 AExpr;
    AExpr a() const;

    
    static GrGLSLExpr4 VectorCast(const GrGLSLExpr1& expr);
    static GrGLSLExpr4 VectorCast(const GrGLSLExpr4& expr);

private:
    GrGLSLExpr4(const char format[], const char in0[])
        : INHERITED(format, in0) {
    }
    GrGLSLExpr4(const char format[], const char in0[], const char in1[])
        : INHERITED(format, in0, in1) {
    }

    static const char* ZerosStr();
    static const char* OnesStr();
    static const char* CastStr();
    static const char* CastIntStr();

    
    friend GrGLSLExpr4 operator*(const GrGLSLExpr1& in0, const GrGLSLExpr4&in1);
    friend GrGLSLExpr4 operator+(const GrGLSLExpr1& in0, const GrGLSLExpr4&in1);
    friend GrGLSLExpr4 operator-(const GrGLSLExpr1& in0, const GrGLSLExpr4&in1);
    friend GrGLSLExpr4 operator*(const GrGLSLExpr4& in0, const GrGLSLExpr1&in1);
    friend GrGLSLExpr4 operator+(const GrGLSLExpr4& in0, const GrGLSLExpr1&in1);
    friend GrGLSLExpr4 operator-(const GrGLSLExpr4& in0, const GrGLSLExpr1&in1);

    
    friend GrGLSLExpr4 operator*(const GrGLSLExpr4& in0, const GrGLSLExpr4&in1);
    friend GrGLSLExpr4 operator+(const GrGLSLExpr4& in0, const GrGLSLExpr4&in1);
    friend GrGLSLExpr4 operator-(const GrGLSLExpr4& in0, const GrGLSLExpr4&in1);

    friend class GrGLSLExpr<GrGLSLExpr4>;

    typedef GrGLSLExpr<GrGLSLExpr4> INHERITED;
};





void GrGLSLMulVarBy4f(SkString* outAppend, unsigned tabCnt,
                      const char* vec4VarName, const GrGLSLExpr4& mulFactor);

#include "GrGLSL_impl.h"

#endif









#ifndef GrGLShaderVar_DEFINED
#define GrGLShaderVar_DEFINED

#include "GrGLInterface.h"
#include "GrGLSL.h"
#include "GrStringBuilder.h"

#define USE_UNIFORM_FLOAT_ARRAYS true




class GrGLShaderVar {
public:

    enum Type {
        kFloat_Type,
        kVec2f_Type,
        kVec3f_Type,
        kVec4f_Type,
        kMat33f_Type,
        kMat44f_Type,
        kSampler2D_Type,
    };

    




    enum TypeModifier {
        kNone_TypeModifier,
        kOut_TypeModifier,
        kIn_TypeModifier,
        kUniform_TypeModifier,
        kAttribute_TypeModifier
    };

    


    GrGLShaderVar() {
        fType = kFloat_Type;
        fTypeModifier = kNone_TypeModifier;
        fCount = kNonArray;
        fEmitPrecision = false;
        fUseUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS;
    }

    GrGLShaderVar(const GrGLShaderVar& var)
        : fType(var.fType)
        , fTypeModifier(var.fTypeModifier)
        , fName(var.fName)
        , fCount(var.fCount)
        , fEmitPrecision(var.fEmitPrecision)
        , fUseUniformFloatArrays(var.fUseUniformFloatArrays) {}

    


    enum {
        kNonArray     =  0, 
        kUnsizedArray = -1, 
    };

    


    void set(Type type,
             TypeModifier typeModifier,
             const GrStringBuilder& name,
             bool emitPrecision = false,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = kNonArray;
        fEmitPrecision = emitPrecision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(Type type,
             TypeModifier typeModifier,
             const char* name,
             bool specifyPrecision = false,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = kNonArray;
        fEmitPrecision = specifyPrecision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(Type type,
             TypeModifier typeModifier,
             const GrStringBuilder& name,
             int count,
             bool specifyPrecision = false,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = count;
        fEmitPrecision = specifyPrecision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(Type type,
             TypeModifier typeModifier,
             const char* name,
             int count,
             bool specifyPrecision = false,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = count;
        fEmitPrecision = specifyPrecision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    bool isArray() const { return kNonArray != fCount; }
    


    bool isUnsizedArray() const { return kUnsizedArray == fCount; }
    


    int getArrayCount() const { return fCount; }
    


    void setArrayCount(int count) { fCount = count; }
    


    void setNonArray() { fCount = kNonArray; }
    


    void setUnsizedArray() { fCount = kUnsizedArray; }

    


    GrStringBuilder* accessName() { return &fName; }
    


    void setName(const GrStringBuilder& n) { fName = n; }
    void setName(const char* n) { fName = n; }
    


    const GrStringBuilder& getName() const { return fName; }

    


    Type getType() const { return fType; }
    


    void setType(Type type) { fType = type; }

    TypeModifier getTypeModifier() const { return fTypeModifier; }
    void setTypeModifier(TypeModifier type) { fTypeModifier = type; }

    


    bool emitsPrecision() const { return fEmitPrecision; }
    


    void setEmitPrecision(bool p) { fEmitPrecision = p; }

    


    void appendDecl(const GrGLInterface* gl, GrStringBuilder* out,
                    GrGLSLGeneration gen) const {
        if (this->getTypeModifier() != kNone_TypeModifier) {
           out->append(TypeModifierString(this->getTypeModifier(), gen));
           out->append(" ");
        }
        if (this->emitsPrecision()) {
            out->append(PrecisionString(gl));
            out->append(" ");
        }
        Type effectiveType = this->getType();
        if (this->isArray()) {
            if (this->isUnsizedArray()) {
                out->appendf("%s %s[]", 
                             TypeString(effectiveType), 
                             this->getName().c_str());
            } else {
                GrAssert(this->getArrayCount() > 0);
                out->appendf("%s %s[%d]", 
                             TypeString(effectiveType),
                             this->getName().c_str(),
                             this->getArrayCount());
            }
        } else {
            out->appendf("%s %s",
                         TypeString(effectiveType),
                         this->getName().c_str());
        }
        out->append(";\n");
    }

    static const char* TypeString(Type t) {
        switch (t) {
            case kFloat_Type:
                return "float";
            case kVec2f_Type:
                return "vec2";
            case kVec3f_Type:
                return "vec3";
            case kVec4f_Type:
                return "vec4";
            case kMat33f_Type:
                return "mat3";
            case kMat44f_Type:
                return "mat4";
            case kSampler2D_Type:
                return "sampler2D";
            default:
                GrCrash("Unknown shader var type.");
                return ""; 
        }
    }

    void appendArrayAccess(int index, GrStringBuilder* out) {
        out->appendf("%s[%d]%s",
                     this->getName().c_str(),
                     index,
                     fUseUniformFloatArrays ? "" : ".x");
    }

    void appendArrayAccess(const char* indexName, GrStringBuilder* out) {
        out->appendf("%s[%s]%s",
                     this->getName().c_str(),
                     indexName,
                     fUseUniformFloatArrays ? "" : ".x");
    }

private:
    static const char* PrecisionString(const GrGLInterface* gl) {
        return gl->supportsDesktop() ? "" : "mediump";
    }

    static const char* TypeModifierString(TypeModifier t,
                                          GrGLSLGeneration gen) {
        switch (t) {
            case kNone_TypeModifier:
                return "";
            case kOut_TypeModifier:
                return k110_GLSLGeneration == gen ? "varying" : "out";
            case kIn_TypeModifier:
                return k110_GLSLGeneration == gen ? "varying" : "in";
            case kUniform_TypeModifier:
                return "uniform";
            case kAttribute_TypeModifier:
                return k110_GLSLGeneration == gen ? "attribute" : "in";
            default:
                GrCrash("Unknown shader variable type modifier.");
                return ""; 
        }
    }

    Type            fType;
    TypeModifier    fTypeModifier;
    GrStringBuilder fName;
    int             fCount;
    bool            fEmitPrecision;
    
    
    bool            fUseUniformFloatArrays;
};

#endif

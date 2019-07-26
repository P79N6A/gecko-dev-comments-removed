






#ifndef GrGLShaderVar_DEFINED
#define GrGLShaderVar_DEFINED

#include "GrGLContextInfo.h"
#include "GrGLSL.h"
#include "SkString.h"

#define USE_UNIFORM_FLOAT_ARRAYS true




class GrGLShaderVar {
public:

    




    enum TypeModifier {
        kNone_TypeModifier,
        kOut_TypeModifier,
        kIn_TypeModifier,
        kUniform_TypeModifier,
        kAttribute_TypeModifier
    };

    enum Precision {
        kLow_Precision,         
        kMedium_Precision,      
        kHigh_Precision,        
        kDefault_Precision,     
                                
                                
                                
                                
    };

    


    GrGLShaderVar() {
        fType = kFloat_GrSLType;
        fTypeModifier = kNone_TypeModifier;
        fCount = kNonArray;
        fPrecision = kDefault_Precision;
        fUseUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS;
    }

    GrGLShaderVar(const char* name, GrSLType type, int arrayCount = kNonArray) {
        GrAssert(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = kNone_TypeModifier;
        fCount = arrayCount;
        fPrecision = kDefault_Precision;
        fUseUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS;
        fName = name;
    }

    GrGLShaderVar(const GrGLShaderVar& var)
        : fType(var.fType)
        , fTypeModifier(var.fTypeModifier)
        , fName(var.fName)
        , fCount(var.fCount)
        , fPrecision(var.fPrecision)
        , fUseUniformFloatArrays(var.fUseUniformFloatArrays) {
        GrAssert(kVoid_GrSLType != var.fType);
    }

    


    enum {
        kNonArray     =  0, 
        kUnsizedArray = -1, 
    };

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const SkString& name,
             Precision precision = kDefault_Precision,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        GrAssert(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = kNonArray;
        fPrecision = precision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const char* name,
             Precision precision = kDefault_Precision,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        GrAssert(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = kNonArray;
        fPrecision = precision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const SkString& name,
             int count,
             Precision precision = kDefault_Precision,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        GrAssert(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = count;
        fPrecision = precision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const char* name,
             int count,
             Precision precision = kDefault_Precision,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        GrAssert(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = count;
        fPrecision = precision;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    bool isArray() const { return kNonArray != fCount; }
    


    bool isUnsizedArray() const { return kUnsizedArray == fCount; }
    


    int getArrayCount() const { return fCount; }
    


    void setArrayCount(int count) { fCount = count; }
    


    void setNonArray() { fCount = kNonArray; }
    


    void setUnsizedArray() { fCount = kUnsizedArray; }

    


    SkString* accessName() { return &fName; }
    


    void setName(const SkString& n) { fName = n; }
    void setName(const char* n) { fName = n; }

    


    const SkString& getName() const { return fName; }

    


    const char* c_str() const { return this->getName().c_str(); }

    


    GrSLType getType() const { return fType; }
    


    void setType(GrSLType type) { fType = type; }

    TypeModifier getTypeModifier() const { return fTypeModifier; }
    void setTypeModifier(TypeModifier type) { fTypeModifier = type; }

    


    Precision getPrecision() const { return fPrecision; }

    


    void setPrecision(Precision p) { fPrecision = p; }

    


    void appendDecl(const GrGLContextInfo& gl, SkString* out) const {
        if (this->getTypeModifier() != kNone_TypeModifier) {
           out->append(TypeModifierString(this->getTypeModifier(),
                                          gl.glslGeneration()));
           out->append(" ");
        }
        out->append(PrecisionString(fPrecision, gl.binding()));
        GrSLType effectiveType = this->getType();
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
    }

    static const char* TypeString(GrSLType t) {
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
                GrCrash("Unknown shader var type.");
                return ""; 
        }
    }

    void appendArrayAccess(int index, SkString* out) const {
        out->appendf("%s[%d]%s",
                     this->getName().c_str(),
                     index,
                     fUseUniformFloatArrays ? "" : ".x");
    }

    void appendArrayAccess(const char* indexName, SkString* out) const {
        out->appendf("%s[%s]%s",
                     this->getName().c_str(),
                     indexName,
                     fUseUniformFloatArrays ? "" : ".x");
    }

private:
    static const char* TypeModifierString(TypeModifier t, GrGLSLGeneration gen) {
        switch (t) {
            case kNone_TypeModifier:
                return "";
            case kOut_TypeModifier:
                return k110_GrGLSLGeneration == gen ? "varying" : "out";
            case kIn_TypeModifier:
                return k110_GrGLSLGeneration == gen ? "varying" : "in";
            case kUniform_TypeModifier:
                return "uniform";
            case kAttribute_TypeModifier:
                return k110_GrGLSLGeneration == gen ? "attribute" : "in";
            default:
                GrCrash("Unknown shader variable type modifier.");
                return ""; 
        }
    }

    static const char* PrecisionString(Precision p, GrGLBinding binding) {
        
        if (kES2_GrGLBinding == binding) {
            switch (p) {
                case kLow_Precision:
                    return "lowp ";
                case kMedium_Precision:
                    return "mediump ";
                case kHigh_Precision:
                    return "highp ";
                case kDefault_Precision:
                    return "";
                default:
                    GrCrash("Unexpected precision type.");
            }
        }
        return "";
    }

    GrSLType        fType;
    TypeModifier    fTypeModifier;
    SkString        fName;
    int             fCount;
    Precision       fPrecision;
    
    
    bool            fUseUniformFloatArrays;
};

#endif








#ifndef GrGLShaderVar_DEFINED
#define GrGLShaderVar_DEFINED

#include "GrGLContext.h"
#include "GrGLSL.h"
#include "SkString.h"

#define USE_UNIFORM_FLOAT_ARRAYS true




class GrGLShaderVar {
public:

    




    enum TypeModifier {
        kNone_TypeModifier,
        kOut_TypeModifier,
        kIn_TypeModifier,
        kInOut_TypeModifier,
        kUniform_TypeModifier,
        kAttribute_TypeModifier,
        kVaryingIn_TypeModifier,
        kVaryingOut_TypeModifier
    };

    enum Precision {
        kLow_Precision,         
        kMedium_Precision,      
        kHigh_Precision,        
        kDefault_Precision,     
                                
                                
                                
                                
    };

    


    enum Origin {
        kDefault_Origin,        
        kUpperLeft_Origin,      
    };

    


    GrGLShaderVar() {
        fType = kFloat_GrSLType;
        fTypeModifier = kNone_TypeModifier;
        fCount = kNonArray;
        fPrecision = kDefault_Precision;
        fOrigin = kDefault_Origin;
        fUseUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS;
    }

    GrGLShaderVar(const char* name, GrSLType type, int arrayCount = kNonArray,
                  Precision precision = kDefault_Precision) {
        SkASSERT(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = kNone_TypeModifier;
        fCount = arrayCount;
        fPrecision = precision;
        fOrigin = kDefault_Origin;
        fUseUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS;
        fName = name;
    }

    GrGLShaderVar(const GrGLShaderVar& var)
        : fType(var.fType)
        , fTypeModifier(var.fTypeModifier)
        , fName(var.fName)
        , fCount(var.fCount)
        , fPrecision(var.fPrecision)
        , fOrigin(var.fOrigin)
        , fUseUniformFloatArrays(var.fUseUniformFloatArrays) {
        SkASSERT(kVoid_GrSLType != var.fType);
    }

    


    enum {
        kNonArray     =  0, 
        kUnsizedArray = -1, 
    };

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const SkString& name,
             Precision precision = kDefault_Precision,
             Origin origin = kDefault_Origin,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        SkASSERT(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = kNonArray;
        fPrecision = precision;
        fOrigin = origin;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const char* name,
             Precision precision = kDefault_Precision,
             Origin origin = kDefault_Origin,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        SkASSERT(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = kNonArray;
        fPrecision = precision;
        fOrigin = origin;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const SkString& name,
             int count,
             Precision precision = kDefault_Precision,
             Origin origin = kDefault_Origin,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        SkASSERT(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = count;
        fPrecision = precision;
        fOrigin = origin;
        fUseUniformFloatArrays = useUniformFloatArrays;
    }

    


    void set(GrSLType type,
             TypeModifier typeModifier,
             const char* name,
             int count,
             Precision precision = kDefault_Precision,
             Origin origin = kDefault_Origin,
             bool useUniformFloatArrays = USE_UNIFORM_FLOAT_ARRAYS) {
        SkASSERT(kVoid_GrSLType != type);
        fType = type;
        fTypeModifier = typeModifier;
        fName = name;
        fCount = count;
        fPrecision = precision;
        fOrigin = origin;
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

    


    Origin getOrigin() const { return fOrigin; }

    


    void setOrigin(Origin origin) { fOrigin = origin; }

    


    void appendDecl(const GrGLContextInfo& ctxInfo, SkString* out) const {
        if (kUpperLeft_Origin == fOrigin) {
            
            
            out->append("layout(origin_upper_left) ");
        }
        if (this->getTypeModifier() != kNone_TypeModifier) {
           out->append(TypeModifierString(this->getTypeModifier(),
                                          ctxInfo.glslGeneration()));
           out->append(" ");
        }
        out->append(PrecisionString(fPrecision, ctxInfo.standard()));
        GrSLType effectiveType = this->getType();
        if (this->isArray()) {
            if (this->isUnsizedArray()) {
                out->appendf("%s %s[]",
                             GrGLSLTypeString(effectiveType),
                             this->getName().c_str());
            } else {
                SkASSERT(this->getArrayCount() > 0);
                out->appendf("%s %s[%d]",
                             GrGLSLTypeString(effectiveType),
                             this->getName().c_str(),
                             this->getArrayCount());
            }
        } else {
            out->appendf("%s %s",
                         GrGLSLTypeString(effectiveType),
                         this->getName().c_str());
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

    static const char* PrecisionString(Precision p, GrGLStandard standard) {
        
        if (kGLES_GrGLStandard == standard) {
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
                    SkFAIL("Unexpected precision type.");
            }
        }
        return "";
    }

private:
    static const char* TypeModifierString(TypeModifier t, GrGLSLGeneration gen) {
        switch (t) {
            case kNone_TypeModifier:
                return "";
            case kIn_TypeModifier:
                return "in";
            case kInOut_TypeModifier:
                return "inout";
            case kOut_TypeModifier:
                return "out";
            case kUniform_TypeModifier:
                return "uniform";
            case kAttribute_TypeModifier:
                return k110_GrGLSLGeneration == gen ? "attribute" : "in";
            case kVaryingIn_TypeModifier:
                return k110_GrGLSLGeneration == gen ? "varying" : "in";
            case kVaryingOut_TypeModifier:
                return k110_GrGLSLGeneration == gen ? "varying" : "out";
            default:
                SkFAIL("Unknown shader variable type modifier.");
                return ""; 
        }
    }

    GrSLType        fType;
    TypeModifier    fTypeModifier;
    SkString        fName;
    int             fCount;
    Precision       fPrecision;
    Origin          fOrigin;
    
    
    bool            fUseUniformFloatArrays;
};

#endif

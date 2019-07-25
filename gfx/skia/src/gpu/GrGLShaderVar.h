







#ifndef GrGLShaderVar_DEFINED
#define GrGLShaderVar_DEFINED

#include "GrGLInterface.h"
#include "GrStringBuilder.h"




class GrGLShaderVar {
public:

    enum Type {
        kFloat_Type,
        kVec2f_Type,
        kVec3f_Type,
        kVec4f_Type,
        kMat33f_Type,
        kSampler2D_Type,
    };

    


    GrGLShaderVar() {
        fType = kFloat_Type;
        fCount = kNonArray;
        fEmitPrecision = false;
    }

    GrGLShaderVar(const GrGLShaderVar& var)
        : fType(var.fType)
        , fName(var.fName)
        , fCount(var.fCount)
        , fEmitPrecision(var.fEmitPrecision) {}

    


    enum {
        kNonArray     =  0, 
        kUnsizedArray = -1, 
    };

    


    void set(Type type,
             const GrStringBuilder& name,
             bool emitPrecision = false) {
        fType = type;
        fName = name;
        fCount = kNonArray;
        fEmitPrecision = emitPrecision;
    }

    


    void set(Type type,
             const char* name,
             bool specifyPrecision = false) {
        fType = type;
        fName = name;
        fCount = kNonArray;
        fEmitPrecision = specifyPrecision;
    }

    


    void set(Type type,
             const GrStringBuilder& name,
             int count,
             bool specifyPrecision = false) {
        fType = type;
        fName = name;
        fCount = count;
        fEmitPrecision = specifyPrecision;
    }

    


    void set(Type type,
             const char* name,
             int count,
             bool specifyPrecision = false) {
        fType = type;
        fName = name;
        fCount = count;
        fEmitPrecision = specifyPrecision;
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

    


    bool emitsPrecision() const { return fEmitPrecision; }
    


    void setEmitPrecision(bool p) { fEmitPrecision = p; }

    


    void appendDecl(const GrGLInterface* gl, GrStringBuilder* out) const {
        if (this->emitsPrecision()) {
            out->append(PrecisionString(gl));
            out->append(" ");
        }
        if (this->isArray()) {
            if (this->isUnsizedArray()) {
                out->appendf("%s %s[]", 
                             TypeString(this->getType()), 
                             this->getName().c_str());
            } else {
                GrAssert(this->getArrayCount() > 0);
                out->appendf("%s %s[%d]", 
                             TypeString(this->getType()),
                             this->getName().c_str(),
                             this->getArrayCount());
            }
        } else {
            out->appendf("%s %s",
                         TypeString(this->getType()),
                         this->getName().c_str());
        }
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
            case kSampler2D_Type:
                return "sampler2D";
            default:
                GrCrash("Unknown shader var type.");
                return ""; 
        }
    }

private:
    static const char* PrecisionString(const GrGLInterface* gl) {
        return gl->supportsDesktop() ? "" : "mediump";
    }

    Type            fType;
    GrStringBuilder fName;
    int             fCount;
    bool            fEmitPrecision;
};

#endif

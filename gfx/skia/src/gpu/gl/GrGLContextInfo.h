







#ifndef GrGLContextInfo_DEFINED
#define GrGLContextInfo_DEFINED

#include "gl/GrGLInterface.h"
#include "GrGLCaps.h"
#include "GrGLSL.h"
#include "GrGLUtil.h"

#include "SkString.h"






class GrGLContextInfo {
public:

    


    GrGLContextInfo();

    



    explicit GrGLContextInfo(const GrGLInterface* interface);

    


    GrGLContextInfo(const GrGLContextInfo& ctx);

    ~GrGLContextInfo();

    


    GrGLContextInfo& operator = (const GrGLContextInfo& ctx);

    



    bool initialize(const GrGLInterface* interface);
    bool isInitialized() const;

    const GrGLInterface* interface() const { return fInterface; }
    GrGLBinding binding() const { return fBindingInUse; }
    GrGLVersion version() const { return fGLVersion; }
    GrGLSLGeneration glslGeneration() const { return fGLSLGeneration; }
    const GrGLCaps& caps() const { return fGLCaps; }
    GrGLCaps& caps() { return fGLCaps; }

    



    bool hasExtension(const char* ext) const {
        if (!this->isInitialized()) {
            return false;
        }
        return GrGLHasExtensionFromString(ext, fExtensionString.c_str());
    }

private:
    void reset();

    const GrGLInterface* fInterface;
    GrGLBinding          fBindingInUse;
    GrGLVersion          fGLVersion;
    GrGLSLGeneration     fGLSLGeneration;
    SkString             fExtensionString;
    GrGLCaps             fGLCaps;
};

#endif

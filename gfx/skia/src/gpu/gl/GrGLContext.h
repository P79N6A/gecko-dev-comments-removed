







#ifndef GrGLContext_DEFINED
#define GrGLContext_DEFINED

#include "gl/GrGLExtensions.h"
#include "gl/GrGLInterface.h"
#include "GrGLCaps.h"
#include "GrGLSL.h"
#include "GrGLUtil.h"

#include "SkString.h"





class GrGLContextInfo {
public:
    


    GrGLContextInfo() {
        fGLCaps.reset(SkNEW(GrGLCaps));
        this->reset();
    }

    


    GrGLContextInfo& operator= (const GrGLContextInfo& ctxInfo);

    



    bool initialize(const GrGLInterface* interface);
    bool isInitialized() const;

    GrGLBinding binding() const { return fBindingInUse; }
    GrGLVersion version() const { return fGLVersion; }
    GrGLSLGeneration glslGeneration() const { return fGLSLGeneration; }
    GrGLVendor vendor() const { return fVendor; }
    const GrGLCaps* caps() const { return fGLCaps.get(); }
    GrGLCaps* caps() { return fGLCaps; }

    



    bool hasExtension(const char* ext) const {
        if (!this->isInitialized()) {
            return false;
        }
        return fExtensions.has(ext);
    }

    


    void reset();

private:

    GrGLBinding             fBindingInUse;
    GrGLVersion             fGLVersion;
    GrGLSLGeneration        fGLSLGeneration;
    GrGLVendor              fVendor;
    GrGLExtensions          fExtensions;
    SkAutoTUnref<GrGLCaps>  fGLCaps;
};





class GrGLContext {
public:
    


    GrGLContext() { this->reset(); }

    



    explicit GrGLContext(const GrGLInterface* interface);

    


    GrGLContext(const GrGLContext& ctx);

    ~GrGLContext() { GrSafeUnref(fInterface); }

    


    GrGLContext& operator= (const GrGLContext& ctx);

    



    bool initialize(const GrGLInterface* interface);
    bool isInitialized() const { return fInfo.isInitialized(); }

    const GrGLInterface* interface() const { return fInterface; }
    const GrGLContextInfo& info() const { return fInfo; }
    GrGLContextInfo& info() { return fInfo; }

private:
    void reset();

    const GrGLInterface* fInterface;
    GrGLContextInfo      fInfo;
};

#endif

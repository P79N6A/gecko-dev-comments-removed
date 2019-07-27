







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

    GrGLContextInfo(const GrGLContextInfo& that) {
        fGLCaps.reset(SkNEW(GrGLCaps));
        *this = that;
    }

    GrGLContextInfo& operator= (const GrGLContextInfo&);

    



    bool initialize(const GrGLInterface* interface);
    bool isInitialized() const;

    GrGLStandard standard() const { return fInterface->fStandard; }
    GrGLVersion version() const { return fGLVersion; }
    GrGLSLGeneration glslGeneration() const { return fGLSLGeneration; }
    GrGLVendor vendor() const { return fVendor; }
    GrGLRenderer renderer() const { return fRenderer; }
    
    bool isMesa() const { return fIsMesa; }
    


    bool isChromium() const { return fIsChromium; }
    const GrGLCaps* caps() const { return fGLCaps.get(); }
    GrGLCaps* caps() { return fGLCaps; }
    bool hasExtension(const char* ext) const {
        if (!this->isInitialized()) {
            return false;
        }
        return fInterface->hasExtension(ext);
    }

    const GrGLExtensions& extensions() const { return fInterface->fExtensions; }

    


    void reset();

protected:
    SkAutoTUnref<const GrGLInterface>   fInterface;
    GrGLVersion                         fGLVersion;
    GrGLSLGeneration                    fGLSLGeneration;
    GrGLVendor                          fVendor;
    GrGLRenderer                        fRenderer;
    bool                                fIsMesa;
    bool                                fIsChromium;
    SkAutoTUnref<GrGLCaps>              fGLCaps;
};




class GrGLContext : public GrGLContextInfo {
public:
    



    explicit GrGLContext(const GrGLInterface* interface) {
        this->initialize(interface);
    }

    GrGLContext(const GrGLContext& that) : INHERITED(that) {}

    GrGLContext& operator= (const GrGLContext& that) {
        this->INHERITED::operator=(that);
        return *this;
    }

    const GrGLInterface* interface() const { return fInterface.get(); }

private:
    typedef GrGLContextInfo INHERITED;
};

#endif

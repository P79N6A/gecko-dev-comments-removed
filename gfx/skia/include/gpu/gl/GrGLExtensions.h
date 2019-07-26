






#ifndef GrGLExtensions_DEFINED
#define GrGLExtensions_DEFINED

#include "GrGLInterface.h"
#include "SkString.h"
#include "SkTArray.h"






class GrGLExtensions {
public:
    bool init(GrGLBinding binding, const GrGLInterface* iface) {
        GrAssert(binding & iface->fBindingsExported);
        return this->init(binding, iface->fGetString, iface->fGetStringi, iface->fGetIntegerv);
    }
    




    bool init(GrGLBinding binding,
              GrGLGetStringProc getString,
              GrGLGetStringiProc getStringi,
              GrGLGetIntegervProc getIntegerv);

    


    bool has(const char*) const;

    void reset() { fStrings.reset(); }

private:
    SkTArray<SkString> fStrings;
};

#endif

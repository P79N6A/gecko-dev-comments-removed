






#ifndef GrGLExtensions_DEFINED
#define GrGLExtensions_DEFINED

#include "GrGLFunctions.h"
#include "SkString.h"
#include "SkTArray.h"

struct GrGLInterface;






class SK_API GrGLExtensions {
public:
    GrGLExtensions() : fInitialized(false), fStrings(SkNEW(SkTArray<SkString>)) {}

    GrGLExtensions(const GrGLExtensions&);

    GrGLExtensions& operator=(const GrGLExtensions&);

    void swap(GrGLExtensions* that) {
        fStrings.swap(&that->fStrings);
        SkTSwap(fInitialized, that->fInitialized);
    }

    




    bool init(GrGLStandard standard,
              GrGLGetStringProc getString,
              GrGLGetStringiProc getStringi,
              GrGLGetIntegervProc getIntegerv);

    bool isInitialized() const { return fInitialized; }

    


    bool has(const char[]) const;

    


    bool remove(const char[]);

    


    void add(const char[]);

    void reset() { fStrings->reset(); }

    void print(const char* sep = "\n") const;

private:
    bool                                fInitialized;
    SkAutoTDelete<SkTArray<SkString> >  fStrings;
};

#endif

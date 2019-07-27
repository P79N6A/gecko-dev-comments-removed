







#ifndef GrGLPathRange_DEFINED
#define GrGLPathRange_DEFINED

#include "../GrPathRange.h"
#include "gl/GrGLFunctions.h"

class GrGpuGL;







class GrGLPathRange : public GrPathRange {
public:
    GrGLPathRange(GrGpu*, size_t size, const SkStrokeRec&);
    virtual ~GrGLPathRange();

    GrGLuint basePathID() const { return fBasePathID; }

    virtual void initAt(size_t index, const SkPath&);

    
    virtual size_t gpuMemorySize() const SK_OVERRIDE {
        return 100 * fNumDefinedPaths;
    }

protected:
    virtual void onRelease() SK_OVERRIDE;
    virtual void onAbandon() SK_OVERRIDE;

private:
    GrGLuint fBasePathID;
    size_t fNumDefinedPaths;

    typedef GrPathRange INHERITED;
};

#endif

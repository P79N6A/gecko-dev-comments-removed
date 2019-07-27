







#ifndef GrGLPath_DEFINED
#define GrGLPath_DEFINED

#include "../GrPath.h"
#include "gl/GrGLFunctions.h"

class GrGpuGL;
struct GrGLInterface;







class GrGLPath : public GrPath {
public:
    static void InitPathObject(const GrGLInterface*,
                               GrGLuint pathID,
                               const SkPath&,
                               const SkStrokeRec&);

    GrGLPath(GrGpuGL* gpu, const SkPath& path, const SkStrokeRec& stroke);
    virtual ~GrGLPath();
    GrGLuint pathID() const { return fPathID; }
    
    
    virtual size_t gpuMemorySize() const SK_OVERRIDE { return 100; }

protected:
    virtual void onRelease() SK_OVERRIDE;
    virtual void onAbandon() SK_OVERRIDE;

private:
    GrGLuint fPathID;

    typedef GrPath INHERITED;
};

#endif

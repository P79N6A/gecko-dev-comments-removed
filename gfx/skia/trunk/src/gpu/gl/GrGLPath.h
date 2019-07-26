







#ifndef GrGLPath_DEFINED
#define GrGLPath_DEFINED

#include "../GrPath.h"
#include "gl/GrGLFunctions.h"

class GrGpuGL;







class GrGLPath : public GrPath {
public:
    GrGLPath(GrGpuGL* gpu, const SkPath& path, const SkStrokeRec& stroke);
    virtual ~GrGLPath();
    GrGLuint pathID() const { return fPathID; }
    
    
    virtual size_t sizeInBytes() const SK_OVERRIDE { return 100; }

protected:
    virtual void onRelease() SK_OVERRIDE;
    virtual void onAbandon() SK_OVERRIDE;

private:
    GrGLuint fPathID;

    typedef GrPath INHERITED;
};

#endif

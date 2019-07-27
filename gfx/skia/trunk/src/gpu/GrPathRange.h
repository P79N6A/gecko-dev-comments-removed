






#ifndef GrPathRange_DEFINED
#define GrPathRange_DEFINED

#include "GrGpuResource.h"
#include "GrResourceCache.h"
#include "SkStrokeRec.h"

class SkPath;







class GrPathRange : public GrGpuResource {
public:
    SK_DECLARE_INST_COUNT(GrPathRange);

    static const bool kIsWrapped = false;

    static GrResourceKey::ResourceType resourceType() {
        static const GrResourceKey::ResourceType type = GrResourceKey::GenerateResourceType();
        return type;
    }

    


    GrPathRange(GrGpu* gpu, size_t size, const SkStrokeRec& stroke)
        : INHERITED(gpu, kIsWrapped),
          fSize(size),
          fStroke(stroke) {
    }

    size_t getSize() const { return fSize; }
    const SkStrokeRec& getStroke() const { return fStroke; }

    



    virtual void initAt(size_t index, const SkPath&) = 0;

protected:
    size_t fSize;
    SkStrokeRec fStroke;

private:
    typedef GrGpuResource INHERITED;
};

#endif

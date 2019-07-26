







#ifndef GrAARectRenderer_DEFINED
#define GrAARectRenderer_DEFINED

#include "GrRect.h"
#include "GrRefCnt.h"

class GrGpu;
class GrDrawTarget;
class GrIndexBuffer;




class GrAARectRenderer : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrAARectRenderer)

    GrAARectRenderer()
    : fAAFillRectIndexBuffer(NULL)
    , fAAStrokeRectIndexBuffer(NULL) {
    }

    void reset();

    ~GrAARectRenderer() {
        this->reset();
    }

    
    

    
    
    void fillAARect(GrGpu* gpu,
                    GrDrawTarget* target,
                    const GrRect& devRect,
                    bool useVertexCoverage);

    void strokeAARect(GrGpu* gpu,
                      GrDrawTarget* target,
                      const GrRect& devRect,
                      const GrVec& devStrokeSize,
                      bool useVertexCoverage);

private:
    GrIndexBuffer*              fAAFillRectIndexBuffer;
    GrIndexBuffer*              fAAStrokeRectIndexBuffer;

    static const uint16_t       gFillAARectIdx[];
    static const uint16_t       gStrokeAARectIdx[];

    static int aaFillRectIndexCount();
    GrIndexBuffer* aaFillRectIndexBuffer(GrGpu* gpu);

    static int aaStrokeRectIndexCount();
    GrIndexBuffer* aaStrokeRectIndexBuffer(GrGpu* gpu);

    typedef GrRefCnt INHERITED;
};

#endif 

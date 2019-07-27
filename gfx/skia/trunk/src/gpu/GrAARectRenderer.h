






#ifndef GrAARectRenderer_DEFINED
#define GrAARectRenderer_DEFINED

#include "SkMatrix.h"
#include "SkRect.h"
#include "SkRefCnt.h"
#include "SkStrokeRec.h"

class GrGpu;
class GrDrawTarget;
class GrIndexBuffer;




class GrAARectRenderer : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrAARectRenderer)

    GrAARectRenderer()
    : fAAFillRectIndexBuffer(NULL)
    , fAAMiterStrokeRectIndexBuffer(NULL)
    , fAABevelStrokeRectIndexBuffer(NULL) {
    }

    void reset();

    ~GrAARectRenderer() {
        this->reset();
    }

    
    

    void fillAARect(GrGpu* gpu,
                    GrDrawTarget* target,
                    const SkRect& rect,
                    const SkMatrix& combinedMatrix,
                    const SkRect& devRect,
                    bool useVertexCoverage) {
#ifdef SHADER_AA_FILL_RECT
        if (combinedMatrix.rectStaysRect()) {
            this->shaderFillAlignedAARect(gpu, target,
                                          rect, combinedMatrix);
        } else {
            this->shaderFillAARect(gpu, target,
                                   rect, combinedMatrix);
        }
#else
        this->geometryFillAARect(gpu, target,
                                 rect, combinedMatrix,
                                 devRect, useVertexCoverage);
#endif
    }

    void strokeAARect(GrGpu* gpu,
                      GrDrawTarget* target,
                      const SkRect& rect,
                      const SkMatrix& combinedMatrix,
                      const SkRect& devRect,
                      const SkStrokeRec& stroke,
                      bool useVertexCoverage);

    
    void fillAANestedRects(GrGpu* gpu,
                           GrDrawTarget* target,
                           const SkRect rects[2],
                           const SkMatrix& combinedMatrix,
                           bool useVertexCoverage);

private:
    GrIndexBuffer*              fAAFillRectIndexBuffer;
    GrIndexBuffer*              fAAMiterStrokeRectIndexBuffer;
    GrIndexBuffer*              fAABevelStrokeRectIndexBuffer;

    GrIndexBuffer* aaFillRectIndexBuffer(GrGpu* gpu);

    static int aaStrokeRectIndexCount(bool miterStroke);
    GrIndexBuffer* aaStrokeRectIndexBuffer(GrGpu* gpu, bool miterStroke);

    
    
    void geometryFillAARect(GrGpu* gpu,
                            GrDrawTarget* target,
                            const SkRect& rect,
                            const SkMatrix& combinedMatrix,
                            const SkRect& devRect,
                            bool useVertexCoverage);

    void shaderFillAARect(GrGpu* gpu,
                          GrDrawTarget* target,
                          const SkRect& rect,
                          const SkMatrix& combinedMatrix);

    void shaderFillAlignedAARect(GrGpu* gpu,
                                 GrDrawTarget* target,
                                 const SkRect& rect,
                                 const SkMatrix& combinedMatrix);

    void geometryStrokeAARect(GrGpu* gpu,
                              GrDrawTarget* target,
                              const SkRect& devOutside,
                              const SkRect& devOutsideAssist,
                              const SkRect& devInside,
                              bool useVertexCoverage,
                              bool miterStroke);

    typedef SkRefCnt INHERITED;
};

#endif 

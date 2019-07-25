









#ifndef GrGpuGLFixed_DEFINED
#define GrGpuGLFixed_DEFINED

#include "GrDrawState.h"
#include "GrGpuGL.h"


class GrGpuGLFixed : public GrGpuGL {
public:
             GrGpuGLFixed(const GrGLInterface* glInterface);
    virtual ~GrGpuGLFixed();

protected:
    
    virtual bool flushGraphicsState(GrPrimitiveType type);
    virtual void setupGeometry(int* startVertex,
                               int* startIndex,
                               int vertexCount,
                               int indexCount);

private:
    virtual void resetContext();

    
    const GrMatrix& getHWSamplerMatrix(int stage) const {
        return fHWDrawState.fSamplerStates[stage].getMatrix();
    }
    void recordHWSamplerMatrix(int stage, const GrMatrix& matrix) {
        fHWDrawState.fSamplerStates[stage].setMatrix(matrix);
    }

    
    
    
    
    
    enum TextureEnvRGBOperands {
        kAlpha_TextureEnvRGBOperand,
        kColor_TextureEnvRGBOperand,
    };
    TextureEnvRGBOperands fHWRGBOperand0[GrDrawState::kNumStages];

    void flushProjectionMatrix();

    
    
    bool fTextVerts;

    
    
    int fBaseVertex;

    typedef GrGpuGL INHERITED;
};

#endif

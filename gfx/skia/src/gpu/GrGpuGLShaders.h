









#ifndef GrGpuGLShaders_DEFINED
#define GrGpuGLShaders_DEFINED

#include "GrGpuGL.h"
#include "GrGLProgram.h"

class GrGpuGLProgram;


class GrGpuGLShaders : public GrGpuGL {
public:
             GrGpuGLShaders(const GrGLInterface* glInterface);
    virtual ~GrGpuGLShaders();

    virtual void resetContext();

    virtual void abandonResources();

    bool programUnitTest();

protected:
    
    virtual bool flushGraphicsState(GrPrimitiveType type);
    virtual void setupGeometry(int* startVertex,
                               int* startIndex,
                               int vertexCount,
                               int indexCount);
    virtual void postDraw();

private:

    
    typedef GrGLProgram::ProgramDesc ProgramDesc;
    typedef ProgramDesc::StageDesc   StageDesc;
    typedef GrGLProgram::CachedData  CachedData;

    class ProgramCache;

    
    const GrMatrix& getHWSamplerMatrix(int stage);
    void recordHWSamplerMatrix(int stage, const GrMatrix& matrix);

    
    void flushTextureMatrix(int stage);

    
    void flushTextureDomain(int stage);

    
    void flushColor(GrColor color);

    
    void flushViewMatrix();

    
    void flushRadial2(int stage);

    
    void flushConvolution(int stage);

    
    void flushTexelSize(int stage);

    
    void flushEdgeAAData();

    static void DeleteProgram(const GrGLInterface* gl,
                              CachedData* programData);

    void buildProgram(GrPrimitiveType typeBlend,
                      BlendOptFlags blendOpts,
                      GrBlendCoeff dstCoeff);

    ProgramCache*               fProgramCache;
    CachedData*                 fProgramData;
    GrGLuint                    fHWProgramID;
    GrGLProgram                 fCurrentProgram;
    
    
    GrGLint                     fMaxVertexAttribs;

    typedef GrGpuGL INHERITED;
};

#endif


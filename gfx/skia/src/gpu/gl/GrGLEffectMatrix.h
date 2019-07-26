






#ifndef GrGLEffectMatrix_DEFINED
#define GrGLEffectMatrix_DEFINED

#include "GrGLEffect.h"
#include "SkMatrix.h"

class GrTexture;












class GrGLEffectMatrix {
private:
    
    enum MatrixTypes {
        kIdentity_MatrixType    = 0,
        kTrans_MatrixType       = 1,
        kNoPersp_MatrixType     = 2,
        kGeneral_MatrixType     = 3,
    };
    
    
    enum {
        kMatrixTypeKeyBits      = 2,
        kMatrixTypeKeyMask      = (1 << kMatrixTypeKeyBits) - 1,
        kPositionCoords_Flag    = (1 << kMatrixTypeKeyBits),
        kKeyBitsPrivate         = kMatrixTypeKeyBits + 1,
    };

public:

    typedef GrEffect::CoordsType CoordsType;

    typedef GrGLEffect::EffectKey EffectKey;

    




    enum {
        kKeyBits = kKeyBitsPrivate,
        kKeyMask = (1 << kKeyBits) - 1,
    };

    GrGLEffectMatrix(CoordsType coordsType)
        : fUni(GrGLUniformManager::kInvalidUniformHandle)
        , fCoordsType(coordsType) {
        GrAssert(GrEffect::kLocal_CoordsType == coordsType ||
                 GrEffect::kPosition_CoordsType == coordsType);
        fPrevMatrix = SkMatrix::InvalidMatrix();
    }

    






    static EffectKey GenKey(const SkMatrix& effectMatrix,
                            const GrDrawEffect&,
                            CoordsType,
                            const GrTexture*);

    








    GrSLType emitCode(GrGLShaderBuilder*,
                      EffectKey,
                      const char** fsCoordName, 
                      const char** vsCoordName = NULL,
                      const char* suffix = NULL);

    



    void emitCodeMakeFSCoords2D(GrGLShaderBuilder*,
                                EffectKey,
                                const char** fsCoordName, 
                                const char** vsVaryingName = NULL,
                                GrSLType* vsVaryingType = NULL,
                                const char* suffix = NULL);
    



    void setData(const GrGLUniformManager& uniformManager,
                 const SkMatrix& effectMatrix,
                 const GrDrawEffect& drawEffect,
                 const GrTexture*);

    GrGLUniformManager::UniformHandle fUni;
    GrSLType                          fUniType;
    SkMatrix                          fPrevMatrix;
    CoordsType                        fCoordsType;
};

#endif

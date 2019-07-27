









#ifndef GrEffectStage_DEFINED
#define GrEffectStage_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrEffect.h"
#include "SkMatrix.h"
#include "GrTypes.h"

#include "SkShader.h"

class GrEffectStage {
public:
    explicit GrEffectStage(const GrEffect* effect, int attrIndex0 = -1, int attrIndex1 = -1)
    : fEffect(SkRef(effect)) {
        fCoordChangeMatrixSet = false;
        fVertexAttribIndices[0] = attrIndex0;
        fVertexAttribIndices[1] = attrIndex1;
    }

    GrEffectStage(const GrEffectStage& other) {
        *this = other;
    }

    GrEffectStage& operator= (const GrEffectStage& other) {
        fCoordChangeMatrixSet = other.fCoordChangeMatrixSet;
        if (other.fCoordChangeMatrixSet) {
            fCoordChangeMatrix = other.fCoordChangeMatrix;
        }
        fEffect.reset(SkRef(other.fEffect.get()));
        memcpy(fVertexAttribIndices, other.fVertexAttribIndices, sizeof(fVertexAttribIndices));
        return *this;
    }

    bool operator== (const GrEffectStage& other) const {
        SkASSERT(NULL != fEffect.get());
        SkASSERT(NULL != other.fEffect.get());

        if (!this->getEffect()->isEqual(*other.getEffect())) {
            return false;
        }

        if (fCoordChangeMatrixSet != other.fCoordChangeMatrixSet) {
            return false;
        }

        if (!fCoordChangeMatrixSet) {
            return true;
        }

        return fCoordChangeMatrix == other.fCoordChangeMatrix;
    }

    bool operator!= (const GrEffectStage& s) const { return !(*this == s); }

    





    void localCoordChange(const SkMatrix& matrix) {
        if (fCoordChangeMatrixSet) {
            fCoordChangeMatrix.preConcat(matrix);
        } else {
            fCoordChangeMatrixSet = true;
            fCoordChangeMatrix = matrix;
        }
    }

    class SavedCoordChange {
    private:
        bool fCoordChangeMatrixSet;
        SkMatrix fCoordChangeMatrix;
        SkDEBUGCODE(mutable SkAutoTUnref<const GrEffect> fEffect;)

        friend class GrEffectStage;
    };

    





    void saveCoordChange(SavedCoordChange* savedCoordChange) const {
        savedCoordChange->fCoordChangeMatrixSet = fCoordChangeMatrixSet;
        if (fCoordChangeMatrixSet) {
            savedCoordChange->fCoordChangeMatrix = fCoordChangeMatrix;
        }
        SkASSERT(NULL == savedCoordChange->fEffect.get());
        SkDEBUGCODE(SkRef(fEffect.get());)
        SkDEBUGCODE(savedCoordChange->fEffect.reset(fEffect.get());)
    }

    


    void restoreCoordChange(const SavedCoordChange& savedCoordChange) {
        fCoordChangeMatrixSet = savedCoordChange.fCoordChangeMatrixSet;
        if (fCoordChangeMatrixSet) {
            fCoordChangeMatrix = savedCoordChange.fCoordChangeMatrix;
        }
        SkASSERT(savedCoordChange.fEffect.get() == fEffect);
        SkDEBUGCODE(savedCoordChange.fEffect.reset(NULL);)
    }

    



    const SkMatrix& getCoordChangeMatrix() const {
        if (fCoordChangeMatrixSet) {
            return fCoordChangeMatrix;
        } else {
            return SkMatrix::I();
        }
    }

    const GrEffect* getEffect() const { return fEffect.get(); }

    const int* getVertexAttribIndices() const { return fVertexAttribIndices; }
    int getVertexAttribIndexCount() const { return fEffect->numVertexAttribs(); }

private:
    bool                                fCoordChangeMatrixSet;
    SkMatrix                            fCoordChangeMatrix;
    SkAutoTUnref<const GrEffect>        fEffect;
    int                                 fVertexAttribIndices[2];
};

#endif











#ifndef GrEffectStage_DEFINED
#define GrEffectStage_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrEffect.h"
#include "SkMatrix.h"
#include "GrTypes.h"

#include "SkShader.h"

class GrEffectStage {
public:
    explicit GrEffectStage(const GrEffectRef* effectRef, int attrIndex0 = -1, int attrIndex1 = -1)
    : fEffectRef(SkRef(effectRef)) {
        fCoordChangeMatrixSet = false;
        fVertexAttribIndices[0] = attrIndex0;
        fVertexAttribIndices[1] = attrIndex1;
    }

    GrEffectStage(const GrEffectStage& other) {
        *this = other;
    }

    class DeferredStage;
    
    explicit GrEffectStage(const DeferredStage& deferredStage) {
        deferredStage.restoreTo(this);
    }

    GrEffectStage& operator= (const GrEffectStage& other) {
        fCoordChangeMatrixSet = other.fCoordChangeMatrixSet;
        if (other.fCoordChangeMatrixSet) {
            fCoordChangeMatrix = other.fCoordChangeMatrix;
        }
        fEffectRef.reset(SkRef(other.fEffectRef.get()));
        memcpy(fVertexAttribIndices, other.fVertexAttribIndices, sizeof(fVertexAttribIndices));
        return *this;
    }

    bool operator== (const GrEffectStage& other) const {
        SkASSERT(NULL != fEffectRef.get());
        SkASSERT(NULL != other.fEffectRef.get());

        if (!(*this->getEffect())->isEqual(*other.getEffect())) {
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
        SkDEBUGCODE(mutable SkAutoTUnref<const GrEffectRef> fEffectRef;)

        friend class GrEffectStage;
    };

    





    void saveCoordChange(SavedCoordChange* savedCoordChange) const {
        savedCoordChange->fCoordChangeMatrixSet = fCoordChangeMatrixSet;
        if (fCoordChangeMatrixSet) {
            savedCoordChange->fCoordChangeMatrix = fCoordChangeMatrix;
        }
        SkASSERT(NULL == savedCoordChange->fEffectRef.get());
        SkDEBUGCODE(SkRef(fEffectRef.get());)
        SkDEBUGCODE(savedCoordChange->fEffectRef.reset(fEffectRef.get());)
    }

    


    void restoreCoordChange(const SavedCoordChange& savedCoordChange) {
        fCoordChangeMatrixSet = savedCoordChange.fCoordChangeMatrixSet;
        if (fCoordChangeMatrixSet) {
            fCoordChangeMatrix = savedCoordChange.fCoordChangeMatrix;
        }
        SkASSERT(savedCoordChange.fEffectRef.get() == fEffectRef);
        SkDEBUGCODE(savedCoordChange.fEffectRef.reset(NULL);)
    }

    



    class DeferredStage {
    public:
        DeferredStage() : fEffect(NULL) {
            SkDEBUGCODE(fInitialized = false;)
        }

        ~DeferredStage() {
            if (NULL != fEffect) {
                fEffect->decDeferredRefCounts();
            }
        }

        void saveFrom(const GrEffectStage& stage) {
            SkASSERT(!fInitialized);
            SkASSERT(NULL != stage.fEffectRef.get());
            stage.fEffectRef->get()->incDeferredRefCounts();
            fEffect = stage.fEffectRef->get();
            fCoordChangeMatrixSet = stage.fCoordChangeMatrixSet;
            if (fCoordChangeMatrixSet) {
                fCoordChangeMatrix = stage.fCoordChangeMatrix;
            }
            fVertexAttribIndices[0] = stage.fVertexAttribIndices[0];
            fVertexAttribIndices[1] = stage.fVertexAttribIndices[1];
            SkDEBUGCODE(fInitialized = true;)
        }

        void restoreTo(GrEffectStage* stage) const {
            SkASSERT(fInitialized);
            stage->fEffectRef.reset(GrEffect::CreateEffectRef(fEffect));
            stage->fCoordChangeMatrixSet = fCoordChangeMatrixSet;
            if (fCoordChangeMatrixSet) {
                stage->fCoordChangeMatrix = fCoordChangeMatrix;
            }
            stage->fVertexAttribIndices[0] = fVertexAttribIndices[0];
            stage->fVertexAttribIndices[1] = fVertexAttribIndices[1];
        }

        bool isEqual(const GrEffectStage& stage, bool ignoreCoordChange) const {
            if (fVertexAttribIndices[0] != stage.fVertexAttribIndices[0] ||
                fVertexAttribIndices[1] != stage.fVertexAttribIndices[1]) {
                return false;
            }

            if (!(*stage.getEffect())->isEqual(*fEffect)) {
                return false;
            }

            if (ignoreCoordChange) {
                
                
                return true;
            }

            if (fCoordChangeMatrixSet != stage.fCoordChangeMatrixSet) {
                return false;
            }

            if (!fCoordChangeMatrixSet) {
                return true;
            }

            return fCoordChangeMatrix == stage.fCoordChangeMatrix;
        }

    private:
        const GrEffect*               fEffect;
        bool                          fCoordChangeMatrixSet;
        SkMatrix                      fCoordChangeMatrix;
        int                           fVertexAttribIndices[2];
        SkDEBUGCODE(bool fInitialized;)
    };

    



    const SkMatrix& getCoordChangeMatrix() const {
        if (fCoordChangeMatrixSet) {
            return fCoordChangeMatrix;
        } else {
            return SkMatrix::I();
        }
    }

    const GrEffectRef* getEffect() const { return fEffectRef.get(); }

    const int* getVertexAttribIndices() const { return fVertexAttribIndices; }
    int getVertexAttribIndexCount() const { return fEffectRef->get()->numVertexAttribs(); }

private:
    bool                                fCoordChangeMatrixSet;
    SkMatrix                            fCoordChangeMatrix;
    SkAutoTUnref<const GrEffectRef>     fEffectRef;
    int                                 fVertexAttribIndices[2];
};

#endif

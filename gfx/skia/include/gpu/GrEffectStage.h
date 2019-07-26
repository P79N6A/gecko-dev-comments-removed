









#ifndef GrEffectStage_DEFINED
#define GrEffectStage_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrEffect.h"
#include "SkMatrix.h"
#include "GrTypes.h"

#include "SkShader.h"

class GrEffectStage {
public:
    GrEffectStage()
    : fEffectRef (NULL) {
        GR_DEBUGCODE(fSavedCoordChangeCnt = 0;)
    }

    ~GrEffectStage() {
        GrSafeUnref(fEffectRef);
        GrAssert(0 == fSavedCoordChangeCnt);
    }

    bool operator ==(const GrEffectStage& other) const {
        
        if (NULL == fEffectRef) {
            return NULL == other.fEffectRef;
        } else if (NULL == other.fEffectRef) {
            return false;
        }

        if (!(*this->getEffect())->isEqual(*other.getEffect())) {
            return false;
        }

        return fCoordChangeMatrix == other.fCoordChangeMatrix;
    }

    bool operator !=(const GrEffectStage& s) const { return !(*this == s); }

    GrEffectStage& operator =(const GrEffectStage& other) {
        GrSafeAssign(fEffectRef, other.fEffectRef);
        if (NULL != fEffectRef) {
            fCoordChangeMatrix = other.fCoordChangeMatrix;
        }
        return *this;
    }

    





    void localCoordChange(const SkMatrix& matrix) { fCoordChangeMatrix.preConcat(matrix); }

    class SavedCoordChange {
    private:
        SkMatrix fCoordChangeMatrix;
        GR_DEBUGCODE(mutable SkAutoTUnref<const GrEffectRef> fEffectRef;)

        friend class GrEffectStage;
    };

    





    void saveCoordChange(SavedCoordChange* savedCoordChange) const {
        savedCoordChange->fCoordChangeMatrix = fCoordChangeMatrix;
        GrAssert(NULL == savedCoordChange->fEffectRef.get());
        GR_DEBUGCODE(GrSafeRef(fEffectRef);)
        GR_DEBUGCODE(savedCoordChange->fEffectRef.reset(fEffectRef);)
        GR_DEBUGCODE(++fSavedCoordChangeCnt);
    }

    


    void restoreCoordChange(const SavedCoordChange& savedCoordChange) {
        fCoordChangeMatrix = savedCoordChange.fCoordChangeMatrix;
        GrAssert(savedCoordChange.fEffectRef.get() == fEffectRef);
        GR_DEBUGCODE(--fSavedCoordChangeCnt);
        GR_DEBUGCODE(savedCoordChange.fEffectRef.reset(NULL);)
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
            GrAssert(!fInitialized);
            if (NULL != stage.fEffectRef) {
                stage.fEffectRef->get()->incDeferredRefCounts();
                fEffect = stage.fEffectRef->get();
                fCoordChangeMatrix = stage.fCoordChangeMatrix;
                fVertexAttribIndices[0] = stage.fVertexAttribIndices[0];
                fVertexAttribIndices[1] = stage.fVertexAttribIndices[1];
            }
            SkDEBUGCODE(fInitialized = true;)
        }

        void restoreTo(GrEffectStage* stage) {
            GrAssert(fInitialized);
            const GrEffectRef* oldEffectRef = stage->fEffectRef;
            if (NULL != fEffect) {
                stage->fEffectRef = GrEffect::CreateEffectRef(fEffect);
                stage->fCoordChangeMatrix = fCoordChangeMatrix;
                stage->fVertexAttribIndices[0] = fVertexAttribIndices[0];
                stage->fVertexAttribIndices[1] = fVertexAttribIndices[1];
            } else {
                stage->fEffectRef = NULL;
            }
            SkSafeUnref(oldEffectRef);
        }

        bool isEqual(const GrEffectStage& stage) const {
            if (NULL == stage.fEffectRef) {
                return NULL == fEffect;
            } else if (NULL == fEffect) {
                return false;
            }

            if (fVertexAttribIndices[0] != stage.fVertexAttribIndices[0]
                || fVertexAttribIndices[1] != stage.fVertexAttribIndices[1]) {
                return false;
            }

            if (!(*stage.getEffect())->isEqual(*fEffect)) {
                return false;
            }

            return fCoordChangeMatrix == stage.fCoordChangeMatrix;
        }

    private:
        const GrEffect*               fEffect;
        SkMatrix                      fCoordChangeMatrix;
        int                           fVertexAttribIndices[2];
        SkDEBUGCODE(bool fInitialized;)
    };

    



    const SkMatrix& getCoordChangeMatrix() const { return fCoordChangeMatrix; }

    void reset() {
        GrSafeSetNull(fEffectRef);
    }

    const GrEffectRef* setEffect(const GrEffectRef* EffectRef) {
        GrAssert(0 == fSavedCoordChangeCnt);
        GrSafeAssign(fEffectRef, EffectRef);
        fCoordChangeMatrix.reset();

        fVertexAttribIndices[0] = -1;
        fVertexAttribIndices[1] = -1;

        return EffectRef;
    }

    const GrEffectRef* setEffect(const GrEffectRef* EffectRef, int attr0, int attr1 = -1) {
        GrAssert(0 == fSavedCoordChangeCnt);
        GrSafeAssign(fEffectRef, EffectRef);
        fCoordChangeMatrix.reset();

        fVertexAttribIndices[0] = attr0;
        fVertexAttribIndices[1] = attr1;

        return EffectRef;
    }

    const GrEffectRef* getEffect() const { return fEffectRef; }

    const int* getVertexAttribIndices() const { return fVertexAttribIndices; }
    int getVertexAttribIndexCount() const { return fEffectRef->get()->numVertexAttribs(); }

private:
    SkMatrix                fCoordChangeMatrix;
    const GrEffectRef*      fEffectRef;
    int                     fVertexAttribIndices[2];

    GR_DEBUGCODE(mutable int fSavedCoordChangeCnt;)
};

#endif

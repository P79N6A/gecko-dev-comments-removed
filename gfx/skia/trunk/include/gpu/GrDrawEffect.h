
#ifndef GrDrawEffect_DEFINED
#define GrDrawEffect_DEFINED

#include "GrEffectStage.h"













class GrDrawEffect {
public:
    GrDrawEffect(const GrEffectStage& stage, bool explicitLocalCoords)
        : fEffectStage(&stage)
        , fExplicitLocalCoords(explicitLocalCoords) {
        SkASSERT(NULL != fEffectStage);
        SkASSERT(NULL != fEffectStage->getEffect());
    }
    const GrEffect* effect() const { return fEffectStage->getEffect(); }

    template <typename T>
    const T& castEffect() const { return *static_cast<const T*>(this->effect()); }

    const SkMatrix& getCoordChangeMatrix() const {
        if (fExplicitLocalCoords) {
            return SkMatrix::I();
        } else {
            return fEffectStage->getCoordChangeMatrix();
        }
    }

    bool programHasExplicitLocalCoords() const { return fExplicitLocalCoords; }

    const int* getVertexAttribIndices() const { return fEffectStage->getVertexAttribIndices(); }
    int getVertexAttribIndexCount() const { return fEffectStage->getVertexAttribIndexCount(); }

private:
    const GrEffectStage*    fEffectStage;
    bool                    fExplicitLocalCoords;
};

#endif








#ifndef GrCoordTransform_DEFINED
#define GrCoordTransform_DEFINED

#include "GrEffect.h"
#include "SkMatrix.h"
#include "GrTexture.h"
#include "GrTypes.h"





enum GrCoordSet {
    





    kLocal_GrCoordSet,

    




    kPosition_GrCoordSet
};






class GrCoordTransform : SkNoncopyable {
public:
    GrCoordTransform() { SkDEBUGCODE(fInEffect = false); }

    


    GrCoordTransform(GrCoordSet sourceCoords, const GrTexture* texture) {
        SkDEBUGCODE(fInEffect = false);
        this->reset(sourceCoords, texture);
    }

    




    GrCoordTransform(GrCoordSet sourceCoords, const SkMatrix& m, const GrTexture* texture = NULL) {
        SkDEBUGCODE(fInEffect = false);
        this->reset(sourceCoords, m, texture);
    }

    void reset(GrCoordSet sourceCoords, const GrTexture* texture) {
        SkASSERT(!fInEffect);
        SkASSERT(NULL != texture);
        this->reset(sourceCoords, GrEffect::MakeDivByTextureWHMatrix(texture), texture);
    }

    void reset(GrCoordSet sourceCoords, const SkMatrix& m, const GrTexture* texture = NULL) {
        SkASSERT(!fInEffect);
        fSourceCoords = sourceCoords;
        fMatrix = m;
        fReverseY = NULL != texture && kBottomLeft_GrSurfaceOrigin == texture->origin();
    }

    GrCoordTransform& operator= (const GrCoordTransform& other) {
        SkASSERT(!fInEffect);
        fSourceCoords = other.fSourceCoords;
        fMatrix = other.fMatrix;
        fReverseY = other.fReverseY;
        return *this;
    }

    



    SkMatrix* accessMatrix() {
        SkASSERT(!fInEffect);
        return &fMatrix;
    }

    bool operator== (const GrCoordTransform& other) const {
        return fSourceCoords == other.fSourceCoords &&
               fMatrix.cheapEqualTo(other.fMatrix) &&
               fReverseY == other.fReverseY;
    }

    GrCoordSet sourceCoords() const { return fSourceCoords; }
    const SkMatrix& getMatrix() const { return fMatrix; }
    bool reverseY() const { return fReverseY; }

private:
    GrCoordSet fSourceCoords;
    SkMatrix   fMatrix;
    bool       fReverseY;

    typedef SkNoncopyable INHERITED;

#ifdef SK_DEBUG
public:
    void setInEffect() const { fInEffect = true; }
private:
    mutable bool fInEffect;
#endif
};

#endif

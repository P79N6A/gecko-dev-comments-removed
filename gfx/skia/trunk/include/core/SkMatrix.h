








#ifndef SkMatrix_DEFINED
#define SkMatrix_DEFINED

#include "SkDynamicAnnotations.h"
#include "SkRect.h"

class SkString;


typedef SkScalar SkPersp;
#define SkScalarToPersp(x) (x)
#define SkPerspToScalar(x) (x)








class SK_API SkMatrix {
public:
    


    enum TypeMask {
        kIdentity_Mask      = 0,
        kTranslate_Mask     = 0x01,  
        kScale_Mask         = 0x02,  
        kAffine_Mask        = 0x04,  
        kPerspective_Mask   = 0x08   
    };

    





    TypeMask getType() const {
        if (fTypeMask & kUnknown_Mask) {
            fTypeMask = this->computeTypeMask();
        }
        
        return (TypeMask)(fTypeMask & 0xF);
    }

    

    bool isIdentity() const {
        return this->getType() == 0;
    }

    



    bool rectStaysRect() const {
        if (fTypeMask & kUnknown_Mask) {
            fTypeMask = this->computeTypeMask();
        }
        return (fTypeMask & kRectStaysRect_Mask) != 0;
    }
    
    bool preservesAxisAlignment() const { return this->rectStaysRect(); }

    


    bool hasPerspective() const {
        return SkToBool(this->getPerspectiveTypeMaskOnly() &
                        kPerspective_Mask);
    }

    


    bool isSimilarity(SkScalar tol = SK_ScalarNearlyZero) const;

    



    bool preservesRightAngles(SkScalar tol = SK_ScalarNearlyZero) const;

    enum {
        kMScaleX,
        kMSkewX,
        kMTransX,
        kMSkewY,
        kMScaleY,
        kMTransY,
        kMPersp0,
        kMPersp1,
        kMPersp2
    };

    


    enum {
        kAScaleX,
        kASkewY,
        kASkewX,
        kAScaleY,
        kATransX,
        kATransY
    };

    SkScalar operator[](int index) const {
        SkASSERT((unsigned)index < 9);
        return fMat[index];
    }

    SkScalar get(int index) const {
        SkASSERT((unsigned)index < 9);
        return fMat[index];
    }

    SkScalar getScaleX() const { return fMat[kMScaleX]; }
    SkScalar getScaleY() const { return fMat[kMScaleY]; }
    SkScalar getSkewY() const { return fMat[kMSkewY]; }
    SkScalar getSkewX() const { return fMat[kMSkewX]; }
    SkScalar getTranslateX() const { return fMat[kMTransX]; }
    SkScalar getTranslateY() const { return fMat[kMTransY]; }
    SkPersp getPerspX() const { return fMat[kMPersp0]; }
    SkPersp getPerspY() const { return fMat[kMPersp1]; }

    SkScalar& operator[](int index) {
        SkASSERT((unsigned)index < 9);
        this->setTypeMask(kUnknown_Mask);
        return fMat[index];
    }

    void set(int index, SkScalar value) {
        SkASSERT((unsigned)index < 9);
        fMat[index] = value;
        this->setTypeMask(kUnknown_Mask);
    }

    void setScaleX(SkScalar v) { this->set(kMScaleX, v); }
    void setScaleY(SkScalar v) { this->set(kMScaleY, v); }
    void setSkewY(SkScalar v) { this->set(kMSkewY, v); }
    void setSkewX(SkScalar v) { this->set(kMSkewX, v); }
    void setTranslateX(SkScalar v) { this->set(kMTransX, v); }
    void setTranslateY(SkScalar v) { this->set(kMTransY, v); }
    void setPerspX(SkPersp v) { this->set(kMPersp0, v); }
    void setPerspY(SkPersp v) { this->set(kMPersp1, v); }

    void setAll(SkScalar scaleX, SkScalar skewX, SkScalar transX,
                SkScalar skewY, SkScalar scaleY, SkScalar transY,
                SkPersp persp0, SkPersp persp1, SkPersp persp2) {
        fMat[kMScaleX] = scaleX;
        fMat[kMSkewX]  = skewX;
        fMat[kMTransX] = transX;
        fMat[kMSkewY]  = skewY;
        fMat[kMScaleY] = scaleY;
        fMat[kMTransY] = transY;
        fMat[kMPersp0] = persp0;
        fMat[kMPersp1] = persp1;
        fMat[kMPersp2] = persp2;
        this->setTypeMask(kUnknown_Mask);
    }

    

    void reset();
    
    void setIdentity() { this->reset(); }

    

    void setTranslate(SkScalar dx, SkScalar dy);
    void setTranslate(const SkVector& v) { this->setTranslate(v.fX, v.fY); }

    



    void setScale(SkScalar sx, SkScalar sy, SkScalar px, SkScalar py);
    

    void setScale(SkScalar sx, SkScalar sy);
    


    bool setIDiv(int divx, int divy);
    



    void setRotate(SkScalar degrees, SkScalar px, SkScalar py);
    

    void setRotate(SkScalar degrees);
    



    void setSinCos(SkScalar sinValue, SkScalar cosValue,
                   SkScalar px, SkScalar py);
    

    void setSinCos(SkScalar sinValue, SkScalar cosValue);
    



    void setSkew(SkScalar kx, SkScalar ky, SkScalar px, SkScalar py);
    

    void setSkew(SkScalar kx, SkScalar ky);
    



    void setConcat(const SkMatrix& a, const SkMatrix& b);

    


    void preTranslate(SkScalar dx, SkScalar dy);
    


    void preScale(SkScalar sx, SkScalar sy, SkScalar px, SkScalar py);
    


    void preScale(SkScalar sx, SkScalar sy);
    


    void preRotate(SkScalar degrees, SkScalar px, SkScalar py);
    


    void preRotate(SkScalar degrees);
    


    void preSkew(SkScalar kx, SkScalar ky, SkScalar px, SkScalar py);
    


    void preSkew(SkScalar kx, SkScalar ky);
    


    void preConcat(const SkMatrix& other);

    


    void postTranslate(SkScalar dx, SkScalar dy);
    


    void postScale(SkScalar sx, SkScalar sy, SkScalar px, SkScalar py);
    


    void postScale(SkScalar sx, SkScalar sy);
    


    bool postIDiv(int divx, int divy);
    


    void postRotate(SkScalar degrees, SkScalar px, SkScalar py);
    


    void postRotate(SkScalar degrees);
    


    void postSkew(SkScalar kx, SkScalar ky, SkScalar px, SkScalar py);
    


    void postSkew(SkScalar kx, SkScalar ky);
    


    void postConcat(const SkMatrix& other);

    enum ScaleToFit {
        



        kFill_ScaleToFit,
        





        kStart_ScaleToFit,
        




        kCenter_ScaleToFit,
        





        kEnd_ScaleToFit
    };

    







    bool setRectToRect(const SkRect& src, const SkRect& dst, ScaleToFit stf);

    






    bool setPolyToPoly(const SkPoint src[], const SkPoint dst[], int count);

    



    bool SK_WARN_UNUSED_RESULT invert(SkMatrix* inverse) const {
        
        if (this->isIdentity()) {
            if (NULL != inverse) {
                inverse->reset();
            }
            return true;
        }
        return this->invertNonIdentity(inverse);
    }

    




    static void SetAffineIdentity(SkScalar affine[6]);

    




    bool asAffine(SkScalar affine[6]) const;

    









    void mapPoints(SkPoint dst[], const SkPoint src[], int count) const;

    






    void mapPoints(SkPoint pts[], int count) const {
        this->mapPoints(pts, pts, count);
    }

    


    void mapPointsWithStride(SkPoint pts[], size_t stride, int count) const {
        SkASSERT(stride >= sizeof(SkPoint));
        SkASSERT(0 == stride % sizeof(SkScalar));
        for (int i = 0; i < count; ++i) {
            this->mapPoints(pts, pts, 1);
            pts = (SkPoint*)((intptr_t)pts + stride);
        }
    }

    

    void mapPointsWithStride(SkPoint dst[], SkPoint src[],
                             size_t stride, int count) const {
        SkASSERT(stride >= sizeof(SkPoint));
        SkASSERT(0 == stride % sizeof(SkScalar));
        for (int i = 0; i < count; ++i) {
            this->mapPoints(dst, src, 1);
            src = (SkPoint*)((intptr_t)src + stride);
            dst = (SkPoint*)((intptr_t)dst + stride);
        }
    }

    










    void mapHomogeneousPoints(SkScalar dst[], const SkScalar src[], int count) const;

    void mapXY(SkScalar x, SkScalar y, SkPoint* result) const {
        SkASSERT(result);
        this->getMapXYProc()(*this, x, y, result);
    }

    









    void mapVectors(SkVector dst[], const SkVector src[], int count) const;

    






    void mapVectors(SkVector vecs[], int count) const {
        this->mapVectors(vecs, vecs, count);
    }

    






    bool mapRect(SkRect* dst, const SkRect& src) const;

    





    bool mapRect(SkRect* rect) const {
        return this->mapRect(rect, *rect);
    }

    





    void mapRectToQuad(SkPoint dst[4], const SkRect& rect) const {
        
        rect.toQuad(dst);
        this->mapPoints(dst, 4);
    }

    



    SkScalar mapRadius(SkScalar radius) const;

    typedef void (*MapXYProc)(const SkMatrix& mat, SkScalar x, SkScalar y,
                                 SkPoint* result);

    static MapXYProc GetMapXYProc(TypeMask mask) {
        SkASSERT((mask & ~kAllMasks) == 0);
        return gMapXYProcs[mask & kAllMasks];
    }

    MapXYProc getMapXYProc() const {
        return GetMapXYProc(this->getType());
    }

    typedef void (*MapPtsProc)(const SkMatrix& mat, SkPoint dst[],
                                  const SkPoint src[], int count);

    static MapPtsProc GetMapPtsProc(TypeMask mask) {
        SkASSERT((mask & ~kAllMasks) == 0);
        return gMapPtsProcs[mask & kAllMasks];
    }

    MapPtsProc getMapPtsProc() const {
        return GetMapPtsProc(this->getType());
    }

    



    bool fixedStepInX(SkScalar y, SkFixed* stepX, SkFixed* stepY) const;

    







    bool cheapEqualTo(const SkMatrix& m) const {
        return 0 == memcmp(fMat, m.fMat, sizeof(fMat));
    }

    friend bool operator==(const SkMatrix& a, const SkMatrix& b);
    friend bool operator!=(const SkMatrix& a, const SkMatrix& b) {
        return !(a == b);
    }

    enum {
        
        kMaxFlattenSize = 9 * sizeof(SkScalar) + sizeof(uint32_t)
    };
    
    size_t writeToMemory(void* buffer) const;
    







    size_t readFromMemory(const void* buffer, size_t length);

    SkDEVCODE(void dump() const;)
    SK_TO_STRING_NONVIRT()

    





    SkScalar getMinScale() const;

    





    SkScalar getMaxScale() const;

    




    bool getMinMaxScales(SkScalar scaleFactors[2]) const;

    


    static const SkMatrix& I();

    



    static const SkMatrix& InvalidMatrix();

    


    static SkMatrix Concat(const SkMatrix& a, const SkMatrix& b) {
        SkMatrix result;
        result.setConcat(a, b);
        return result;
    }

    



    void dirtyMatrixTypeCache() {
        this->setTypeMask(kUnknown_Mask);
    }

private:
    enum {
        





        kRectStaysRect_Mask = 0x10,

        


        kOnlyPerspectiveValid_Mask = 0x40,

        kUnknown_Mask = 0x80,

        kORableMasks =  kTranslate_Mask |
                        kScale_Mask |
                        kAffine_Mask |
                        kPerspective_Mask,

        kAllMasks = kTranslate_Mask |
                    kScale_Mask |
                    kAffine_Mask |
                    kPerspective_Mask |
                    kRectStaysRect_Mask
    };

    SkScalar         fMat[9];
    mutable SkTRacy<uint32_t> fTypeMask;

    uint8_t computeTypeMask() const;
    uint8_t computePerspectiveTypeMask() const;

    void setTypeMask(int mask) {
        
        SkASSERT(kUnknown_Mask == mask || (mask & kAllMasks) == mask ||
                 ((kUnknown_Mask | kOnlyPerspectiveValid_Mask) & mask)
                 == (kUnknown_Mask | kOnlyPerspectiveValid_Mask));
        fTypeMask = SkToU8(mask);
    }

    void orTypeMask(int mask) {
        SkASSERT((mask & kORableMasks) == mask);
        fTypeMask = SkToU8(fTypeMask | mask);
    }

    void clearTypeMask(int mask) {
        
        SkASSERT((mask & kAllMasks) == mask);
        fTypeMask = fTypeMask & ~mask;
    }

    TypeMask getPerspectiveTypeMaskOnly() const {
        if ((fTypeMask & kUnknown_Mask) &&
            !(fTypeMask & kOnlyPerspectiveValid_Mask)) {
            fTypeMask = this->computePerspectiveTypeMask();
        }
        return (TypeMask)(fTypeMask & 0xF);
    }

    


    bool isTriviallyIdentity() const {
        if (fTypeMask & kUnknown_Mask) {
            return false;
        }
        return ((fTypeMask & 0xF) == 0);
    }

    bool SK_WARN_UNUSED_RESULT invertNonIdentity(SkMatrix* inverse) const;

    static bool Poly2Proc(const SkPoint[], SkMatrix*, const SkPoint& scale);
    static bool Poly3Proc(const SkPoint[], SkMatrix*, const SkPoint& scale);
    static bool Poly4Proc(const SkPoint[], SkMatrix*, const SkPoint& scale);

    static void Identity_xy(const SkMatrix&, SkScalar, SkScalar, SkPoint*);
    static void Trans_xy(const SkMatrix&, SkScalar, SkScalar, SkPoint*);
    static void Scale_xy(const SkMatrix&, SkScalar, SkScalar, SkPoint*);
    static void ScaleTrans_xy(const SkMatrix&, SkScalar, SkScalar, SkPoint*);
    static void Rot_xy(const SkMatrix&, SkScalar, SkScalar, SkPoint*);
    static void RotTrans_xy(const SkMatrix&, SkScalar, SkScalar, SkPoint*);
    static void Persp_xy(const SkMatrix&, SkScalar, SkScalar, SkPoint*);

    static const MapXYProc gMapXYProcs[];

    static void Identity_pts(const SkMatrix&, SkPoint[], const SkPoint[], int);
    static void Trans_pts(const SkMatrix&, SkPoint dst[], const SkPoint[], int);
    static void Scale_pts(const SkMatrix&, SkPoint dst[], const SkPoint[], int);
    static void ScaleTrans_pts(const SkMatrix&, SkPoint dst[], const SkPoint[],
                               int count);
    static void Rot_pts(const SkMatrix&, SkPoint dst[], const SkPoint[], int);
    static void RotTrans_pts(const SkMatrix&, SkPoint dst[], const SkPoint[],
                             int count);
    static void Persp_pts(const SkMatrix&, SkPoint dst[], const SkPoint[], int);

    static const MapPtsProc gMapPtsProcs[];

    friend class SkPerspIter;
};

#endif

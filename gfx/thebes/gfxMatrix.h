




#ifndef GFX_MATRIX_H
#define GFX_MATRIX_H

#include "gfxPoint.h"
#include "gfxTypes.h"
#include "gfxRect.h"





















class gfxMatrix {
public:
    double _11; double _12;
    double _21; double _22;
    double _31; double _32;

    


    gfxMatrix() { Reset(); }

    



    gfxMatrix(gfxFloat a, gfxFloat b, gfxFloat c, gfxFloat d, gfxFloat tx, gfxFloat ty) :
        _11(a),  _12(b),
        _21(c),  _22(d),
        _31(tx), _32(ty) { }

    


    const gfxMatrix& operator *= (const gfxMatrix& m) {
        return Multiply(m);
    }

    


    gfxMatrix operator * (const gfxMatrix& m) const {
        return gfxMatrix(*this).Multiply(m);
    }

    


    bool operator==(const gfxMatrix& other) const
    {
      return FuzzyEqual(_11, other._11) && FuzzyEqual(_12, other._12) &&
             FuzzyEqual(_21, other._21) && FuzzyEqual(_22, other._22) &&
             FuzzyEqual(_31, other._31) && FuzzyEqual(_32, other._32);
    }

    bool operator!=(const gfxMatrix& other) const
    {
      return !(*this == other);
    }

    
    


    const gfxMatrix& Reset();

    bool IsIdentity() const {
       return _11 == 1.0 && _12 == 0.0 &&
              _21 == 0.0 && _22 == 1.0 &&
              _31 == 0.0 && _32 == 0.0;
    }

    






    bool Invert();

    


    bool IsSingular() const {
        
        return (_11 * _22) == (_12 * _21);
    }

    



    const gfxMatrix& Scale(gfxFloat x, gfxFloat y);

    



    const gfxMatrix& Translate(const gfxPoint& pt);

    





    const gfxMatrix& Rotate(gfxFloat radians);

     






    const gfxMatrix& Multiply(const gfxMatrix& m);

    




    const gfxMatrix& PreMultiply(const gfxMatrix& m);

    


    gfxPoint Transform(const gfxPoint& point) const;


    



    gfxSize Transform(const gfxSize& size) const;

    


    gfxRect Transform(const gfxRect& rect) const;

    gfxRect TransformBounds(const gfxRect& rect) const;

    


    gfxPoint GetTranslation() const {
        return gfxPoint(_31, _32);
    }

    



    bool HasNonIntegerTranslation() const {
        return HasNonTranslation() ||
            !FuzzyEqual(_31, floor(_31 + 0.5)) ||
            !FuzzyEqual(_32, floor(_32 + 0.5));
    }

    



    bool HasNonTranslation() const {
        return !FuzzyEqual(_11, 1.0) || !FuzzyEqual(_22, 1.0) ||
               !FuzzyEqual(_21, 0.0) || !FuzzyEqual(_12, 0.0);
    }

    


    bool HasOnlyIntegerTranslation() const {
        return !HasNonIntegerTranslation();
    }

    



    bool HasNonTranslationOrFlip() const {
        return !FuzzyEqual(_11, 1.0) ||
               (!FuzzyEqual(_22, 1.0) && !FuzzyEqual(_22, -1.0)) ||
               !FuzzyEqual(_21, 0.0) || !FuzzyEqual(_12, 0.0);
    }

    




    bool HasNonAxisAlignedTransform() const {
        return !FuzzyEqual(_21, 0.0) || !FuzzyEqual(_12, 0.0);
    }

    


    double Determinant() const {
        return _11*_22 - _12*_21;
    }

    




    gfxSize ScaleFactors(bool xMajor) const {
        double det = Determinant();

        if (det == 0.0)
            return gfxSize(0.0, 0.0);

        gfxSize sz = xMajor ? gfxSize(1.0, 0.0) : gfxSize(0.0, 1.0);
        sz = Transform(sz);

        double major = sqrt(sz.width * sz.width + sz.height * sz.height);
        double minor = 0.0;

        
        if (det < 0.0)
            det = - det;

        if (major)
            minor = det / major;

        if (xMajor)
            return gfxSize(major, minor);

        return gfxSize(minor, major);
    }

    




    void NudgeToIntegers(void);

    



    bool PreservesAxisAlignedRectangles() const {
        return ((FuzzyEqual(_11, 0.0) && FuzzyEqual(_22, 0.0))
            || (FuzzyEqual(_21, 0.0) && FuzzyEqual(_12, 0.0)));
    }

    


    bool HasNonIntegerScale() const {
        return !FuzzyEqual(_11, floor(_11 + 0.5)) ||
               !FuzzyEqual(_22, floor(_22 + 0.5));
    }

private:
    static bool FuzzyEqual(gfxFloat aV1, gfxFloat aV2) {
        return fabs(aV2 - aV1) < 1e-6;
    }
};

#endif 

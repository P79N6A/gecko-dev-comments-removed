




































#ifndef GFX_MATRIX_H
#define GFX_MATRIX_H

#include "gfxPoint.h"
#include "gfxTypes.h"
#include "gfxRect.h"
#include "nsMathUtils.h"





















struct THEBES_API gfxMatrix {
    double xx; double yx;
    double xy; double yy;
    double x0; double y0;

public:
    


    gfxMatrix() { Reset(); }

    



    gfxMatrix(gfxFloat a, gfxFloat b, gfxFloat c, gfxFloat d, gfxFloat tx, gfxFloat ty) :
        xx(a),  yx(b),
        xy(c),  yy(d),
        x0(tx), y0(ty) { }

    


    const gfxMatrix& operator *= (const gfxMatrix& m) {
        return Multiply(m);
    }

    


    gfxMatrix operator * (const gfxMatrix& m) const {
        return gfxMatrix(*this).Multiply(m);
    }

    
    


    const gfxMatrix& Reset();

    bool IsIdentity() const {
       return xx == 1.0 && yx == 0.0 &&
              xy == 0.0 && yy == 1.0 &&
              x0 == 0.0 && y0 == 0.0;
    }

    






    const gfxMatrix& Invert();

    


    bool IsSingular() const {
        
        return (xx * yy) == (yx * xy);
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
        return gfxPoint(x0, y0);
    }

    



    bool HasNonIntegerTranslation() const {
        return HasNonTranslation() ||
            !FuzzyEqual(x0, floor(x0 + 0.5)) ||
            !FuzzyEqual(y0, floor(y0 + 0.5));
    }

    



    bool HasNonTranslation() const {
        return !FuzzyEqual(xx, 1.0) || !FuzzyEqual(yy, 1.0) ||
               !FuzzyEqual(xy, 0.0) || !FuzzyEqual(yx, 0.0);
    }

    


    bool HasOnlyIntegerTranslation() const {
        return !HasNonIntegerTranslation();
    }

    



    bool HasNonTranslationOrFlip() const {
        return !FuzzyEqual(xx, 1.0) ||
               (!FuzzyEqual(yy, 1.0) && !FuzzyEqual(yy, -1.0)) ||
               !FuzzyEqual(xy, 0.0) || !FuzzyEqual(yx, 0.0);
    }

    




    bool HasNonAxisAlignedTransform() const {
        return !FuzzyEqual(xy, 0.0) || !FuzzyEqual(yx, 0.0);
    }

    


    double Determinant() const {
        return xx*yy - yx*xy;
    }

    




    gfxSize ScaleFactors(bool xMajor) const {
        double det = Determinant();

        if (det == 0.0)
            return gfxSize(0.0, 0.0);

        gfxSize sz((xMajor != 0 ? 1.0 : 0.0),
                        (xMajor != 0 ? 0.0 : 1.0));
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
        return ((FuzzyEqual(xx, 0.0) && FuzzyEqual(yy, 0.0))
            || (FuzzyEqual(xy, 0.0) && FuzzyEqual(yx, 0.0)));
    }

    


    bool HasNonIntegerScale() const {
        return !FuzzyEqual(xx, floor(xx + 0.5)) ||
               !FuzzyEqual(yy, floor(yy + 0.5));
    }

private:
    static bool FuzzyEqual(gfxFloat aV1, gfxFloat aV2) {
        return fabs(aV2 - aV1) < 1e-6;
    }
};

#endif 

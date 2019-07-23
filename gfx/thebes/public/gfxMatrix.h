




































#ifndef GFX_MATRIX_H
#define GFX_MATRIX_H

#include "gfxPoint.h"
#include "gfxTypes.h"
#include "gfxRect.h"





















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

    






    const gfxMatrix& Invert();

    


    PRBool IsSingular() const {
        
        return (xx * yy) == (yx * xy);
    }

    



    const gfxMatrix& Scale(gfxFloat x, gfxFloat y);

    



    const gfxMatrix& Translate(const gfxPoint& pt);

    





    const gfxMatrix& Rotate(gfxFloat radians);

     






    const gfxMatrix& Multiply(const gfxMatrix& m);

    


    gfxPoint Transform(const gfxPoint& point) const;


    



    gfxSize Transform(const gfxSize& size) const;

    


    gfxRect Transform(const gfxRect& rect) const;

    gfxRect TransformBounds(const gfxRect& rect) const;

    


    gfxPoint GetTranslation() const {
        return gfxPoint(x0, y0);
    }

    



    bool HasNonTranslation() const {
        return ((xx != 1.0) || (yy != 1.0) ||
                (xy != 0.0) || (yx != 0.0));
    }

    



    bool HasNonTranslationOrFlip() const {
        return ((xx != 1.0) || ((yy != 1.0) && (yy != -1.0)) ||
                (xy != 0.0) || (yx != 0.0));
    }

    




    bool HasNonAxisAlignedTransform() const {
        return ((xy != 0.0) || (yx != 0.0));
    }

    


    double Determinant() const {
        return xx*yy - yx*xy;
    }

    




    gfxSize ScaleFactors(PRBool xMajor) const {
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
};

#endif 

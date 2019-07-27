






#ifndef Sk2DPathEffect_DEFINED
#define Sk2DPathEffect_DEFINED

#include "SkPath.h"
#include "SkPathEffect.h"
#include "SkMatrix.h"

class SK_API Sk2DPathEffect : public SkPathEffect {
public:
    virtual bool filterPath(SkPath*, const SkPath&, SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_UNFLATTENABLE_OBJECT()

protected:
    





    virtual void begin(const SkIRect& uvBounds, SkPath* dst) const;
    virtual void next(const SkPoint& loc, int u, int v, SkPath* dst) const;
    virtual void end(SkPath* dst) const;

    



    virtual void nextSpan(int u, int v, int ucount, SkPath* dst) const;

    const SkMatrix& getMatrix() const { return fMatrix; }

    
    explicit Sk2DPathEffect(const SkMatrix& mat);
    explicit Sk2DPathEffect(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkMatrix    fMatrix, fInverse;
    bool        fMatrixIsInvertible;

    
    Sk2DPathEffect(const Sk2DPathEffect&);
    Sk2DPathEffect& operator=(const Sk2DPathEffect&);

    friend class Sk2DPathEffectBlitter;
    typedef SkPathEffect INHERITED;
};

class SK_API SkLine2DPathEffect : public Sk2DPathEffect {
public:
    static SkLine2DPathEffect* Create(SkScalar width, const SkMatrix& matrix) {
        return SkNEW_ARGS(SkLine2DPathEffect, (width, matrix));
    }

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLine2DPathEffect)

protected:
    SkLine2DPathEffect(SkScalar width, const SkMatrix& matrix)
        : Sk2DPathEffect(matrix), fWidth(width) {}
    explicit SkLine2DPathEffect(SkReadBuffer&);

    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual void nextSpan(int u, int v, int ucount, SkPath*) const SK_OVERRIDE;

private:
    SkScalar fWidth;

    typedef Sk2DPathEffect INHERITED;
};

class SK_API SkPath2DPathEffect : public Sk2DPathEffect {
public:
    



    static SkPath2DPathEffect* Create(const SkMatrix& matrix, const SkPath& path) {
        return SkNEW_ARGS(SkPath2DPathEffect, (matrix, path));
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPath2DPathEffect)

protected:
    SkPath2DPathEffect(const SkMatrix&, const SkPath&);
    explicit SkPath2DPathEffect(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual void next(const SkPoint&, int u, int v, SkPath*) const SK_OVERRIDE;

private:
    SkPath  fPath;

    typedef Sk2DPathEffect INHERITED;
};

#endif

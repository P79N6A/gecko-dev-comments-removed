






#ifndef Sk2DPathEffect_DEFINED
#define Sk2DPathEffect_DEFINED

#include "SkPath.h"
#include "SkPathEffect.h"
#include "SkMatrix.h"

class SK_API Sk2DPathEffect : public SkPathEffect {
public:
    Sk2DPathEffect(const SkMatrix& mat);

    virtual bool filterPath(SkPath*, const SkPath&,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(Sk2DPathEffect)

protected:
    





    virtual void begin(const SkIRect& uvBounds, SkPath* dst) const;
    virtual void next(const SkPoint& loc, int u, int v, SkPath* dst) const;
    virtual void end(SkPath* dst) const;

    



    virtual void nextSpan(int u, int v, int ucount, SkPath* dst) const;

    const SkMatrix& getMatrix() const { return fMatrix; }

    
    Sk2DPathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

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
    SkLine2DPathEffect(SkScalar width, const SkMatrix& matrix)
    : Sk2DPathEffect(matrix), fWidth(width) {}

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLine2DPathEffect)

protected:
    virtual void nextSpan(int u, int v, int ucount, SkPath*) const SK_OVERRIDE;

    SkLine2DPathEffect(SkFlattenableReadBuffer&);

    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkScalar fWidth;

    typedef Sk2DPathEffect INHERITED;
};

class SK_API SkPath2DPathEffect : public Sk2DPathEffect {
public:
    



    SkPath2DPathEffect(const SkMatrix&, const SkPath&);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPath2DPathEffect)

protected:
    SkPath2DPathEffect(SkFlattenableReadBuffer& buffer);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    virtual void next(const SkPoint&, int u, int v, SkPath*) const SK_OVERRIDE;

private:
    SkPath  fPath;

    typedef Sk2DPathEffect INHERITED;
};

#endif

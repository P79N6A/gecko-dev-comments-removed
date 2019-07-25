








#ifndef Sk2DPathEffect_DEFINED
#define Sk2DPathEffect_DEFINED

#include "SkPath.h"
#include "SkPathEffect.h"
#include "SkMatrix.h"

class Sk2DPathEffect : public SkPathEffect {
public:
    Sk2DPathEffect(const SkMatrix& mat);

    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    
    virtual void flatten(SkFlattenableWriteBuffer&);
    virtual Factory getFactory();

protected:
    





    virtual void begin(const SkIRect& uvBounds, SkPath* dst);
    virtual void next(const SkPoint& loc, int u, int v, SkPath* dst);
    virtual void end(SkPath* dst);

    



    virtual void nextSpan(int u, int v, int ucount, SkPath* dst);

    const SkMatrix& getMatrix() const { return fMatrix; }

    
    Sk2DPathEffect(SkFlattenableReadBuffer&);

private:
    SkMatrix    fMatrix, fInverse;
    
    Sk2DPathEffect(const Sk2DPathEffect&);
    Sk2DPathEffect& operator=(const Sk2DPathEffect&);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

    friend class Sk2DPathEffectBlitter;
    typedef SkPathEffect INHERITED;
};

class SkPath2DPathEffect : public Sk2DPathEffect {
public:
    



    SkPath2DPathEffect(const SkMatrix&, const SkPath&);
    
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

protected:
    SkPath2DPathEffect(SkFlattenableReadBuffer& buffer);

    virtual void flatten(SkFlattenableWriteBuffer&);
    virtual Factory getFactory();
    virtual void next(const SkPoint& loc, int u, int v, SkPath* dst);

private:
    SkPath  fPath;

    typedef Sk2DPathEffect INHERITED;
};


#endif

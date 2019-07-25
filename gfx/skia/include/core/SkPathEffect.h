








#ifndef SkPathEffect_DEFINED
#define SkPathEffect_DEFINED

#include "SkFlattenable.h"

class SkPath;









class SK_API SkPathEffect : public SkFlattenable {
public:
    SkPathEffect() {}

    





    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width) = 0;

private:
    
    SkPathEffect(const SkPathEffect&);
    SkPathEffect& operator=(const SkPathEffect&);
};







class SkPairPathEffect : public SkPathEffect {
public:
    SkPairPathEffect(SkPathEffect* pe0, SkPathEffect* pe1);
    virtual ~SkPairPathEffect();

protected:
    SkPairPathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) SK_OVERRIDE;
    
    SkPathEffect* fPE0, *fPE1;
    
private:
    typedef SkPathEffect INHERITED;
};






class SkComposePathEffect : public SkPairPathEffect {
public:
    




    SkComposePathEffect(SkPathEffect* outer, SkPathEffect* inner)
        : INHERITED(outer, inner) {}

    
    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkComposePathEffect, (buffer));
    }

protected:
    virtual Factory getFactory() SK_OVERRIDE { return CreateProc; }

private:
    SkComposePathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

    
    SkComposePathEffect(const SkComposePathEffect&);
    SkComposePathEffect& operator=(const SkComposePathEffect&);
    
    typedef SkPairPathEffect INHERITED;
};






class SkSumPathEffect : public SkPairPathEffect {
public:
    




    SkSumPathEffect(SkPathEffect* first, SkPathEffect* second)
        : INHERITED(first, second) {}

    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer)  {
        return SkNEW_ARGS(SkSumPathEffect, (buffer));
    }

protected:
    virtual Factory getFactory() SK_OVERRIDE { return CreateProc; }

private:
    SkSumPathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

    
    SkSumPathEffect(const SkSumPathEffect&);
    SkSumPathEffect& operator=(const SkSumPathEffect&);

    typedef SkPairPathEffect INHERITED;
};

#endif











#ifndef SkPathEffect_DEFINED
#define SkPathEffect_DEFINED

#include "SkFlattenable.h"

class SkPath;









class SK_API SkPathEffect : public SkFlattenable {
public:
    SkPathEffect() {}

    





    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width) = 0;

    



    virtual void computeFastBounds(SkRect* dst, const SkRect& src);

protected:
    SkPathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    
    SkPathEffect(const SkPathEffect&);
    SkPathEffect& operator=(const SkPathEffect&);

    typedef SkFlattenable INHERITED;
};







class SkPairPathEffect : public SkPathEffect {
public:
    SkPairPathEffect(SkPathEffect* pe0, SkPathEffect* pe1);
    virtual ~SkPairPathEffect();

protected:
    SkPairPathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    
    SkPathEffect* fPE0, *fPE1;
    
private:
    typedef SkPathEffect INHERITED;
};






class SkComposePathEffect : public SkPairPathEffect {
public:
    




    SkComposePathEffect(SkPathEffect* outer, SkPathEffect* inner)
        : INHERITED(outer, inner) {}

    
    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkComposePathEffect)

protected:
    SkComposePathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    
    SkComposePathEffect(const SkComposePathEffect&);
    SkComposePathEffect& operator=(const SkComposePathEffect&);
    
    typedef SkPairPathEffect INHERITED;
};






class SkSumPathEffect : public SkPairPathEffect {
public:
    




    SkSumPathEffect(SkPathEffect* first, SkPathEffect* second)
        : INHERITED(first, second) {}

    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkSumPathEffect)

protected:
    SkSumPathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    
    SkSumPathEffect(const SkSumPathEffect&);
    SkSumPathEffect& operator=(const SkSumPathEffect&);

    typedef SkPairPathEffect INHERITED;
};

#endif


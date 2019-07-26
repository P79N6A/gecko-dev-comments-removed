








#ifndef SkPathEffect_DEFINED
#define SkPathEffect_DEFINED

#include "SkFlattenable.h"
#include "SkPath.h"
#include "SkPoint.h"
#include "SkRect.h"
#include "SkStrokeRec.h"
#include "SkTDArray.h"

class SkPath;









class SK_API SkPathEffect : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkPathEffect)

    














    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect* cullR) const = 0;

    



    virtual void computeFastBounds(SkRect* dst, const SkRect& src) const;

    




    class PointData {
    public:
        PointData()
            : fFlags(0)
            , fPoints(NULL)
            , fNumPoints(0) {
            fSize.set(SK_Scalar1, SK_Scalar1);
            
            
        };
        ~PointData() {
            delete [] fPoints;
        }

        
        
        

        
        enum PointFlags {
            kCircles_PointFlag            = 0x01,   
            kUsePath_PointFlag            = 0x02,   
            kUseClip_PointFlag            = 0x04,   
        };

        uint32_t           fFlags;      
        SkPoint*           fPoints;     
        int                fNumPoints;  
        SkVector           fSize;       
        SkRect             fClipRect;   
        SkPath             fPath;       

        SkPath             fFirst;      
        SkPath             fLast;       
    };

    



    virtual bool asPoints(PointData* results, const SkPath& src,
                          const SkStrokeRec&, const SkMatrix&,
                          const SkRect* cullR) const;

    SK_DEFINE_FLATTENABLE_TYPE(SkPathEffect)

protected:
    SkPathEffect() {}
    SkPathEffect(SkReadBuffer& buffer) : INHERITED(buffer) {}

private:
    
    SkPathEffect(const SkPathEffect&);
    SkPathEffect& operator=(const SkPathEffect&);

    typedef SkFlattenable INHERITED;
};







class SkPairPathEffect : public SkPathEffect {
public:
    virtual ~SkPairPathEffect();

protected:
    SkPairPathEffect(SkPathEffect* pe0, SkPathEffect* pe1);
    SkPairPathEffect(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    SkPathEffect* fPE0, *fPE1;

private:
    typedef SkPathEffect INHERITED;
};






class SkComposePathEffect : public SkPairPathEffect {
public:
    




    static SkComposePathEffect* Create(SkPathEffect* outer, SkPathEffect* inner) {
        return SkNEW_ARGS(SkComposePathEffect, (outer, inner));
    }

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkComposePathEffect)

protected:
    SkComposePathEffect(SkReadBuffer& buffer) : INHERITED(buffer) {}

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkComposePathEffect(SkPathEffect* outer, SkPathEffect* inner)
        : INHERITED(outer, inner) {}

private:
    
    SkComposePathEffect(const SkComposePathEffect&);
    SkComposePathEffect& operator=(const SkComposePathEffect&);

    typedef SkPairPathEffect INHERITED;
};






class SkSumPathEffect : public SkPairPathEffect {
public:
    




    static SkSumPathEffect* Create(SkPathEffect* first, SkPathEffect* second) {
        return SkNEW_ARGS(SkSumPathEffect, (first, second));
    }

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkSumPathEffect)

protected:
    SkSumPathEffect(SkReadBuffer& buffer) : INHERITED(buffer) {}

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkSumPathEffect(SkPathEffect* first, SkPathEffect* second)
        : INHERITED(first, second) {}

private:
    
    SkSumPathEffect(const SkSumPathEffect&);
    SkSumPathEffect& operator=(const SkSumPathEffect&);

    typedef SkPairPathEffect INHERITED;
};

#endif

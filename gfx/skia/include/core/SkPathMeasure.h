








#ifndef SkPathMeasure_DEFINED
#define SkPathMeasure_DEFINED

#include "SkPath.h"
#include "SkTDArray.h"

class SkPathMeasure : SkNoncopyable {
public:
    SkPathMeasure();
    




    SkPathMeasure(const SkPath& path, bool forceClosed);
    ~SkPathMeasure();

    




    void    setPath(const SkPath*, bool forceClosed);

    


    SkScalar getLength();

    




    bool getPosTan(SkScalar distance, SkPoint* position, SkVector* tangent);

    enum MatrixFlags {
        kGetPosition_MatrixFlag     = 0x01,
        kGetTangent_MatrixFlag      = 0x02,
        kGetPosAndTan_MatrixFlag    = kGetPosition_MatrixFlag | kGetTangent_MatrixFlag
    };
    




    bool getMatrix(SkScalar distance, SkMatrix* matrix, MatrixFlags flags = kGetPosAndTan_MatrixFlag);
    





    bool getSegment(SkScalar startD, SkScalar stopD, SkPath* dst, bool startWithMoveTo);

    

    bool isClosed();

    


    bool nextContour();

#ifdef SK_DEBUG
    void    dump();
#endif

private:
    SkPath::Iter    fIter;
    const SkPath*   fPath;
    SkScalar        fLength;            
    int             fFirstPtIndex;      
    bool            fIsClosed;          
    bool            fForceClosed;

    struct Segment {
        SkScalar    fDistance;  
        unsigned    fPtIndex : 15;
        unsigned    fTValue : 15;
        unsigned    fType : 2;

        SkScalar getScalarT() const;
    };
    SkTDArray<Segment>  fSegments;

    static const Segment* NextSegment(const Segment*);

    void     buildSegments();
    SkScalar compute_quad_segs(const SkPoint pts[3], SkScalar distance,
                                int mint, int maxt, int ptIndex);
    SkScalar compute_cubic_segs(const SkPoint pts[3], SkScalar distance,
                                int mint, int maxt, int ptIndex);
    const Segment* distanceToSegment(SkScalar distance, SkScalar* t);
};

#endif








#ifndef SkOpAngle_DEFINED
#define SkOpAngle_DEFINED

#include "SkLineParameters.h"
#include "SkPath.h"
#include "SkPathOpsCubic.h"

class SkOpSegment;
struct SkOpSpan;



class SkOpAngle {
public:
    enum { kStackBasedCount = 8 }; 
    enum IncludeType {
        kUnaryWinding,
        kUnaryXor,
        kBinarySingle,
        kBinaryOpp,
    };

    bool operator<(const SkOpAngle& rh) const;

    bool calcSlop(double x, double y, double rx, double ry, bool* result) const;

    double dx() const {
        return fTangentPart.dx();
    }

    double dy() const {
        return fTangentPart.dy();
    }

    int end() const {
        return fEnd;
    }

    bool isHorizontal() const;

    SkOpSpan* lastMarked() const {
        return fLastMarked;
    }

    void set(const SkOpSegment* segment, int start, int end);

    void setLastMarked(SkOpSpan* marked) {
        fLastMarked = marked;
    }

    SkOpSegment* segment() const {
        return const_cast<SkOpSegment*>(fSegment);
    }

    int sign() const {
        return SkSign32(fStart - fEnd);
    }

    int start() const {
        return fStart;
    }

    bool unorderable() const {
        return fUnorderable;
    }

    bool unsortable() const {
        return fUnsortable;
    }

#ifdef SK_DEBUG
    void dump() const;
#endif

#if DEBUG_ANGLE
    void setID(int id) {
        fID = id;
    }
#endif

private:
    bool lengthen(const SkOpAngle& );
    void setSpans();

    SkDCubic fCurvePart; 
    SkDCubic fCurveHalf; 
    double fSide;
    double fSide2;
    SkLineParameters fTangentPart;
    SkLineParameters fTangentHalf;
    const SkOpSegment* fSegment;
    SkOpSpan* fLastMarked;
    int fStart;
    int fEnd;
    bool fComputed; 
    
    
    
    bool fUnorderable;
    mutable bool fUnsortable;  
#if DEBUG_ANGLE
    int fID;
#endif
};

#endif

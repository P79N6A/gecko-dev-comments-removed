





#ifndef SkOpAngle_DEFINED
#define SkOpAngle_DEFINED

#include "SkChunkAlloc.h"
#include "SkLineParameters.h"

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


    int end() const {
        return fEnd;
    }

    const SkOpAngle* findFirst() const;

    bool inLoop() const {
        return !!fNext;
    }

    void insert(SkOpAngle* );
    bool isHorizontal() const;
    SkOpSpan* lastMarked() const;
    bool loopContains(const SkOpAngle& ) const;
    int loopCount() const;
    void markStops();
    bool merge(SkOpAngle* );

    SkOpAngle* next() const {
        return fNext;
    }

    SkOpAngle* previous() const;

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

    bool small() const;

    int start() const {
        return fStart;
    }

    bool unorderable() const {
        return fUnorderable;
    }

    
#if DEBUG_SORT
    void debugLoop() const;  
#endif
#if DEBUG_ANGLE
    void debugSameAs(const SkOpAngle* compare) const;
#endif
    void dump() const;
    void dumpLoop() const;
    void dumpTo(const SkOpSegment* fromSeg, const SkOpAngle* ) const;

#if DEBUG_ANGLE
    int debugID() const { return fID; }

    void setID(int id) {
        fID = id;
    }
#else
    int debugID() const { return 0; }
#endif

#if DEBUG_VALIDATE
    void debugValidateLoop() const;
#endif

private:
    bool after(const SkOpAngle* test) const;
    int allOnOneSide(const SkOpAngle& test) const;
    bool calcSlop(double x, double y, double rx, double ry, bool* result) const;
    bool checkCrossesZero() const;
    bool checkParallel(const SkOpAngle& ) const;
    bool computeSector();
    int convexHullOverlaps(const SkOpAngle& ) const;
    double distEndRatio(double dist) const;
    int findSector(SkPath::Verb verb, double x, double y) const;
    bool endsIntersect(const SkOpAngle& ) const;
    double midT() const;
    bool oppositePlanes(const SkOpAngle& rh) const;
    bool orderable(const SkOpAngle& rh) const;  
    bool overlap(const SkOpAngle& test) const;
    void setCurveHullSweep();
    void setSector();
    void setSpans();
    bool tangentsDiverge(const SkOpAngle& rh, double s0xt0) const;

    SkDCubic fCurvePart; 
    double fSide;
    SkLineParameters fTangentHalf;  
    const SkOpSegment* fSegment;
    SkOpAngle* fNext;
    SkOpSpan* fLastMarked;
    SkDVector fSweep[2];
    int fStart;
    int fEnd;
    int fComputedEnd;
    int fSectorMask;
    int8_t fSectorStart;  
    int8_t fSectorEnd;
    bool fIsCurve;
    bool fStop; 
    mutable bool fUnorderable;  
    bool fUnorderedSweep;  
    bool fComputeSector;
    bool fComputedSector;

#if DEBUG_ANGLE
    int fID;
#endif
#if DEBUG_VALIDATE
    void debugValidateNext() const;  
#else
    void debugValidateNext() const {}
#endif
    void dumpOne(bool showFunc) const;  
    void dumpPartials() const;  
    friend class PathOpsAngleTester;
};

class SkOpAngleSet {
public:
    SkOpAngleSet();
    ~SkOpAngleSet();
    SkOpAngle& push_back();
    void reset();
private:
    void dump() const;  
#if DEBUG_ANGLE
    int fCount;
#endif
    SkChunkAlloc* fAngles;
};

#endif

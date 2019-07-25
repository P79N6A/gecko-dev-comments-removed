






#ifndef SkBoundaryPatch_DEFINED
#define SkBoundaryPatch_DEFINED

#include "SkPoint.h"
#include "SkRefCnt.h"

class SkBoundary : public SkRefCnt {
public:
    
    enum Edge {
        kTop    = 0,
        kRight  = 1,
        kBottom = 2,
        kLeft   = 3
    };
    
    virtual SkPoint eval(Edge, SkScalar unitInterval) = 0;
};

class SkBoundaryPatch {
public:
    SkBoundaryPatch();
    ~SkBoundaryPatch();

    SkBoundary* getBoundary() const { return fBoundary; }
    SkBoundary* setBoundary(SkBoundary*);

    SkPoint eval(SkScalar unitU, SkScalar unitV);
    bool evalPatch(SkPoint verts[], int rows, int cols);

private:
    SkBoundary* fBoundary;
};



class SkLineBoundary : public SkBoundary {
public:
    SkPoint fPts[4];
    
    
    virtual SkPoint eval(Edge, SkScalar);
};

class SkCubicBoundary : public SkBoundary {
public:
    
    SkPoint fPts[13];
    
    
    virtual SkPoint eval(Edge, SkScalar);
};

#endif


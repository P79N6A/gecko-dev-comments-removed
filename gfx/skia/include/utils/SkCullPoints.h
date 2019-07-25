








#ifndef SkCullPoints_DEFINED
#define SkCullPoints_DEFINED

#include "SkRect.h"

class SkCullPoints {
public:
    SkCullPoints();
    SkCullPoints(const SkIRect& r);
    
    void    reset(const SkIRect& r);
    
    

    void    moveTo(int x, int y);
    
    enum LineToResult {
        kNo_Result,             
        kLineTo_Result,         
        kMoveToLineTo_Result    
    };
    

    LineToResult lineTo(int x, int y, SkIPoint pts[2]);

private:
    SkIRect      fR;             
    SkIPoint     fAsQuad[4];     
    SkIPoint     fPrevPt;        
    LineToResult fPrevResult;   
    
    bool sect_test(int x0, int y0, int x1, int y1) const;
};



class SkPath;






class SkCullPointsPath {
public:
    SkCullPointsPath();
    SkCullPointsPath(const SkIRect& r, SkPath* dst);

    void reset(const SkIRect& r, SkPath* dst);
    
    void    moveTo(int x, int y);
    void    lineTo(int x, int y);

private:
    SkCullPoints    fCP;
    SkPath*         fPath;
};

#endif

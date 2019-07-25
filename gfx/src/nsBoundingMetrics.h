




































#ifndef __nsBoundingMetrics_h
#define __nsBoundingMetrics_h

#include "nsCoord.h"







#ifdef MOZ_MATHML
struct nsBoundingMetrics {

    
    

    
    
    
    
    
    
    

    
    
    
    

    
    

    nscoord leftBearing;
    


    nscoord rightBearing;
    




    nscoord ascent;
    


    nscoord descent;
    




    nscoord width;
    




    nsBoundingMetrics() : leftBearing(0), rightBearing(0),
                          ascent(0), descent(0), width(0)
    {}

    void
    operator += (const nsBoundingMetrics& bm) {
        if (ascent + descent == 0 && rightBearing - leftBearing == 0) {
            ascent = bm.ascent;
            descent = bm.descent;
            leftBearing = width + bm.leftBearing;
            rightBearing = width + bm.rightBearing;
        }
        else {
            if (ascent < bm.ascent) ascent = bm.ascent;
            if (descent < bm.descent) descent = bm.descent;
            leftBearing = PR_MIN(leftBearing, width + bm.leftBearing);
            rightBearing = PR_MAX(rightBearing, width + bm.rightBearing);
        }
        width += bm.width;
    }
};
#endif

#endif

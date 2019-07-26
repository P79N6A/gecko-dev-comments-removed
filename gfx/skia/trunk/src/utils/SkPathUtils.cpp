







#include "SkPathUtils.h"

#include "SkPath.h"
#include "SkPathOps.h" 
#include "SkRegion.h"

typedef void (*line2path)(SkPath*, const char*, int, int);
#define SQRT_2 1.41421356237f
#define ON  0xFF000000 // black pixel
#define OFF 0x00000000 // transparent pixel













static int GetBit( const char* buffer, int x ) {
    int byte = x >> 3;
    int bit = x & 7;

    return buffer[byte] & (128 >> bit);
}


























static void Line2path_span(SkPath* path, const char* line,
                           int lineIdx, int width) {
    bool inRun = 0;
    int start = 1;

    for (int i = 0; i < width; ++i) {
        int curPixel = GetBit(line,i);

        if ( (curPixel!=0) != inRun ) { 
            if (curPixel) { 
                inRun = 1;
                start = i; 
            }else { 
                inRun = 0;
                path->addRect(SkRect::MakeXYWH(SkIntToScalar(start), SkIntToScalar(lineIdx),
                                               SkIntToScalar(i-start), SK_Scalar1),
                              SkPath::kCW_Direction);
            }
        }
    }

    if (inRun==1) { 
        int end = 0;
        if ( GetBit(line,width-1) ) ++end;
        path->addRect(SkRect::MakeXYWH(SkIntToScalar(start), SkIntToScalar(lineIdx),
                                       SkIntToScalar(width - 1 + end - start), SK_Scalar1),
                      SkPath::kCW_Direction);
    } else if ( GetBit(line, width - 1) ) { 
        path->addRect(SkRect::MakeXYWH(width - SK_Scalar1, SkIntToScalar(lineIdx),
                                       SK_Scalar1, SK_Scalar1),
                      SkPath::kCW_Direction);
    }
}

void SkPathUtils::BitsToPath_Path(SkPath* path,
                        const char* bitmap,
                        int w, int h, int stride) {
    
    for (int i = 0; i < h; ++i) {
        
        
        Line2path_span(path, &bitmap[i*stride], i, w);
    }
    Simplify(*path, path); 
}

void SkPathUtils::BitsToPath_Region(SkPath* path,
                               const char* bitmap,
                               int w, int h, int stride) {
    SkRegion region;

    
    for (int y = 0; y < h; ++y){
        bool inRun = 0;
        int start = 1;
        const char* line = &bitmap[y * stride];

        
        for (int i = 0; i < w; ++i) {
            int curPixel = GetBit(line,i);

            if ( (curPixel!=0) != inRun ) { 
                if (curPixel) { 
                    inRun = 1;
                    start = i; 
                }else { 
                    inRun = 0;
                    
                    region.op(SkIRect::MakeXYWH(start, y, i-start, 1),
                              SkRegion::kUnion_Op );
                }
            }
        }
        if (inRun==1) { 
            int end = 0;
            if ( GetBit(line,w-1) ) ++end;
            
            region.op(SkIRect::MakeXYWH(start, y, w-1-start+end, 1),
                      SkRegion::kUnion_Op );

        } else if ( GetBit(line,w-1) ) { 
            
            region.op(SkIRect::MakeXYWH(w-1, y, 1, 1),
                      SkRegion::kUnion_Op );
        }
    }
    
    region.getBoundaryPath(path);
}

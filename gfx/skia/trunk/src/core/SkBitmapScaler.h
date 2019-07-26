






#ifndef SkBitmapScaler_DEFINED
#define SkBitmapScaler_DEFINED

#include "SkBitmap.h"
#include "SkConvolver.h"






class SK_API SkBitmapScaler {
public:
    enum ResizeMethod {
        
        
        
        
        
        
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        RESIZE_GOOD,

        
        
        
        
        
        
        
        
        RESIZE_BETTER,

        
        RESIZE_BEST,

        
        
        

        
        
        
        
        
        RESIZE_BOX,
        RESIZE_TRIANGLE,
        RESIZE_LANCZOS3,
        RESIZE_HAMMING,
        RESIZE_MITCHELL,

        
        RESIZE_FIRST_QUALITY_METHOD = RESIZE_GOOD,
        RESIZE_LAST_QUALITY_METHOD = RESIZE_BEST,
        RESIZE_FIRST_ALGORITHM_METHOD = RESIZE_BOX,
        RESIZE_LAST_ALGORITHM_METHOD = RESIZE_MITCHELL,
    };

    
    
    
    
    
    
    
    
    static bool Resize(SkBitmap* result,
                       const SkBitmap& source,
                       ResizeMethod method,
                       int dest_width, int dest_height,
                       const SkIRect& dest_subset,
                       const SkConvolutionProcs&,
                       SkBitmap::Allocator* allocator = NULL);

    
    
    static bool Resize(SkBitmap* result,
                       const SkBitmap& source,
                       ResizeMethod method,
                       int dest_width, int dest_height,
                       const SkConvolutionProcs&,
                       SkBitmap::Allocator* allocator = NULL);
};

#endif

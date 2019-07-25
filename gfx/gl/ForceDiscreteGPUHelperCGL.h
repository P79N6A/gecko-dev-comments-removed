




#ifndef ForceDiscreteGPUHelperCGL_h_
#define ForceDiscreteGPUHelperCGL_h_

#include <OpenGL/OpenGL.h>





class ForceDiscreteGPUHelperCGL
{
    CGLPixelFormatObj mPixelFormatObj;

public:
    ForceDiscreteGPUHelperCGL()
    {
        
        
        CGLPixelFormatAttribute attribs[1];
        attribs[0] = static_cast<CGLPixelFormatAttribute>(0);
        GLint num_pixel_formats = 0;
        CGLChoosePixelFormat(attribs, &mPixelFormatObj, &num_pixel_formats);
    }

    ~ForceDiscreteGPUHelperCGL()
    {
        CGLReleasePixelFormat(mPixelFormatObj);
    }
};

#endif 






































#include "gfxRect.h"
#include "nsRect.h"

#ifndef GFX_THEBES_UTILS_H
#define GFX_THEBES_UTILS_H

class THEBES_API gfxThebesUtils
{
public:
    


    static nsIntRect GfxRectToIntRect(const gfxRect& aIn);
};

#endif 






































#ifndef _GFXALPHARECOVERY_H_
#define _GFXALPHARECOVERY_H_

#include "gfxContext.h"
#include "gfxImageSurface.h"

class THEBES_API gfxAlphaRecovery {
public:
    



    static already_AddRefed<gfxImageSurface> RecoverAlpha (gfxImageSurface *blackSurface,
                                                           gfxImageSurface *whiteSurface,
                                                           gfxIntSize dimensions);
};

#endif 

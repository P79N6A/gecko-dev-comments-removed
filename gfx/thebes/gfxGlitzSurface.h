




































#ifndef GFX_GLITZSURFACE_H
#define GFX_GLITZSURFACE_H

#include "gfxASurface.h"

#include <cairo-glitz.h>




class THEBES_API gfxGlitzSurface : public gfxASurface {
public:
    gfxGlitzSurface(glitz_drawable_t *drawable,
                    glitz_surface_t *glitzSurface,
                    PRBool takeOwnership = PR_FALSE);

    virtual ~gfxGlitzSurface();

    


    void SwapBuffers();

    unsigned long Width();
    unsigned long Height();

    glitz_surface_t* GlitzSurface() { return mGlitzSurface; }
    glitz_drawable_t* GlitzDrawable() { return mGlitzDrawable; }

protected:
    glitz_drawable_t *mGlitzDrawable;
    glitz_surface_t *mGlitzSurface;
    PRBool mOwnsSurface;
};

#endif 

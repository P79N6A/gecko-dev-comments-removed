








#ifndef SkGLCanvas_DEFINED
#define SkGLCanvas_DEFINED

#include "SkCanvas.h"



class SkGLCanvas : public SkCanvas {
public:
    SkGLCanvas();

    static size_t GetTextureCacheMaxCount();
    static void SetTextureCacheMaxCount(size_t count);

    static size_t GetTextureCacheMaxSize();
    static void SetTextureCacheMaxSize(size_t size);

    static void DeleteAllTextures();

    static void AbandonAllTextures();
};

#endif

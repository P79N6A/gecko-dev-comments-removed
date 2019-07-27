






#ifndef GrYUVtoRGBEffect_DEFINED
#define GrYUVtoRGBEffect_DEFINED

class GrEffect;
class GrTexture;

namespace GrYUVtoRGBEffect {
    


    GrEffect* Create(GrTexture* yTexture, GrTexture* uTexture, GrTexture* vTexture);
};

#endif

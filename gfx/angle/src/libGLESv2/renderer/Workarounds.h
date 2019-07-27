







#ifndef LIBGLESV2_RENDERER_WORKAROUNDS_H_
#define LIBGLESV2_RENDERER_WORKAROUNDS_H_





namespace rx
{

enum D3DWorkaroundType
{
    ANGLE_D3D_WORKAROUND_NONE,
    ANGLE_D3D_WORKAROUND_SKIP_OPTIMIZATION,
    ANGLE_D3D_WORKAROUND_MAX_OPTIMIZATION
};

struct Workarounds
{
    Workarounds()
        : mrtPerfWorkaround(false),
          setDataFasterThanImageUpload(false)
    {}

    bool mrtPerfWorkaround;
    bool setDataFasterThanImageUpload;
};

}

#endif 

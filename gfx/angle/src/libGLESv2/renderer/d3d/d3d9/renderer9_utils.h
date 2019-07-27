








#ifndef LIBGLESV2_RENDERER_RENDERER9_UTILS_H
#define LIBGLESV2_RENDERER_RENDERER9_UTILS_H

#include "libGLESv2/angletypes.h"
#include "libGLESv2/Caps.h"

namespace rx
{

namespace gl_d3d9
{

D3DCMPFUNC ConvertComparison(GLenum comparison);
D3DCOLOR ConvertColor(gl::ColorF color);
D3DBLEND ConvertBlendFunc(GLenum blend);
D3DBLENDOP ConvertBlendOp(GLenum blendOp);
D3DSTENCILOP ConvertStencilOp(GLenum stencilOp);
D3DTEXTUREADDRESS ConvertTextureWrap(GLenum wrap);
D3DCULL ConvertCullMode(GLenum cullFace, GLenum frontFace);
D3DCUBEMAP_FACES ConvertCubeFace(GLenum cubeFace);
DWORD ConvertColorMask(bool red, bool green, bool blue, bool alpha);
D3DTEXTUREFILTERTYPE ConvertMagFilter(GLenum magFilter, float maxAnisotropy);
void ConvertMinFilter(GLenum minFilter, D3DTEXTUREFILTERTYPE *d3dMinFilter, D3DTEXTUREFILTERTYPE *d3dMipFilter, float maxAnisotropy);

}

namespace d3d9_gl
{

gl::Caps GenerateCaps(IDirect3D9 *d3d9, IDirect3DDevice9 *device, D3DDEVTYPE deviceType, UINT adapter);

}

namespace d3d9
{

inline bool isDeviceLostError(HRESULT errorCode)
{
    switch (errorCode)
    {
      case D3DERR_DRIVERINTERNALERROR:
      case D3DERR_DEVICELOST:
      case D3DERR_DEVICEHUNG:
      case D3DERR_DEVICEREMOVED:
        return true;
      default:
        return false;
    }
}

}

}

#endif 

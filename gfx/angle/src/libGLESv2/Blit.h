







#ifndef LIBGLESV2_BLIT_H_
#define LIBGLESV2_BLIT_H_

#include <map>

#define GL_APICALL
#include <GLES2/gl2.h>

#include <d3d9.h>

#include "common/angleutils.h"

namespace gl
{
class Context;

class Blit
{
  public:
    explicit Blit(Context *context);
    ~Blit();

    
    
    bool copy(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);

    
    
    
    bool formatConvert(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);

    
    
    bool boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest);

  private:
    Context *mContext;

    IDirect3DVertexBuffer9 *mQuadVertexBuffer;
    IDirect3DVertexDeclaration9 *mQuadVertexDeclaration;

    void initGeometry();

    bool setFormatConvertShaders(GLenum destFormat);

    IDirect3DTexture9 *copySurfaceToTexture(IDirect3DSurface9 *surface, const RECT &sourceRect);
    void setViewport(const RECT &sourceRect, GLint xoffset, GLint yoffset);
    void setCommonBlitState();
    RECT getSurfaceRect(IDirect3DSurface9 *surface) const;

    
    enum ShaderId
    {
        SHADER_VS_STANDARD,
        SHADER_VS_FLIPY,
        SHADER_PS_PASSTHROUGH,
        SHADER_PS_LUMINANCE,
        SHADER_PS_COMPONENTMASK,
        SHADER_COUNT
    };

    static const char * const mShaderSource[];

    
    IUnknown *mCompiledShaders[SHADER_COUNT];

    template <class D3DShaderType>
    bool setShader(ShaderId source, const char *profile,
                   HRESULT (WINAPI IDirect3DDevice9::*createShader)(const DWORD *, D3DShaderType **),
                   HRESULT (WINAPI IDirect3DDevice9::*setShader)(D3DShaderType*));

    bool setVertexShader(ShaderId shader);
    bool setPixelShader(ShaderId shader);
    void render();

    void saveState();
    void restoreState();
    IDirect3DStateBlock9 *mSavedStateBlock;
    IDirect3DSurface9 *mSavedRenderTarget;
    IDirect3DSurface9 *mSavedDepthStencil;

    DISALLOW_COPY_AND_ASSIGN(Blit);
};
}

#endif   

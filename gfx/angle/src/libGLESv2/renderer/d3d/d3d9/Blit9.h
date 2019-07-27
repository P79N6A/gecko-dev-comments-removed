







#ifndef LIBGLESV2_BLIT9_H_
#define LIBGLESV2_BLIT9_H_

#include "common/angleutils.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer9;
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;

class Blit9
{
  public:
    explicit Blit9(Renderer9 *renderer);
    ~Blit9();

    
    
    bool copy(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level);
    bool copy(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level);

    
    
    
    bool formatConvert(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);

    
    
    bool boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest);

  private:
    rx::Renderer9 *mRenderer;

    IDirect3DVertexBuffer9 *mQuadVertexBuffer;
    IDirect3DVertexDeclaration9 *mQuadVertexDeclaration;

    void initGeometry();

    bool setFormatConvertShaders(GLenum destFormat);

    bool copy(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);
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

    
    IUnknown *mCompiledShaders[SHADER_COUNT];

    template <class D3DShaderType>
    bool setShader(ShaderId source, const char *profile,
                   D3DShaderType *(Renderer9::*createShader)(const DWORD *, size_t length),
                   HRESULT (WINAPI IDirect3DDevice9::*setShader)(D3DShaderType*));

    bool setVertexShader(ShaderId shader);
    bool setPixelShader(ShaderId shader);
    void render();

    void saveState();
    void restoreState();
    IDirect3DStateBlock9 *mSavedStateBlock;
    IDirect3DSurface9 *mSavedRenderTarget;
    IDirect3DSurface9 *mSavedDepthStencil;

    DISALLOW_COPY_AND_ASSIGN(Blit9);
};
}

#endif   

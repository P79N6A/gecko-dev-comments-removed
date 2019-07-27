







#ifndef LIBGLESV2_BLIT9_H_
#define LIBGLESV2_BLIT9_H_

#include "common/angleutils.h"
#include "libGLESv2/Error.h"

#include <GLES2/gl2.h>

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer9;
class TextureStorage;

class Blit9
{
  public:
    explicit Blit9(Renderer9 *renderer);
    ~Blit9();

    gl::Error initialize();

    
    
    gl::Error copy2D(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorage *storage, GLint level);
    gl::Error copyCube(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, TextureStorage *storage, GLenum target, GLint level);

    
    
    
    gl::Error formatConvert(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);

    
    
    gl::Error boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest);

  private:
    rx::Renderer9 *mRenderer;

    bool mGeometryLoaded;
    IDirect3DVertexBuffer9 *mQuadVertexBuffer;
    IDirect3DVertexDeclaration9 *mQuadVertexDeclaration;

    gl::Error setFormatConvertShaders(GLenum destFormat);

    gl::Error copy(IDirect3DSurface9 *source, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset, IDirect3DSurface9 *dest);
    gl::Error copySurfaceToTexture(IDirect3DSurface9 *surface, const RECT &sourceRect, IDirect3DTexture9 **outTexture);
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
    gl::Error setShader(ShaderId source, const char *profile,
                        gl::Error (Renderer9::*createShader)(const DWORD *, size_t length, D3DShaderType **outShader),
                        HRESULT (WINAPI IDirect3DDevice9::*setShader)(D3DShaderType*));

    gl::Error setVertexShader(ShaderId shader);
    gl::Error setPixelShader(ShaderId shader);
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

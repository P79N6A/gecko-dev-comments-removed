








#ifndef LIBGLESV2_RENDERER_FORMATUTILS11_H_
#define LIBGLESV2_RENDERER_FORMATUTILS11_H_

#include "libGLESv2/formatutils.h"

namespace rx
{

class Renderer;

namespace d3d11
{

typedef std::set<DXGI_FORMAT> DXGIFormatSet;

MipGenerationFunction GetMipGenerationFunction(DXGI_FORMAT format);
LoadImageFunction GetImageLoadFunction(GLenum internalFormat, GLenum type);

GLuint GetFormatPixelBytes(DXGI_FORMAT format);
GLuint GetBlockWidth(DXGI_FORMAT format);
GLuint GetBlockHeight(DXGI_FORMAT format);
GLenum GetComponentType(DXGI_FORMAT format);

GLuint GetDepthBits(DXGI_FORMAT format);
GLuint GetDepthOffset(DXGI_FORMAT format);
GLuint GetStencilBits(DXGI_FORMAT format);
GLuint GetStencilOffset(DXGI_FORMAT format);

void MakeValidSize(bool isImage, DXGI_FORMAT format, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset);

const DXGIFormatSet &GetAllUsedDXGIFormats();

ColorReadFunction GetColorReadFunction(DXGI_FORMAT format);
ColorCopyFunction GetFastCopyFunction(DXGI_FORMAT sourceFormat, GLenum destFormat, GLenum destType);

}

namespace gl_d3d11
{

DXGI_FORMAT GetTexFormat(GLenum internalFormat);
DXGI_FORMAT GetSRVFormat(GLenum internalFormat);
DXGI_FORMAT GetRTVFormat(GLenum internalFormat);
DXGI_FORMAT GetDSVFormat(GLenum internalFormat);
DXGI_FORMAT GetRenderableFormat(GLenum internalFormat);

DXGI_FORMAT GetSwizzleTexFormat(GLint internalFormat);
DXGI_FORMAT GetSwizzleSRVFormat(GLint internalFormat);
DXGI_FORMAT GetSwizzleRTVFormat(GLint internalFormat);

bool RequiresTextureDataInitialization(GLint internalFormat);
InitializeTextureDataFunction GetTextureDataInitializationFunction(GLint internalFormat);

VertexCopyFunction GetVertexCopyFunction(const gl::VertexFormat &vertexFormat);
size_t GetVertexElementSize(const gl::VertexFormat &vertexFormat);
VertexConversionType GetVertexConversionType(const gl::VertexFormat &vertexFormat);
DXGI_FORMAT GetNativeVertexFormat(const gl::VertexFormat &vertexFormat);

}

namespace d3d11_gl
{

GLenum GetInternalFormat(DXGI_FORMAT format);

}

}

#endif 

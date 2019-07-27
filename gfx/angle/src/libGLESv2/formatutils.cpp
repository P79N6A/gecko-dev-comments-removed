#include "precompiled.h"








#include "common/mathutil.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/imageformats.h"
#include "libGLESv2/renderer/copyimage.h"

namespace gl
{





struct FormatTypeInfo
{
    GLenum mInternalFormat;
    ColorWriteFunction mColorWriteFunction;

    FormatTypeInfo(GLenum internalFormat, ColorWriteFunction writeFunc)
        : mInternalFormat(internalFormat), mColorWriteFunction(writeFunc)
    { }
};

typedef std::pair<GLenum, GLenum> FormatTypePair;
typedef std::pair<FormatTypePair, FormatTypeInfo> FormatPair;
typedef std::map<FormatTypePair, FormatTypeInfo> FormatMap;


static inline void InsertFormatMapping(FormatMap *map, GLenum format, GLenum type, GLenum internalFormat, ColorWriteFunction writeFunc)
{
    map->insert(FormatPair(FormatTypePair(format, type), FormatTypeInfo(internalFormat, writeFunc)));
}

FormatMap BuildFormatMap()
{
    FormatMap map;

    using namespace rx;

    
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_BYTE,                  GL_RGBA8,                  WriteColor<R8G8B8A8, GLfloat>     );
    InsertFormatMapping(&map, GL_RGBA,               GL_BYTE,                           GL_RGBA8_SNORM,            WriteColor<R8G8B8A8S, GLfloat>    );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_SHORT_4_4_4_4,         GL_RGBA4,                  WriteColor<R4G4B4A4, GLfloat>     );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_SHORT_5_5_5_1,         GL_RGB5_A1,                WriteColor<R5G5B5A1, GLfloat>     );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_INT_2_10_10_10_REV,    GL_RGB10_A2,               WriteColor<R10G10B10A2, GLfloat>  );
    InsertFormatMapping(&map, GL_RGBA,               GL_FLOAT,                          GL_RGBA32F,                WriteColor<R32G32B32A32F, GLfloat>);
    InsertFormatMapping(&map, GL_RGBA,               GL_HALF_FLOAT,                     GL_RGBA16F,                WriteColor<R16G16B16A16F, GLfloat>);
    InsertFormatMapping(&map, GL_RGBA,               GL_HALF_FLOAT_OES,                 GL_RGBA16F,                WriteColor<R16G16B16A16F, GLfloat>);

    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_BYTE,                  GL_RGBA8UI,                WriteColor<R8G8B8A8, GLuint>      );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_BYTE,                           GL_RGBA8I,                 WriteColor<R8G8B8A8S, GLint>      );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_SHORT,                 GL_RGBA16UI,               WriteColor<R16G16B16A16, GLuint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_SHORT,                          GL_RGBA16I,                WriteColor<R16G16B16A16S, GLint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_INT,                   GL_RGBA32UI,               WriteColor<R32G32B32A32, GLuint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_INT,                            GL_RGBA32I,                WriteColor<R32G32B32A32S, GLint>  );
    InsertFormatMapping(&map, GL_RGBA_INTEGER,       GL_UNSIGNED_INT_2_10_10_10_REV,    GL_RGB10_A2UI,             WriteColor<R10G10B10A2, GLuint>   );

    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_BYTE,                  GL_RGB8,                   WriteColor<R8G8B8, GLfloat>       );
    InsertFormatMapping(&map, GL_RGB,                GL_BYTE,                           GL_RGB8_SNORM,             WriteColor<R8G8B8S, GLfloat>      );
    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_SHORT_5_6_5,           GL_RGB565,                 WriteColor<R5G6B5, GLfloat>       );
    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_INT_10F_11F_11F_REV,   GL_R11F_G11F_B10F,         WriteColor<R11G11B10F, GLfloat>   );
    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_INT_5_9_9_9_REV,       GL_RGB9_E5,                WriteColor<R9G9B9E5, GLfloat>     );
    InsertFormatMapping(&map, GL_RGB,                GL_FLOAT,                          GL_RGB32F,                 WriteColor<R32G32B32F, GLfloat>   );
    InsertFormatMapping(&map, GL_RGB,                GL_HALF_FLOAT,                     GL_RGB16F,                 WriteColor<R16G16B16F, GLfloat>   );
    InsertFormatMapping(&map, GL_RGB,                GL_HALF_FLOAT_OES,                 GL_RGB16F,                 WriteColor<R16G16B16F, GLfloat>   );

    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_UNSIGNED_BYTE,                  GL_RGB8UI,                 WriteColor<R8G8B8, GLuint>        );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_BYTE,                           GL_RGB8I,                  WriteColor<R8G8B8S, GLint>        );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_UNSIGNED_SHORT,                 GL_RGB16UI,                WriteColor<R16G16B16, GLuint>     );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_SHORT,                          GL_RGB16I,                 WriteColor<R16G16B16S, GLint>     );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_UNSIGNED_INT,                   GL_RGB32UI,                WriteColor<R32G32B32, GLuint>     );
    InsertFormatMapping(&map, GL_RGB_INTEGER,        GL_INT,                            GL_RGB32I,                 WriteColor<R32G32B32S, GLint>     );

    InsertFormatMapping(&map, GL_RG,                 GL_UNSIGNED_BYTE,                  GL_RG8,                    WriteColor<R8G8, GLfloat>         );
    InsertFormatMapping(&map, GL_RG,                 GL_BYTE,                           GL_RG8_SNORM,              WriteColor<R8G8S, GLfloat>        );
    InsertFormatMapping(&map, GL_RG,                 GL_FLOAT,                          GL_RG32F,                  WriteColor<R32G32F, GLfloat>      );
    InsertFormatMapping(&map, GL_RG,                 GL_HALF_FLOAT,                     GL_RG16F,                  WriteColor<R16G16F, GLfloat>      );
    InsertFormatMapping(&map, GL_RG,                 GL_HALF_FLOAT_OES,                 GL_RG16F,                  WriteColor<R16G16F, GLfloat>      );

    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_UNSIGNED_BYTE,                  GL_RG8UI,                  WriteColor<R8G8, GLuint>          );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_BYTE,                           GL_RG8I,                   WriteColor<R8G8S, GLint>          );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_UNSIGNED_SHORT,                 GL_RG16UI,                 WriteColor<R16G16, GLuint>        );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_SHORT,                          GL_RG16I,                  WriteColor<R16G16S, GLint>        );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_UNSIGNED_INT,                   GL_RG32UI,                 WriteColor<R32G32, GLuint>        );
    InsertFormatMapping(&map, GL_RG_INTEGER,         GL_INT,                            GL_RG32I,                  WriteColor<R32G32S, GLint>        );

    InsertFormatMapping(&map, GL_RED,                GL_UNSIGNED_BYTE,                  GL_R8,                     WriteColor<R8, GLfloat>           );
    InsertFormatMapping(&map, GL_RED,                GL_BYTE,                           GL_R8_SNORM,               WriteColor<R8S, GLfloat>          );
    InsertFormatMapping(&map, GL_RED,                GL_FLOAT,                          GL_R32F,                   WriteColor<R32F, GLfloat>         );
    InsertFormatMapping(&map, GL_RED,                GL_HALF_FLOAT,                     GL_R16F,                   WriteColor<R16F, GLfloat>         );
    InsertFormatMapping(&map, GL_RED,                GL_HALF_FLOAT_OES,                 GL_R16F,                   WriteColor<R16F, GLfloat>         );

    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_UNSIGNED_BYTE,                  GL_R8UI,                   WriteColor<R8, GLuint>            );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_BYTE,                           GL_R8I,                    WriteColor<R8S, GLint>            );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_UNSIGNED_SHORT,                 GL_R16UI,                  WriteColor<R16, GLuint>           );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_SHORT,                          GL_R16I,                   WriteColor<R16S, GLint>           );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_UNSIGNED_INT,                   GL_R32UI,                  WriteColor<R32, GLuint>           );
    InsertFormatMapping(&map, GL_RED_INTEGER,        GL_INT,                            GL_R32I,                   WriteColor<R32S, GLint>           );

    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_ALPHA8_EXT,  WriteColor<L8A8, GLfloat>         );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_EXT,         WriteColor<L8, GLfloat>           );
    InsertFormatMapping(&map, GL_ALPHA,              GL_UNSIGNED_BYTE,                  GL_ALPHA8_EXT,             WriteColor<A8, GLfloat>           );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_FLOAT,                          GL_LUMINANCE_ALPHA32F_EXT, WriteColor<L32A32F, GLfloat>      );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_FLOAT,                          GL_LUMINANCE32F_EXT,       WriteColor<L32F, GLfloat>         );
    InsertFormatMapping(&map, GL_ALPHA,              GL_FLOAT,                          GL_ALPHA32F_EXT,           WriteColor<A32F, GLfloat>         );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT,                     GL_LUMINANCE_ALPHA16F_EXT, WriteColor<L16A16F, GLfloat>      );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT_OES,                 GL_LUMINANCE_ALPHA16F_EXT, WriteColor<L16A16F, GLfloat>      );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_HALF_FLOAT,                     GL_LUMINANCE16F_EXT,       WriteColor<L16F, GLfloat>         );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_HALF_FLOAT_OES,                 GL_LUMINANCE16F_EXT,       WriteColor<L16F, GLfloat>         );
    InsertFormatMapping(&map, GL_ALPHA,              GL_HALF_FLOAT,                     GL_ALPHA16F_EXT,           WriteColor<A16F, GLfloat>         );
    InsertFormatMapping(&map, GL_ALPHA,              GL_HALF_FLOAT_OES,                 GL_ALPHA16F_EXT,           WriteColor<A16F, GLfloat>         );

    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_BYTE,                  GL_BGRA8_EXT,              WriteColor<B8G8R8A8, GLfloat>     );
    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, GL_BGRA4_ANGLEX,           WriteColor<B4G4R4A4, GLfloat>     );
    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, GL_BGR5_A1_ANGLEX,         WriteColor<B5G5R5A1, GLfloat>     );

    InsertFormatMapping(&map, GL_SRGB_EXT,           GL_UNSIGNED_BYTE,                  GL_SRGB8,                  WriteColor<R8G8B8, GLfloat>       );
    InsertFormatMapping(&map, GL_SRGB_ALPHA_EXT,     GL_UNSIGNED_BYTE,                  GL_SRGB8_ALPHA8,           WriteColor<R8G8B8A8, GLfloat>     );

    InsertFormatMapping(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    NULL                     );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   NULL                     );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, NULL                     );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, GL_UNSIGNED_BYTE,     GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, NULL                     );

    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_UNSIGNED_SHORT,                 GL_DEPTH_COMPONENT16,      NULL                              );
    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_UNSIGNED_INT,                   GL_DEPTH_COMPONENT32_OES,  NULL                              );
    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_FLOAT,                          GL_DEPTH_COMPONENT32F,     NULL                              );

    InsertFormatMapping(&map, GL_STENCIL,            GL_UNSIGNED_BYTE,                  GL_STENCIL_INDEX8,         NULL                              );

    InsertFormatMapping(&map, GL_DEPTH_STENCIL,      GL_UNSIGNED_INT_24_8,              GL_DEPTH24_STENCIL8,       NULL                              );
    InsertFormatMapping(&map, GL_DEPTH_STENCIL,      GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH32F_STENCIL8,      NULL                              );

    return map;
}

static const FormatMap &GetFormatMap()
{
     static const FormatMap formatMap = BuildFormatMap();
     return formatMap;
}

struct FormatInfo
{
    GLenum mInternalformat;
    GLenum mFormat;
    GLenum mType;

    FormatInfo(GLenum internalformat, GLenum format, GLenum type)
        : mInternalformat(internalformat), mFormat(format), mType(type) { }

    bool operator<(const FormatInfo& other) const
    {
        return memcmp(this, &other, sizeof(FormatInfo)) < 0;
    }
};


typedef std::set<FormatInfo> ES3FormatSet;

ES3FormatSet BuildES3FormatSet()
{
    ES3FormatSet set;

    

    
    
    set.insert(FormatInfo(GL_RGBA8,              GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA4,              GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_SRGB8_ALPHA8,       GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA8_SNORM,        GL_RGBA,            GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGBA4,              GL_RGBA,            GL_UNSIGNED_SHORT_4_4_4_4        ));
    set.insert(FormatInfo(GL_RGB10_A2,           GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV   ));
    set.insert(FormatInfo(GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV   ));
    set.insert(FormatInfo(GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1        ));
    set.insert(FormatInfo(GL_RGBA16F,            GL_RGBA,            GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RGBA16F,            GL_RGBA,            GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_RGBA32F,            GL_RGBA,            GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGBA16F,            GL_RGBA,            GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGBA8UI,            GL_RGBA_INTEGER,    GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA8I,             GL_RGBA_INTEGER,    GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGBA16UI,           GL_RGBA_INTEGER,    GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_RGBA16I,            GL_RGBA_INTEGER,    GL_SHORT                         ));
    set.insert(FormatInfo(GL_RGBA32UI,           GL_RGBA_INTEGER,    GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_RGBA32I,            GL_RGBA_INTEGER,    GL_INT                           ));
    set.insert(FormatInfo(GL_RGB10_A2UI,         GL_RGBA_INTEGER,    GL_UNSIGNED_INT_2_10_10_10_REV   ));
    set.insert(FormatInfo(GL_RGB8,               GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB565,             GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_SRGB8,              GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB8_SNORM,         GL_RGB,             GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGB565,             GL_RGB,             GL_UNSIGNED_SHORT_5_6_5          ));
    set.insert(FormatInfo(GL_R11F_G11F_B10F,     GL_RGB,             GL_UNSIGNED_INT_10F_11F_11F_REV  ));
    set.insert(FormatInfo(GL_RGB9_E5,            GL_RGB,             GL_UNSIGNED_INT_5_9_9_9_REV      ));
    set.insert(FormatInfo(GL_RGB16F,             GL_RGB,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RGB16F,             GL_RGB,             GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_R11F_G11F_B10F,     GL_RGB,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_R11F_G11F_B10F,     GL_RGB,             GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_RGB9_E5,            GL_RGB,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RGB9_E5,            GL_RGB,             GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_RGB32F,             GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGB16F,             GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_R11F_G11F_B10F,     GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGB9_E5,            GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGB8UI,             GL_RGB_INTEGER,     GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB8I,              GL_RGB_INTEGER,     GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGB16UI,            GL_RGB_INTEGER,     GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_RGB16I,             GL_RGB_INTEGER,     GL_SHORT                         ));
    set.insert(FormatInfo(GL_RGB32UI,            GL_RGB_INTEGER,     GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_RGB32I,             GL_RGB_INTEGER,     GL_INT                           ));
    set.insert(FormatInfo(GL_RG8,                GL_RG,              GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RG8_SNORM,          GL_RG,              GL_BYTE                          ));
    set.insert(FormatInfo(GL_RG16F,              GL_RG,              GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RG16F,              GL_RG,              GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_RG32F,              GL_RG,              GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RG16F,              GL_RG,              GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RG8UI,              GL_RG_INTEGER,      GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RG8I,               GL_RG_INTEGER,      GL_BYTE                          ));
    set.insert(FormatInfo(GL_RG16UI,             GL_RG_INTEGER,      GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_RG16I,              GL_RG_INTEGER,      GL_SHORT                         ));
    set.insert(FormatInfo(GL_RG32UI,             GL_RG_INTEGER,      GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_RG32I,              GL_RG_INTEGER,      GL_INT                           ));
    set.insert(FormatInfo(GL_R8,                 GL_RED,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_R8_SNORM,           GL_RED,             GL_BYTE                          ));
    set.insert(FormatInfo(GL_R16F,               GL_RED,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_R16F,               GL_RED,             GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_R32F,               GL_RED,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_R16F,               GL_RED,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_R8UI,               GL_RED_INTEGER,     GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_R8I,                GL_RED_INTEGER,     GL_BYTE                          ));
    set.insert(FormatInfo(GL_R16UI,              GL_RED_INTEGER,     GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_R16I,               GL_RED_INTEGER,     GL_SHORT                         ));
    set.insert(FormatInfo(GL_R32UI,              GL_RED_INTEGER,     GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_R32I,               GL_RED_INTEGER,     GL_INT                           ));

    
    set.insert(FormatInfo(GL_RGBA,               GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA,               GL_RGBA,            GL_UNSIGNED_SHORT_4_4_4_4        ));
    set.insert(FormatInfo(GL_RGBA,               GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1        ));
    set.insert(FormatInfo(GL_RGB,                GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB,                GL_RGB,             GL_UNSIGNED_SHORT_5_6_5          ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_LUMINANCE,          GL_LUMINANCE,       GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_ALPHA,              GL_ALPHA,           GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_SRGB_ALPHA_EXT,     GL_SRGB_ALPHA_EXT,  GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_SRGB_EXT,           GL_SRGB_EXT,        GL_UNSIGNED_BYTE                 ));

    
    set.insert(FormatInfo(GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT                         ));
    set.insert(FormatInfo(GL_DEPTH24_STENCIL8,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8             ));
    set.insert(FormatInfo(GL_DEPTH32F_STENCIL8,  GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV));

    
    set.insert(FormatInfo(GL_SRGB8_ALPHA8_EXT,   GL_SRGB_ALPHA_EXT, GL_UNSIGNED_BYTE                  ));
    set.insert(FormatInfo(GL_SRGB8,              GL_SRGB_EXT,       GL_UNSIGNED_BYTE                  ));

    
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_FLOAT                         ));
    set.insert(FormatInfo(GL_LUMINANCE,          GL_LUMINANCE,       GL_FLOAT                         ));
    set.insert(FormatInfo(GL_ALPHA,              GL_ALPHA,           GL_FLOAT                         ));

    
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_LUMINANCE,          GL_LUMINANCE,       GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_LUMINANCE,          GL_LUMINANCE,       GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_ALPHA,              GL_ALPHA,           GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_ALPHA,              GL_ALPHA,           GL_HALF_FLOAT_OES                ));

    
    set.insert(FormatInfo(GL_BGRA_EXT,           GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 ));

    
    
    
    set.insert(FormatInfo(GL_ALPHA8_EXT,             GL_ALPHA,           GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_LUMINANCE8_EXT,         GL_LUMINANCE,       GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_LUMINANCE8_ALPHA8_EXT,  GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_ALPHA32F_EXT,           GL_ALPHA,           GL_FLOAT                         ));
    set.insert(FormatInfo(GL_LUMINANCE32F_EXT,       GL_LUMINANCE,       GL_FLOAT                         ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA32F_EXT, GL_LUMINANCE_ALPHA, GL_FLOAT                         ));
    set.insert(FormatInfo(GL_ALPHA16F_EXT,           GL_ALPHA,           GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_ALPHA16F_EXT,           GL_ALPHA,           GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_LUMINANCE16F_EXT,       GL_LUMINANCE,       GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_LUMINANCE16F_EXT,       GL_LUMINANCE,       GL_HALF_FLOAT_OES                ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA16F_EXT, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA16F_EXT, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT_OES                ));

    
    set.insert(FormatInfo(GL_BGRA8_EXT,              GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_BGRA4_ANGLEX,           GL_BGRA_EXT,        GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT));
    set.insert(FormatInfo(GL_BGRA4_ANGLEX,           GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_BGR5_A1_ANGLEX,         GL_BGRA_EXT,        GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT));
    set.insert(FormatInfo(GL_BGR5_A1_ANGLEX,         GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 ));

    
    set.insert(FormatInfo(GL_DEPTH_COMPONENT32_OES,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT_24_8_OES         ));

    
    
    
    
    set.insert(FormatInfo(GL_COMPRESSED_R11_EAC,                        GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_R11_EAC,                        GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SIGNED_R11_EAC,                 GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RG11_EAC,                       GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SIGNED_RG11_EAC,                GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGB8_ETC2,                      GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SRGB8_ETC2,                     GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE));


    
    set.insert(FormatInfo(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_UNSIGNED_BYTE));

    
    set.insert(FormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_UNSIGNED_BYTE));

    
    set.insert(FormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_UNSIGNED_BYTE));

    return set;
}

static const ES3FormatSet &GetES3FormatSet()
{
    static const ES3FormatSet es3FormatSet = BuildES3FormatSet();
    return es3FormatSet;
}


struct TypeInfo
{
    GLuint mTypeBytes;
    bool mSpecialInterpretation;

    TypeInfo()
        : mTypeBytes(0), mSpecialInterpretation(false) { }

    TypeInfo(GLuint typeBytes, bool specialInterpretation)
        : mTypeBytes(typeBytes), mSpecialInterpretation(specialInterpretation) { }

    bool operator<(const TypeInfo& other) const
    {
        return memcmp(this, &other, sizeof(TypeInfo)) < 0;
    }
};

typedef std::pair<GLenum, TypeInfo> TypeInfoPair;
typedef std::map<GLenum, TypeInfo> TypeInfoMap;

static TypeInfoMap BuildTypeInfoMap()
{
    TypeInfoMap map;

    map.insert(TypeInfoPair(GL_UNSIGNED_BYTE,                  TypeInfo( 1, false)));
    map.insert(TypeInfoPair(GL_BYTE,                           TypeInfo( 1, false)));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT,                 TypeInfo( 2, false)));
    map.insert(TypeInfoPair(GL_SHORT,                          TypeInfo( 2, false)));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT,                   TypeInfo( 4, false)));
    map.insert(TypeInfoPair(GL_INT,                            TypeInfo( 4, false)));
    map.insert(TypeInfoPair(GL_HALF_FLOAT,                     TypeInfo( 2, false)));
    map.insert(TypeInfoPair(GL_HALF_FLOAT_OES,                 TypeInfo( 2, false)));
    map.insert(TypeInfoPair(GL_FLOAT,                          TypeInfo( 4, false)));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_5_6_5,           TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_4_4_4_4,         TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_5_5_5_1,         TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_2_10_10_10_REV,    TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_24_8,              TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_10F_11F_11F_REV,   TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_5_9_9_9_REV,       TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_24_8_OES,          TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_FLOAT_32_UNSIGNED_INT_24_8_REV, TypeInfo( 8, true )));

    return map;
}

static bool GetTypeInfo(GLenum type, TypeInfo *outTypeInfo)
{
    static const TypeInfoMap infoMap = BuildTypeInfoMap();
    TypeInfoMap::const_iterator iter = infoMap.find(type);
    if (iter != infoMap.end())
    {
        if (outTypeInfo)
        {
            *outTypeInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}


typedef bool(*SupportCheckFunction)(GLuint, const Extensions &);

static bool UnimplementedSupport(GLuint clientVersion, const Extensions &)
{
    UNIMPLEMENTED();
    return false;
}

static bool NeverSupported(GLuint clientVersion, const Extensions &)
{
    return false;
}

template <GLuint minCoreGLVersion>
static bool RequireESVersion(GLuint clientVersion, const Extensions &)
{
    return clientVersion >= minCoreGLVersion;
}


typedef bool(Extensions::*ExtensionBool);


template <ExtensionBool bool1>
static bool RequireExtension(GLuint, const Extensions & extensions)
{
    return extensions.*bool1;
}


template <GLuint minCoreGLVersion, ExtensionBool bool1>
static bool RequireESVersionOrExtension(GLuint clientVersion, const Extensions &extensions)
{
    return clientVersion >= minCoreGLVersion || extensions.*bool1;
}


template <GLuint minCoreGLVersion, ExtensionBool bool1, ExtensionBool bool2>
static bool RequireESVersionOrExtensions(GLuint clientVersion, const Extensions &extensions)
{
    return clientVersion >= minCoreGLVersion || (extensions.*bool1 || extensions.*bool2);
}


template <ExtensionBool bool1, ExtensionBool bool2>
static bool RequireExtensions(GLuint, const Extensions &extensions)
{
    return extensions.*bool1 || extensions.*bool2;
}

struct InternalFormatInfo
{
    GLuint mRedBits;
    GLuint mGreenBits;
    GLuint mBlueBits;

    GLuint mLuminanceBits;

    GLuint mAlphaBits;
    GLuint mSharedBits;

    GLuint mDepthBits;
    GLuint mStencilBits;

    GLuint mPixelBits;

    GLuint mComponentCount;

    GLuint mCompressedBlockWidth;
    GLuint mCompressedBlockHeight;

    GLenum mFormat;
    GLenum mType;

    GLenum mComponentType;
    GLenum mColorEncoding;

    bool mIsCompressed;

    SupportCheckFunction mSupportFunction;

    InternalFormatInfo() : mRedBits(0), mGreenBits(0), mBlueBits(0), mLuminanceBits(0), mAlphaBits(0), mSharedBits(0), mDepthBits(0), mStencilBits(0),
                           mPixelBits(0), mComponentCount(0), mCompressedBlockWidth(0), mCompressedBlockHeight(0), mFormat(GL_NONE), mType(GL_NONE),
                           mComponentType(GL_NONE), mColorEncoding(GL_NONE), mIsCompressed(false), mSupportFunction(NeverSupported)
    {
    }

    static InternalFormatInfo UnsizedFormat(GLenum format, SupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mFormat = format;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo RGBAFormat(GLuint red, GLuint green, GLuint blue, GLuint alpha, GLuint shared,
                                         GLenum format, GLenum type, GLenum componentType, bool srgb,
                                         SupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mRedBits = red;
        formatInfo.mGreenBits = green;
        formatInfo.mBlueBits = blue;
        formatInfo.mAlphaBits = alpha;
        formatInfo.mSharedBits = shared;
        formatInfo.mPixelBits = red + green + blue + alpha + shared;
        formatInfo.mComponentCount = ((red > 0) ? 1 : 0) + ((green > 0) ? 1 : 0) + ((blue > 0) ? 1 : 0) + ((alpha > 0) ? 1 : 0);
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mComponentType = componentType;
        formatInfo.mColorEncoding = (srgb ? GL_SRGB : GL_LINEAR);
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo LUMAFormat(GLuint luminance, GLuint alpha, GLenum format, GLenum type, GLenum componentType,
                                         SupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mLuminanceBits = luminance;
        formatInfo.mAlphaBits = alpha;
        formatInfo.mPixelBits = luminance + alpha;
        formatInfo.mComponentCount = ((luminance > 0) ? 1 : 0) + ((alpha > 0) ? 1 : 0);
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mComponentType = componentType;
        formatInfo.mColorEncoding = GL_LINEAR;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo DepthStencilFormat(GLuint depthBits, GLuint stencilBits, GLuint unusedBits, GLenum format,
                                                 GLenum type, GLenum componentType, SupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mDepthBits = depthBits;
        formatInfo.mStencilBits = stencilBits;
        formatInfo.mPixelBits = depthBits + stencilBits + unusedBits;
        formatInfo.mComponentCount = ((depthBits > 0) ? 1 : 0) + ((stencilBits > 0) ? 1 : 0);
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mComponentType = componentType;
        formatInfo.mColorEncoding = GL_LINEAR;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo CompressedFormat(GLuint compressedBlockWidth, GLuint compressedBlockHeight, GLuint compressedBlockSize,
                                               GLuint componentCount, GLenum format, GLenum type, bool srgb,
                                               SupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mCompressedBlockWidth = compressedBlockWidth;
        formatInfo.mCompressedBlockHeight = compressedBlockHeight;
        formatInfo.mPixelBits = compressedBlockSize;
        formatInfo.mComponentCount = componentCount;
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mComponentType = GL_UNSIGNED_NORMALIZED;
        formatInfo.mColorEncoding = (srgb ? GL_SRGB : GL_LINEAR);
        formatInfo.mIsCompressed = true;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }
};

typedef std::pair<GLenum, InternalFormatInfo> InternalFormatInfoPair;
typedef std::map<GLenum, InternalFormatInfo> InternalFormatInfoMap;

static InternalFormatInfoMap BuildInternalFormatInfoMap()
{
    InternalFormatInfoMap map;

    
    map.insert(InternalFormatInfoPair(GL_NONE,              InternalFormatInfo()));

    
    map.insert(InternalFormatInfoPair(GL_R8,                InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED,          GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESVersionOrExtension<3, &Extensions::textureRG>)));
    map.insert(InternalFormatInfoPair(GL_R8_SNORM,          InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED,          GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RG8,               InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG,           GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESVersionOrExtension<3, &Extensions::textureRG>)));
    map.insert(InternalFormatInfoPair(GL_RG8_SNORM,         InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG,           GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB8,              InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESVersionOrExtension<3, &Extensions::rgb8rgba8>)));
    map.insert(InternalFormatInfoPair(GL_RGB8_SNORM,        InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB565,            InternalFormatInfo::RGBAFormat( 5,  6,  5,  0, 0, GL_RGB,          GL_UNSIGNED_SHORT_5_6_5,         GL_UNSIGNED_NORMALIZED, false, RequireESVersion<2>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA4,             InternalFormatInfo::RGBAFormat( 4,  4,  4,  4, 0, GL_RGBA,         GL_UNSIGNED_SHORT_4_4_4_4,       GL_UNSIGNED_NORMALIZED, false, RequireESVersion<2>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB5_A1,           InternalFormatInfo::RGBAFormat( 5,  5,  5,  1, 0, GL_RGBA,         GL_UNSIGNED_SHORT_5_5_5_1,       GL_UNSIGNED_NORMALIZED, false, RequireESVersion<2>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA8,             InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, false, RequireESVersionOrExtension<3, &Extensions::rgb8rgba8>)));
    map.insert(InternalFormatInfoPair(GL_RGBA8_SNORM,       InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_BYTE,                         GL_SIGNED_NORMALIZED,   false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB10_A2,          InternalFormatInfo::RGBAFormat(10, 10, 10,  2, 0, GL_RGBA,         GL_UNSIGNED_INT_2_10_10_10_REV,  GL_UNSIGNED_NORMALIZED, false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB10_A2UI,        InternalFormatInfo::RGBAFormat(10, 10, 10,  2, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV,  GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_SRGB8,             InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, true,  RequireESVersionOrExtension<3, &Extensions::sRGB>)));
    map.insert(InternalFormatInfoPair(GL_SRGB8_ALPHA8,      InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_UNSIGNED_BYTE,                GL_UNSIGNED_NORMALIZED, true,  RequireESVersionOrExtension<3, &Extensions::sRGB>)));
    map.insert(InternalFormatInfoPair(GL_R11F_G11F_B10F,    InternalFormatInfo::RGBAFormat(11, 11, 10,  0, 0, GL_RGB,          GL_UNSIGNED_INT_10F_11F_11F_REV, GL_FLOAT,               false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB9_E5,           InternalFormatInfo::RGBAFormat( 9,  9,  9,  0, 5, GL_RGB,          GL_UNSIGNED_INT_5_9_9_9_REV,     GL_FLOAT,               false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_R8I,               InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED_INTEGER,  GL_BYTE,                         GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_R8UI,              InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_R16I,              InternalFormatInfo::RGBAFormat(16,  0,  0,  0, 0, GL_RED_INTEGER,  GL_SHORT,                        GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_R16UI,             InternalFormatInfo::RGBAFormat(16,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_R32I,              InternalFormatInfo::RGBAFormat(32,  0,  0,  0, 0, GL_RED_INTEGER,  GL_INT,                          GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_R32UI,             InternalFormatInfo::RGBAFormat(32,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RG8I,              InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG_INTEGER,   GL_BYTE,                         GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RG8UI,             InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RG16I,             InternalFormatInfo::RGBAFormat(16, 16,  0,  0, 0, GL_RG_INTEGER,   GL_SHORT,                        GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RG16UI,            InternalFormatInfo::RGBAFormat(16, 16,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RG32I,             InternalFormatInfo::RGBAFormat(32, 32,  0,  0, 0, GL_RG_INTEGER,   GL_INT,                          GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RG32UI,            InternalFormatInfo::RGBAFormat(32, 32,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB8I,             InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB_INTEGER,  GL_BYTE,                         GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB8UI,            InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB16I,            InternalFormatInfo::RGBAFormat(16, 16, 16,  0, 0, GL_RGB_INTEGER,  GL_SHORT,                        GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB16UI,           InternalFormatInfo::RGBAFormat(16, 16, 16,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB32I,            InternalFormatInfo::RGBAFormat(32, 32, 32,  0, 0, GL_RGB_INTEGER,  GL_INT,                          GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGB32UI,           InternalFormatInfo::RGBAFormat(32, 32, 32,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA8I,            InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA_INTEGER, GL_BYTE,                         GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA8UI,           InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,                GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA16I,           InternalFormatInfo::RGBAFormat(16, 16, 16, 16, 0, GL_RGBA_INTEGER, GL_SHORT,                        GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA16UI,          InternalFormatInfo::RGBAFormat(16, 16, 16, 16, 0, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT,               GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA32I,           InternalFormatInfo::RGBAFormat(32, 32, 32, 32, 0, GL_RGBA_INTEGER, GL_INT,                          GL_INT,                 false, RequireESVersion<3>                    )));
    map.insert(InternalFormatInfoPair(GL_RGBA32UI,          InternalFormatInfo::RGBAFormat(32, 32, 32, 32, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT,                 GL_UNSIGNED_INT,        false, RequireESVersion<3>                    )));

    map.insert(InternalFormatInfoPair(GL_BGRA8_EXT,         InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_BGRA_EXT,     GL_UNSIGNED_BYTE,                  GL_UNSIGNED_NORMALIZED, false, RequireExtension<&Extensions::textureFormatBGRA8888>)));
    map.insert(InternalFormatInfoPair(GL_BGRA4_ANGLEX,      InternalFormatInfo::RGBAFormat( 4,  4,  4,  4, 0, GL_BGRA_EXT,     GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, GL_UNSIGNED_NORMALIZED, false, RequireExtension<&Extensions::textureFormatBGRA8888>)));
    map.insert(InternalFormatInfoPair(GL_BGR5_A1_ANGLEX,    InternalFormatInfo::RGBAFormat( 5,  5,  5,  1, 0, GL_BGRA_EXT,     GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, GL_UNSIGNED_NORMALIZED, false, RequireExtension<&Extensions::textureFormatBGRA8888>)));

    
    
    
    map.insert(InternalFormatInfoPair(GL_R16F,              InternalFormatInfo::RGBAFormat(16,  0,  0,  0, 0, GL_RED,          GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESVersionOrExtensions<3, &Extensions::textureHalfFloat, &Extensions::textureRG>)));
    map.insert(InternalFormatInfoPair(GL_RG16F,             InternalFormatInfo::RGBAFormat(16, 16,  0,  0, 0, GL_RG,           GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESVersionOrExtensions<3, &Extensions::textureHalfFloat, &Extensions::textureRG>)));
    map.insert(InternalFormatInfoPair(GL_RGB16F,            InternalFormatInfo::RGBAFormat(16, 16, 16,  0, 0, GL_RGB,          GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESVersionOrExtension<3, &Extensions::textureHalfFloat>                         )));
    map.insert(InternalFormatInfoPair(GL_RGBA16F,           InternalFormatInfo::RGBAFormat(16, 16, 16, 16, 0, GL_RGBA,         GL_HALF_FLOAT,                   GL_FLOAT, false, RequireESVersionOrExtension<3, &Extensions::textureHalfFloat>                         )));
    map.insert(InternalFormatInfoPair(GL_R32F,              InternalFormatInfo::RGBAFormat(32,  0,  0,  0, 0, GL_RED,          GL_FLOAT,                        GL_FLOAT, false, RequireESVersionOrExtensions<3, &Extensions::textureFloat, &Extensions::textureRG>    )));
    map.insert(InternalFormatInfoPair(GL_RG32F,             InternalFormatInfo::RGBAFormat(32, 32,  0,  0, 0, GL_RG,           GL_FLOAT,                        GL_FLOAT, false, RequireESVersionOrExtensions<3, &Extensions::textureFloat, &Extensions::textureRG>    )));
    map.insert(InternalFormatInfoPair(GL_RGB32F,            InternalFormatInfo::RGBAFormat(32, 32, 32,  0, 0, GL_RGB,          GL_FLOAT,                        GL_FLOAT, false, RequireESVersionOrExtension<3, &Extensions::textureFloat>                             )));
    map.insert(InternalFormatInfoPair(GL_RGBA32F,           InternalFormatInfo::RGBAFormat(32, 32, 32, 32, 0, GL_RGBA,         GL_FLOAT,                        GL_FLOAT, false, RequireESVersionOrExtension<3, &Extensions::textureFloat>                             )));

    
    
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT16,     InternalFormatInfo::DepthStencilFormat(16, 0,  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,                 GL_UNSIGNED_NORMALIZED, RequireESVersion<2>                        )));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT24,     InternalFormatInfo::DepthStencilFormat(24, 0,  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,                   GL_UNSIGNED_NORMALIZED, RequireESVersion<3>                        )));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT32F,    InternalFormatInfo::DepthStencilFormat(32, 0,  0, GL_DEPTH_COMPONENT, GL_FLOAT,                          GL_FLOAT,               RequireESVersion<3>                        )));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT32_OES, InternalFormatInfo::DepthStencilFormat(32, 0,  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,                   GL_UNSIGNED_NORMALIZED, RequireExtension<&Extensions::depthTextures>)));
    map.insert(InternalFormatInfoPair(GL_DEPTH24_STENCIL8,      InternalFormatInfo::DepthStencilFormat(24, 8,  0, GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8,              GL_UNSIGNED_NORMALIZED, RequireESVersionOrExtension<2, &Extensions::depthTextures>)));
    map.insert(InternalFormatInfoPair(GL_DEPTH32F_STENCIL8,     InternalFormatInfo::DepthStencilFormat(32, 8, 24, GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_FLOAT,               RequireESVersion<3>                        )));
    map.insert(InternalFormatInfoPair(GL_STENCIL_INDEX8,        InternalFormatInfo::DepthStencilFormat( 0, 8,  0, GL_DEPTH_STENCIL,   GL_UNSIGNED_BYTE,                  GL_UNSIGNED_INT,        RequireESVersion<2>                        )));

    
    
    map.insert(InternalFormatInfoPair(GL_ALPHA8_EXT,             InternalFormatInfo::LUMAFormat( 0,  8, GL_ALPHA,           GL_UNSIGNED_BYTE, GL_UNSIGNED_NORMALIZED, RequireExtension<&Extensions::textureStorage>                                )));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_EXT,         InternalFormatInfo::LUMAFormat( 8,  0, GL_LUMINANCE,       GL_UNSIGNED_BYTE, GL_UNSIGNED_NORMALIZED, RequireExtension<&Extensions::textureStorage>                                )));
    map.insert(InternalFormatInfoPair(GL_ALPHA32F_EXT,           InternalFormatInfo::LUMAFormat( 0, 32, GL_ALPHA,           GL_FLOAT,         GL_FLOAT,               RequireExtensions<&Extensions::textureStorage, &Extensions::textureFloat>    )));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE32F_EXT,       InternalFormatInfo::LUMAFormat(32,  0, GL_LUMINANCE,       GL_FLOAT,         GL_FLOAT,               RequireExtensions<&Extensions::textureStorage, &Extensions::textureFloat>    )));
    map.insert(InternalFormatInfoPair(GL_ALPHA16F_EXT,           InternalFormatInfo::LUMAFormat( 0, 16, GL_ALPHA,           GL_HALF_FLOAT,    GL_FLOAT,               RequireExtensions<&Extensions::textureStorage, &Extensions::textureHalfFloat>)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE16F_EXT,       InternalFormatInfo::LUMAFormat(16,  0, GL_LUMINANCE,       GL_HALF_FLOAT,    GL_FLOAT,               RequireExtensions<&Extensions::textureStorage, &Extensions::textureHalfFloat>)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_ALPHA8_EXT,  InternalFormatInfo::LUMAFormat( 8,  8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_UNSIGNED_NORMALIZED, RequireExtension<&Extensions::textureStorage>                                )));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA32F_EXT, InternalFormatInfo::LUMAFormat(32, 32, GL_LUMINANCE_ALPHA, GL_FLOAT,         GL_FLOAT,               RequireExtensions<&Extensions::textureStorage, &Extensions::textureFloat>    )));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA16F_EXT, InternalFormatInfo::LUMAFormat(16, 16, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT,    GL_FLOAT,               RequireExtensions<&Extensions::textureStorage, &Extensions::textureHalfFloat>)));

    
    
    map.insert(InternalFormatInfoPair(GL_ALPHA,           InternalFormatInfo::UnsizedFormat(GL_ALPHA,           RequireESVersion<2>                                            )));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE,       InternalFormatInfo::UnsizedFormat(GL_LUMINANCE,       RequireESVersion<2>                                            )));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA, InternalFormatInfo::UnsizedFormat(GL_LUMINANCE_ALPHA, RequireESVersion<2>                                            )));
    map.insert(InternalFormatInfoPair(GL_RED,             InternalFormatInfo::UnsizedFormat(GL_RED,             RequireESVersionOrExtension<3, &Extensions::textureRG>         )));
    map.insert(InternalFormatInfoPair(GL_RG,              InternalFormatInfo::UnsizedFormat(GL_RG,              RequireESVersionOrExtension<3, &Extensions::textureRG>         )));
    map.insert(InternalFormatInfoPair(GL_RGB,             InternalFormatInfo::UnsizedFormat(GL_RGB,             RequireESVersion<2>                                            )));
    map.insert(InternalFormatInfoPair(GL_RGBA,            InternalFormatInfo::UnsizedFormat(GL_RGBA,            RequireESVersion<2>                                            )));
    map.insert(InternalFormatInfoPair(GL_RED_INTEGER,     InternalFormatInfo::UnsizedFormat(GL_RED_INTEGER,     RequireESVersion<3>                                            )));
    map.insert(InternalFormatInfoPair(GL_RG_INTEGER,      InternalFormatInfo::UnsizedFormat(GL_RG_INTEGER,      RequireESVersion<3>                                            )));
    map.insert(InternalFormatInfoPair(GL_RGB_INTEGER,     InternalFormatInfo::UnsizedFormat(GL_RGB_INTEGER,     RequireESVersion<3>                                            )));
    map.insert(InternalFormatInfoPair(GL_RGBA_INTEGER,    InternalFormatInfo::UnsizedFormat(GL_RGBA_INTEGER,    RequireESVersion<3>                                            )));
    map.insert(InternalFormatInfoPair(GL_BGRA_EXT,        InternalFormatInfo::UnsizedFormat(GL_BGRA_EXT,        RequireExtension<&Extensions::textureFormatBGRA8888>           )));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT, InternalFormatInfo::UnsizedFormat(GL_DEPTH_COMPONENT, RequireESVersion<2>                                            )));
    map.insert(InternalFormatInfoPair(GL_DEPTH_STENCIL,   InternalFormatInfo::UnsizedFormat(GL_DEPTH_STENCIL,   RequireESVersionOrExtension<3, &Extensions::packedDepthStencil>)));
    map.insert(InternalFormatInfoPair(GL_SRGB_EXT,        InternalFormatInfo::UnsizedFormat(GL_RGB,             RequireESVersionOrExtension<3, &Extensions::sRGB>              )));
    map.insert(InternalFormatInfoPair(GL_SRGB_ALPHA_EXT,  InternalFormatInfo::UnsizedFormat(GL_RGBA,            RequireESVersionOrExtension<3, &Extensions::sRGB>              )));

    
    
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_R11_EAC,                        InternalFormatInfo::CompressedFormat(4, 4,  64, 1, GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE, false, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SIGNED_R11_EAC,                 InternalFormatInfo::CompressedFormat(4, 4,  64, 1, GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE, false, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RG11_EAC,                       InternalFormatInfo::CompressedFormat(4, 4, 128, 2, GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE, false, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SIGNED_RG11_EAC,                InternalFormatInfo::CompressedFormat(4, 4, 128, 2, GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE, false, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB8_ETC2,                      InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE, false, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_ETC2,                     InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE, true,  UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE, false, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE, true,  UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA8_ETC2_EAC,                 InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE, false, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE, true,  UnimplementedSupport)));

    
    
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    GL_UNSIGNED_BYTE, false, RequireExtension<&Extensions::textureCompressionDXT1>)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   InternalFormatInfo::CompressedFormat(4, 4,  64, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   GL_UNSIGNED_BYTE, false, RequireExtension<&Extensions::textureCompressionDXT1>)));

    
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, GL_UNSIGNED_BYTE, false, RequireExtension<&Extensions::textureCompressionDXT5>)));

    
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, GL_UNSIGNED_BYTE, false, RequireExtension<&Extensions::textureCompressionDXT5>)));

    return map;
}

static const InternalFormatInfoMap &GetInternalFormatMap()
{
    static const InternalFormatInfoMap formatMap = BuildInternalFormatInfoMap();
    return formatMap;
}

static bool GetInternalFormatInfo(GLenum internalFormat, InternalFormatInfo *outFormatInfo)
{
    const InternalFormatInfoMap &map = GetInternalFormatMap();
    InternalFormatInfoMap::const_iterator iter = map.find(internalFormat);
    if (iter != map.end())
    {
        if (outFormatInfo)
        {
            *outFormatInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}

static FormatSet BuildAllSizedInternalFormatSet()
{
    FormatSet result;

    const InternalFormatInfoMap &formats = GetInternalFormatMap();
    for (InternalFormatInfoMap::const_iterator i = formats.begin(); i != formats.end(); i++)
    {
        if (i->second.mPixelBits > 0)
        {
            result.insert(i->first);
        }
    }

    return result;
}

typedef std::set<GLenum> TypeSet;

struct EffectiveInternalFormatInfo
{
    GLenum mEffectiveFormat;
    GLenum mDestFormat;
    GLuint mMinRedBits;
    GLuint mMaxRedBits;
    GLuint mMinGreenBits;
    GLuint mMaxGreenBits;
    GLuint mMinBlueBits;
    GLuint mMaxBlueBits;
    GLuint mMinAlphaBits;
    GLuint mMaxAlphaBits;

    EffectiveInternalFormatInfo(GLenum effectiveFormat, GLenum destFormat, GLuint minRedBits, GLuint maxRedBits,
                                GLuint minGreenBits, GLuint maxGreenBits, GLuint minBlueBits, GLuint maxBlueBits,
                                GLuint minAlphaBits, GLuint maxAlphaBits)
        : mEffectiveFormat(effectiveFormat), mDestFormat(destFormat), mMinRedBits(minRedBits),
          mMaxRedBits(maxRedBits), mMinGreenBits(minGreenBits), mMaxGreenBits(maxGreenBits),
          mMinBlueBits(minBlueBits), mMaxBlueBits(maxBlueBits), mMinAlphaBits(minAlphaBits),
          mMaxAlphaBits(maxAlphaBits) {};
};

typedef std::vector<EffectiveInternalFormatInfo> EffectiveInternalFormatList;

static EffectiveInternalFormatList BuildSizedEffectiveInternalFormatList()
{
    EffectiveInternalFormatList list;

    
    
    
    
    list.push_back(EffectiveInternalFormatInfo(GL_ALPHA8_EXT,              GL_NONE, 0,  0, 0,  0, 0,  0, 1, 8));
    list.push_back(EffectiveInternalFormatInfo(GL_R8,                      GL_NONE, 1,  8, 0,  0, 0,  0, 0, 0));
    list.push_back(EffectiveInternalFormatInfo(GL_RG8,                     GL_NONE, 1,  8, 1,  8, 0,  0, 0, 0));
    list.push_back(EffectiveInternalFormatInfo(GL_RGB565,                  GL_NONE, 1,  5, 1,  6, 1,  5, 0, 0));
    list.push_back(EffectiveInternalFormatInfo(GL_RGB8,                    GL_NONE, 6,  8, 7,  8, 6,  8, 0, 0));
    list.push_back(EffectiveInternalFormatInfo(GL_RGBA4,                   GL_NONE, 1,  4, 1,  4, 1,  4, 1, 4));
    list.push_back(EffectiveInternalFormatInfo(GL_RGB5_A1,                 GL_NONE, 5,  5, 5,  5, 5,  5, 1, 1));
    list.push_back(EffectiveInternalFormatInfo(GL_RGBA8,                   GL_NONE, 5,  8, 5,  8, 5,  8, 2, 8));
    list.push_back(EffectiveInternalFormatInfo(GL_RGB10_A2,                GL_NONE, 9, 10, 9, 10, 9, 10, 2, 2));

    return list;
}


static EffectiveInternalFormatList BuildUnsizedEffectiveInternalFormatList()
{
    EffectiveInternalFormatList list;

    
    
    
    
    list.push_back(EffectiveInternalFormatInfo(GL_ALPHA8_EXT,              GL_ALPHA,           0, UINT_MAX, 0, UINT_MAX, 0, UINT_MAX, 1,        8));
    list.push_back(EffectiveInternalFormatInfo(GL_LUMINANCE8_EXT,          GL_LUMINANCE,       1,        8, 0, UINT_MAX, 0, UINT_MAX, 0, UINT_MAX));
    list.push_back(EffectiveInternalFormatInfo(GL_LUMINANCE8_ALPHA8_EXT,   GL_LUMINANCE_ALPHA, 1,        8, 0, UINT_MAX, 0, UINT_MAX, 1,        8));
    list.push_back(EffectiveInternalFormatInfo(GL_RGB565,                  GL_RGB,             1,        5, 1,        6, 1,        5, 0, UINT_MAX));
    list.push_back(EffectiveInternalFormatInfo(GL_RGB8,                    GL_RGB,             6,        8, 7,        8, 6,        8, 0, UINT_MAX));
    list.push_back(EffectiveInternalFormatInfo(GL_RGBA4,                   GL_RGBA,            1,        4, 1,        4, 1,        4, 1,        4));
    list.push_back(EffectiveInternalFormatInfo(GL_RGB5_A1,                 GL_RGBA,            5,        5, 5,        5, 5,        5, 1,        1));
    list.push_back(EffectiveInternalFormatInfo(GL_RGBA8,                   GL_RGBA,            5,        8, 5,        8, 5,        8, 5,        8));

    return list;
}

static bool GetEffectiveInternalFormat(const InternalFormatInfo &srcFormat, const InternalFormatInfo &destFormat,
                                       GLenum *outEffectiveFormat)
{
    const EffectiveInternalFormatList *list = NULL;
    GLenum targetFormat = GL_NONE;

    if (gl::IsSizedInternalFormat(destFormat.mFormat))
    {
        static const EffectiveInternalFormatList sizedList = BuildSizedEffectiveInternalFormatList();
        list = &sizedList;
    }
    else
    {
        static const EffectiveInternalFormatList unsizedList = BuildUnsizedEffectiveInternalFormatList();
        list = &unsizedList;
        targetFormat = destFormat.mFormat;
    }

    for (size_t curFormat = 0; curFormat < list->size(); ++curFormat)
    {
        const EffectiveInternalFormatInfo& formatInfo = list->at(curFormat);
        if ((formatInfo.mDestFormat == targetFormat) &&
            (formatInfo.mMinRedBits   <= srcFormat.mRedBits   && formatInfo.mMaxRedBits   >= srcFormat.mRedBits)   &&
            (formatInfo.mMinGreenBits <= srcFormat.mGreenBits && formatInfo.mMaxGreenBits >= srcFormat.mGreenBits) &&
            (formatInfo.mMinBlueBits  <= srcFormat.mBlueBits  && formatInfo.mMaxBlueBits  >= srcFormat.mBlueBits)  &&
            (formatInfo.mMinAlphaBits <= srcFormat.mAlphaBits && formatInfo.mMaxAlphaBits >= srcFormat.mAlphaBits))
        {
            *outEffectiveFormat = formatInfo.mEffectiveFormat;
            return true;
        }
    }

    return false;
}

struct CopyConversion
{
    GLenum mTextureFormat;
    GLenum mFramebufferFormat;

    CopyConversion(GLenum textureFormat, GLenum framebufferFormat)
        : mTextureFormat(textureFormat), mFramebufferFormat(framebufferFormat) { }

    bool operator<(const CopyConversion& other) const
    {
        return memcmp(this, &other, sizeof(CopyConversion)) < 0;
    }
};

typedef std::set<CopyConversion> CopyConversionSet;

static CopyConversionSet BuildValidES3CopyTexImageCombinations()
{
    CopyConversionSet set;

    
    set.insert(CopyConversion(GL_ALPHA,           GL_RGBA));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RED));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RG));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RGB));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RGBA));
    set.insert(CopyConversion(GL_LUMINANCE_ALPHA, GL_RGBA));
    set.insert(CopyConversion(GL_RED,             GL_RED));
    set.insert(CopyConversion(GL_RED,             GL_RG));
    set.insert(CopyConversion(GL_RED,             GL_RGB));
    set.insert(CopyConversion(GL_RED,             GL_RGBA));
    set.insert(CopyConversion(GL_RG,              GL_RG));
    set.insert(CopyConversion(GL_RG,              GL_RGB));
    set.insert(CopyConversion(GL_RG,              GL_RGBA));
    set.insert(CopyConversion(GL_RGB,             GL_RGB));
    set.insert(CopyConversion(GL_RGB,             GL_RGBA));
    set.insert(CopyConversion(GL_RGBA,            GL_RGBA));

    
    set.insert(CopyConversion(GL_ALPHA,           GL_BGRA_EXT));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_BGRA_EXT));
    set.insert(CopyConversion(GL_LUMINANCE_ALPHA, GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RED,             GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RG,              GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RGB,             GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RGBA,            GL_BGRA_EXT));

    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RED_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RG_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER,      GL_RG_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER,      GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER,      GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RGB_INTEGER,     GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RGB_INTEGER,     GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RGBA_INTEGER,    GL_RGBA_INTEGER));

    return set;
}

bool IsValidInternalFormat(GLenum internalFormat, const Extensions &extensions, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        ASSERT(internalFormatInfo.mSupportFunction != NULL);
        return internalFormatInfo.mSupportFunction(clientVersion, extensions);
    }
    else
    {
        return false;
    }
}

bool IsValidFormat(GLenum format, const Extensions &extensions, GLuint clientVersion)
{
    const InternalFormatInfoMap &internalFormats = GetInternalFormatMap();
    for (InternalFormatInfoMap::const_iterator i = internalFormats.begin(); i != internalFormats.end(); i++)
    {
        if (i->second.mFormat == format && i->second.mSupportFunction(clientVersion, extensions))
        {
            return true;
        }
    }

    return false;
}

bool IsValidType(GLenum type, const Extensions &extensions, GLuint clientVersion)
{
    const InternalFormatInfoMap &internalFormats = GetInternalFormatMap();
    for (InternalFormatInfoMap::const_iterator i = internalFormats.begin(); i != internalFormats.end(); i++)
    {
        if (i->second.mType == type && i->second.mSupportFunction(clientVersion, extensions))
        {
            return true;
        }
    }

    return false;
}

bool IsValidFormatCombination(GLenum internalFormat, GLenum format, GLenum type, const Extensions &extensions, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        if (!internalFormatInfo.mSupportFunction(clientVersion, extensions))
        {
            return false;
        }
    }
    else
    {
        UNREACHABLE();
        return false;
    }

    if (clientVersion == 2)
    {
        static const FormatMap &formats = GetFormatMap();
        FormatMap::const_iterator iter = formats.find(FormatTypePair(format, type));
        return (iter != formats.end()) && ((internalFormat == (GLint)type) || (internalFormat == iter->second.mInternalFormat));
    }
    else if (clientVersion == 3)
    {
        static const ES3FormatSet &formats = GetES3FormatSet();
        return formats.find(FormatInfo(internalFormat, format, type)) != formats.end();
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsValidCopyTexImageCombination(GLenum textureInternalFormat, GLenum frameBufferInternalFormat, GLuint readBufferHandle, GLuint clientVersion)
{
    InternalFormatInfo textureInternalFormatInfo;
    InternalFormatInfo framebufferInternalFormatInfo;
    if (GetInternalFormatInfo(textureInternalFormat, &textureInternalFormatInfo) &&
        GetInternalFormatInfo(frameBufferInternalFormat, &framebufferInternalFormatInfo))
    {
        if (clientVersion == 2)
        {
            UNIMPLEMENTED();
            return false;
        }
        else if (clientVersion == 3)
        {
            static const CopyConversionSet conversionSet = BuildValidES3CopyTexImageCombinations();
            const CopyConversion conversion = CopyConversion(textureInternalFormatInfo.mFormat,
                                                             framebufferInternalFormatInfo.mFormat);
            if (conversionSet.find(conversion) != conversionSet.end())
            {
                
                
                
                

                if ((textureInternalFormatInfo.mColorEncoding == GL_SRGB) != (framebufferInternalFormatInfo.mColorEncoding == GL_SRGB))
                {
                    return false;
                }

                if (((textureInternalFormatInfo.mComponentType == GL_INT) != (framebufferInternalFormatInfo.mComponentType == GL_INT)) ||
                    ((textureInternalFormatInfo.mComponentType == GL_UNSIGNED_INT) != (framebufferInternalFormatInfo.mComponentType == GL_UNSIGNED_INT)))
                {
                    return false;
                }

                if (gl::IsFloatOrFixedComponentType(textureInternalFormatInfo.mComponentType) &&
                    !gl::IsFloatOrFixedComponentType(framebufferInternalFormatInfo.mComponentType))
                {
                    return false;
                }

                
                
                
                
                
                
                
                
                
                
                
                
                
                InternalFormatInfo sourceEffectiveFormat;
                if (readBufferHandle != 0)
                {
                    
                    if (gl::IsSizedInternalFormat(framebufferInternalFormatInfo.mFormat))
                    {
                        sourceEffectiveFormat = framebufferInternalFormatInfo;
                    }
                    else
                    {
                        
                        
                        GLenum effectiveFormat = gl::GetSizedInternalFormat(framebufferInternalFormatInfo.mFormat,
                                                                            framebufferInternalFormatInfo.mType);
                        gl::GetInternalFormatInfo(effectiveFormat, &sourceEffectiveFormat);
                    }
                }
                else
                {
                    
                    
                    if (framebufferInternalFormatInfo.mColorEncoding == GL_LINEAR)
                    {
                        GLenum effectiveFormat;
                        if (GetEffectiveInternalFormat(framebufferInternalFormatInfo, textureInternalFormatInfo, &effectiveFormat))
                        {
                            gl::GetInternalFormatInfo(effectiveFormat, &sourceEffectiveFormat);
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else if (framebufferInternalFormatInfo.mColorEncoding == GL_SRGB)
                    {
                        
                        if (gl::IsSizedInternalFormat(textureInternalFormat) &&
                            (framebufferInternalFormatInfo.mRedBits   >= 1 && framebufferInternalFormatInfo.mRedBits   <= 8) &&
                            (framebufferInternalFormatInfo.mGreenBits >= 1 && framebufferInternalFormatInfo.mGreenBits <= 8) &&
                            (framebufferInternalFormatInfo.mBlueBits  >= 1 && framebufferInternalFormatInfo.mBlueBits  <= 8) &&
                            (framebufferInternalFormatInfo.mAlphaBits >= 1 && framebufferInternalFormatInfo.mAlphaBits <= 8))
                        {
                            gl::GetInternalFormatInfo(GL_SRGB8_ALPHA8, &sourceEffectiveFormat);
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        UNREACHABLE();
                    }
                }

                if (gl::IsSizedInternalFormat(textureInternalFormatInfo.mFormat))
                {
                    
                    
                    if (textureInternalFormatInfo.mRedBits != sourceEffectiveFormat.mRedBits ||
                        textureInternalFormatInfo.mGreenBits != sourceEffectiveFormat.mGreenBits ||
                        textureInternalFormatInfo.mBlueBits != sourceEffectiveFormat.mBlueBits ||
                        textureInternalFormatInfo.mAlphaBits != sourceEffectiveFormat.mAlphaBits)
                    {
                        return false;
                    }
                }


                return true; 
                             
            }

            return false;
        }
        else
        {
            UNREACHABLE();
            return false;
        }
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsSizedInternalFormat(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mPixelBits > 0;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

GLenum GetSizedInternalFormat(GLenum format, GLenum type)
{
    const FormatMap &formats = GetFormatMap();
    FormatMap::const_iterator iter = formats.find(FormatTypePair(format, type));
    return (iter != formats.end()) ? iter->second.mInternalFormat : GL_NONE;
}

GLuint GetPixelBytes(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mPixelBits / 8;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetAlphaBits(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mAlphaBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetRedBits(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mRedBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetGreenBits(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mGreenBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlueBits(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mBlueBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetLuminanceBits(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mLuminanceBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetDepthBits(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mDepthBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetStencilBits(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mStencilBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetTypeBytes(GLenum type)
{
    TypeInfo typeInfo;
    if (GetTypeInfo(type, &typeInfo))
    {
        return typeInfo.mTypeBytes;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

bool IsSpecialInterpretationType(GLenum type)
{
    TypeInfo typeInfo;
    if (GetTypeInfo(type, &typeInfo))
    {
        return typeInfo.mSpecialInterpretation;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsFloatOrFixedComponentType(GLenum type)
{
    if (type == GL_UNSIGNED_NORMALIZED ||
        type == GL_SIGNED_NORMALIZED ||
        type == GL_FLOAT)
    {
        return true;
    }
    else
    {
        return false;
    }
}

GLenum GetFormat(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mFormat;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

GLenum GetType(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mType;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

GLenum GetComponentType(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mComponentType;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

GLuint GetComponentCount(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mComponentCount;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

GLenum GetColorEncoding(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mColorEncoding;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

GLuint GetRowPitch(GLenum internalFormat, GLenum type, GLsizei width, GLint alignment)
{
    ASSERT(alignment > 0 && isPow2(alignment));
    return rx::roundUp(GetBlockSize(internalFormat, type, width, 1), static_cast<GLuint>(alignment));
}

GLuint GetDepthPitch(GLenum internalFormat, GLenum type, GLsizei width, GLsizei height, GLint alignment)
{
    return GetRowPitch(internalFormat, type, width, alignment) * height;
}

GLuint GetBlockSize(GLenum internalFormat, GLenum type, GLsizei width, GLsizei height)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        if (internalFormatInfo.mIsCompressed)
        {
            GLsizei numBlocksWide = (width + internalFormatInfo.mCompressedBlockWidth - 1) / internalFormatInfo.mCompressedBlockWidth;
            GLsizei numBlocksHight = (height + internalFormatInfo.mCompressedBlockHeight - 1) / internalFormatInfo.mCompressedBlockHeight;

            return (internalFormatInfo.mPixelBits * numBlocksWide * numBlocksHight) / 8;
        }
        else
        {
            TypeInfo typeInfo;
            if (GetTypeInfo(type, &typeInfo))
            {
                if (typeInfo.mSpecialInterpretation)
                {
                    return typeInfo.mTypeBytes * width * height;
                }
                else
                {
                    return internalFormatInfo.mComponentCount * typeInfo.mTypeBytes * width * height;
                }
            }
            else
            {
                UNREACHABLE();
                return 0;
            }
        }
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

bool IsFormatCompressed(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mIsCompressed;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

GLuint GetCompressedBlockWidth(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mCompressedBlockWidth;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetCompressedBlockHeight(GLenum internalFormat)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, &internalFormatInfo))
    {
        return internalFormatInfo.mCompressedBlockHeight;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

const FormatSet &GetAllSizedInternalFormats()
{
    static FormatSet formatSet = BuildAllSizedInternalFormatSet();
    return formatSet;
}

ColorWriteFunction GetColorWriteFunction(GLenum format, GLenum type)
{
    static const FormatMap &formats = GetFormatMap();
    FormatMap::const_iterator iter = formats.find(FormatTypePair(format, type));
    return (iter != formats.end()) ? iter->second.mColorWriteFunction : NULL;
}

}

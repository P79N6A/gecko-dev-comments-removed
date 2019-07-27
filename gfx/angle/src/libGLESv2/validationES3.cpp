







#include "libGLESv2/validationES3.h"
#include "libGLESv2/validationES.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/main.h"
#include "libGLESv2/FramebufferAttachment.h"

#include "common/mathutil.h"

namespace gl
{

struct ES3FormatCombination
{
    GLenum internalFormat;
    GLenum format;
    GLenum type;
};

bool operator<(const ES3FormatCombination& a, const ES3FormatCombination& b)
{
    return memcmp(&a, &b, sizeof(ES3FormatCombination)) < 0;
}

typedef std::set<ES3FormatCombination> ES3FormatCombinationSet;

static inline void InsertES3FormatCombo(ES3FormatCombinationSet *set, GLenum internalFormat, GLenum format, GLenum type)
{
    ES3FormatCombination info;
    info.internalFormat = internalFormat;
    info.format = format;
    info.type = type;
    set->insert(info);
}

ES3FormatCombinationSet BuildES3FormatSet()
{
    ES3FormatCombinationSet set;

    

    
    
    InsertES3FormatCombo(&set, GL_RGBA8,              GL_RGBA,            GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGBA4,              GL_RGBA,            GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_SRGB8_ALPHA8,       GL_RGBA,            GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGBA8_SNORM,        GL_RGBA,            GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_RGBA4,              GL_RGBA,            GL_UNSIGNED_SHORT_4_4_4_4        );
    InsertES3FormatCombo(&set, GL_RGB10_A2,           GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV   );
    InsertES3FormatCombo(&set, GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV   );
    InsertES3FormatCombo(&set, GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1        );
    InsertES3FormatCombo(&set, GL_RGBA16F,            GL_RGBA,            GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_RGBA16F,            GL_RGBA,            GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_RGBA32F,            GL_RGBA,            GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_RGBA16F,            GL_RGBA,            GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_RGBA8UI,            GL_RGBA_INTEGER,    GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGBA8I,             GL_RGBA_INTEGER,    GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_RGBA16UI,           GL_RGBA_INTEGER,    GL_UNSIGNED_SHORT                );
    InsertES3FormatCombo(&set, GL_RGBA16I,            GL_RGBA_INTEGER,    GL_SHORT                         );
    InsertES3FormatCombo(&set, GL_RGBA32UI,           GL_RGBA_INTEGER,    GL_UNSIGNED_INT                  );
    InsertES3FormatCombo(&set, GL_RGBA32I,            GL_RGBA_INTEGER,    GL_INT                           );
    InsertES3FormatCombo(&set, GL_RGB10_A2UI,         GL_RGBA_INTEGER,    GL_UNSIGNED_INT_2_10_10_10_REV   );
    InsertES3FormatCombo(&set, GL_RGB8,               GL_RGB,             GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGB565,             GL_RGB,             GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_SRGB8,              GL_RGB,             GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGB8_SNORM,         GL_RGB,             GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_RGB565,             GL_RGB,             GL_UNSIGNED_SHORT_5_6_5          );
    InsertES3FormatCombo(&set, GL_R11F_G11F_B10F,     GL_RGB,             GL_UNSIGNED_INT_10F_11F_11F_REV  );
    InsertES3FormatCombo(&set, GL_RGB9_E5,            GL_RGB,             GL_UNSIGNED_INT_5_9_9_9_REV      );
    InsertES3FormatCombo(&set, GL_RGB16F,             GL_RGB,             GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_RGB16F,             GL_RGB,             GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_R11F_G11F_B10F,     GL_RGB,             GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_R11F_G11F_B10F,     GL_RGB,             GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_RGB9_E5,            GL_RGB,             GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_RGB9_E5,            GL_RGB,             GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_RGB32F,             GL_RGB,             GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_RGB16F,             GL_RGB,             GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_R11F_G11F_B10F,     GL_RGB,             GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_RGB9_E5,            GL_RGB,             GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_RGB8UI,             GL_RGB_INTEGER,     GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGB8I,              GL_RGB_INTEGER,     GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_RGB16UI,            GL_RGB_INTEGER,     GL_UNSIGNED_SHORT                );
    InsertES3FormatCombo(&set, GL_RGB16I,             GL_RGB_INTEGER,     GL_SHORT                         );
    InsertES3FormatCombo(&set, GL_RGB32UI,            GL_RGB_INTEGER,     GL_UNSIGNED_INT                  );
    InsertES3FormatCombo(&set, GL_RGB32I,             GL_RGB_INTEGER,     GL_INT                           );
    InsertES3FormatCombo(&set, GL_RG8,                GL_RG,              GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RG8_SNORM,          GL_RG,              GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_RG16F,              GL_RG,              GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_RG16F,              GL_RG,              GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_RG32F,              GL_RG,              GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_RG16F,              GL_RG,              GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_RG8UI,              GL_RG_INTEGER,      GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RG8I,               GL_RG_INTEGER,      GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_RG16UI,             GL_RG_INTEGER,      GL_UNSIGNED_SHORT                );
    InsertES3FormatCombo(&set, GL_RG16I,              GL_RG_INTEGER,      GL_SHORT                         );
    InsertES3FormatCombo(&set, GL_RG32UI,             GL_RG_INTEGER,      GL_UNSIGNED_INT                  );
    InsertES3FormatCombo(&set, GL_RG32I,              GL_RG_INTEGER,      GL_INT                           );
    InsertES3FormatCombo(&set, GL_R8,                 GL_RED,             GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_R8_SNORM,           GL_RED,             GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_R16F,               GL_RED,             GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_R16F,               GL_RED,             GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_R32F,               GL_RED,             GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_R16F,               GL_RED,             GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_R8UI,               GL_RED_INTEGER,     GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_R8I,                GL_RED_INTEGER,     GL_BYTE                          );
    InsertES3FormatCombo(&set, GL_R16UI,              GL_RED_INTEGER,     GL_UNSIGNED_SHORT                );
    InsertES3FormatCombo(&set, GL_R16I,               GL_RED_INTEGER,     GL_SHORT                         );
    InsertES3FormatCombo(&set, GL_R32UI,              GL_RED_INTEGER,     GL_UNSIGNED_INT                  );
    InsertES3FormatCombo(&set, GL_R32I,               GL_RED_INTEGER,     GL_INT                           );

    
    InsertES3FormatCombo(&set, GL_RGBA,               GL_RGBA,            GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGBA,               GL_RGBA,            GL_UNSIGNED_SHORT_4_4_4_4        );
    InsertES3FormatCombo(&set, GL_RGBA,               GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1        );
    InsertES3FormatCombo(&set, GL_RGB,                GL_RGB,             GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_RGB,                GL_RGB,             GL_UNSIGNED_SHORT_5_6_5          );
    InsertES3FormatCombo(&set, GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_LUMINANCE,          GL_LUMINANCE,       GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_ALPHA,              GL_ALPHA,           GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_SRGB_ALPHA_EXT,     GL_SRGB_ALPHA_EXT,  GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_SRGB_EXT,           GL_SRGB_EXT,        GL_UNSIGNED_BYTE                 );

    
    InsertES3FormatCombo(&set, GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT                );
    InsertES3FormatCombo(&set, GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT                  );
    InsertES3FormatCombo(&set, GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT                  );
    InsertES3FormatCombo(&set, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_DEPTH24_STENCIL8,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8             );
    InsertES3FormatCombo(&set, GL_DEPTH32F_STENCIL8,  GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

    
    InsertES3FormatCombo(&set, GL_SRGB8_ALPHA8_EXT,   GL_SRGB_ALPHA_EXT, GL_UNSIGNED_BYTE                  );
    InsertES3FormatCombo(&set, GL_SRGB8,              GL_SRGB_EXT,       GL_UNSIGNED_BYTE                  );

    
    InsertES3FormatCombo(&set, GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_LUMINANCE,          GL_LUMINANCE,       GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_ALPHA,              GL_ALPHA,           GL_FLOAT                         );

    
    InsertES3FormatCombo(&set, GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_LUMINANCE,          GL_LUMINANCE,       GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_LUMINANCE,          GL_LUMINANCE,       GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_ALPHA,              GL_ALPHA,           GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_ALPHA,              GL_ALPHA,           GL_HALF_FLOAT_OES                );

    
    InsertES3FormatCombo(&set, GL_BGRA_EXT,           GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 );

    
    
    
    InsertES3FormatCombo(&set, GL_ALPHA8_EXT,             GL_ALPHA,           GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_LUMINANCE8_EXT,         GL_LUMINANCE,       GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_LUMINANCE8_ALPHA8_EXT,  GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_ALPHA32F_EXT,           GL_ALPHA,           GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_LUMINANCE32F_EXT,       GL_LUMINANCE,       GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_LUMINANCE_ALPHA32F_EXT, GL_LUMINANCE_ALPHA, GL_FLOAT                         );
    InsertES3FormatCombo(&set, GL_ALPHA16F_EXT,           GL_ALPHA,           GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_ALPHA16F_EXT,           GL_ALPHA,           GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_LUMINANCE16F_EXT,       GL_LUMINANCE,       GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_LUMINANCE16F_EXT,       GL_LUMINANCE,       GL_HALF_FLOAT_OES                );
    InsertES3FormatCombo(&set, GL_LUMINANCE_ALPHA16F_EXT, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT                    );
    InsertES3FormatCombo(&set, GL_LUMINANCE_ALPHA16F_EXT, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT_OES                );

    
    InsertES3FormatCombo(&set, GL_BGRA8_EXT,              GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_BGRA4_ANGLEX,           GL_BGRA_EXT,        GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT);
    InsertES3FormatCombo(&set, GL_BGRA4_ANGLEX,           GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 );
    InsertES3FormatCombo(&set, GL_BGR5_A1_ANGLEX,         GL_BGRA_EXT,        GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT);
    InsertES3FormatCombo(&set, GL_BGR5_A1_ANGLEX,         GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 );

    
    InsertES3FormatCombo(&set, GL_DEPTH_COMPONENT32_OES,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT_24_8_OES         );

    
    
    
    
    InsertES3FormatCombo(&set, GL_COMPRESSED_R11_EAC,                        GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_R11_EAC,                        GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_SIGNED_R11_EAC,                 GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_RG11_EAC,                       GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_SIGNED_RG11_EAC,                GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_RGB8_ETC2,                      GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_SRGB8_ETC2,                     GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE);


    
    InsertES3FormatCombo(&set, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_UNSIGNED_BYTE);
    InsertES3FormatCombo(&set, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_UNSIGNED_BYTE);

    
    InsertES3FormatCombo(&set, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_UNSIGNED_BYTE);

    
    InsertES3FormatCombo(&set, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_UNSIGNED_BYTE);

    return set;
}

static bool ValidateTexImageFormatCombination(gl::Context *context, GLenum internalFormat, GLenum format, GLenum type)
{
    
    
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);
    if (!formatInfo.textureSupport(context->getClientVersion(), context->getExtensions()))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    
    bool formatSupported = false;
    bool typeSupported = false;

    static const ES3FormatCombinationSet es3FormatSet = BuildES3FormatSet();
    for (ES3FormatCombinationSet::const_iterator i = es3FormatSet.begin(); i != es3FormatSet.end(); i++)
    {
        if (i->format == format || i->type == type)
        {
            const gl::InternalFormat &info = gl::GetInternalFormatInfo(i->internalFormat);
            bool supported = info.textureSupport(context->getClientVersion(), context->getExtensions());
            if (supported && i->type == type)
            {
                typeSupported = true;
            }
            if (supported && i->format == format)
            {
                formatSupported = true;
            }

            
            if (typeSupported && formatSupported)
            {
                break;
            }
        }
    }

    if (!typeSupported || !formatSupported)
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    
    ES3FormatCombination searchFormat;
    searchFormat.internalFormat = internalFormat;
    searchFormat.format = format;
    searchFormat.type = type;

    if (es3FormatSet.find(searchFormat) == es3FormatSet.end())
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateES3TexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat, bool isCompressed, bool isSubImage,
                                   GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                   GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    if (!ValidTexture2DDestinationTarget(context, target))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    
    if (!ValidImageSize(context, target, level, width, height, depth))
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    
    if (border != 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (xoffset < 0 || yoffset < 0 || zoffset < 0 ||
        std::numeric_limits<GLsizei>::max() - xoffset < width ||
        std::numeric_limits<GLsizei>::max() - yoffset < height ||
        std::numeric_limits<GLsizei>::max() - zoffset < depth)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    const gl::Caps &caps = context->getCaps();

    gl::Texture *texture = NULL;
    bool textureCompressed = false;
    GLenum textureInternalFormat = GL_NONE;
    GLint textureLevelWidth = 0;
    GLint textureLevelHeight = 0;
    GLint textureLevelDepth = 0;
    switch (target)
    {
      case GL_TEXTURE_2D:
        {
            if (static_cast<GLuint>(width) > (caps.max2DTextureSize >> level) ||
                static_cast<GLuint>(height) > (caps.max2DTextureSize >> level))
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }

            gl::Texture2D *texture2d = context->getTexture2D();
            if (texture2d)
            {
                textureCompressed = texture2d->isCompressed(level);
                textureInternalFormat = texture2d->getInternalFormat(level);
                textureLevelWidth = texture2d->getWidth(level);
                textureLevelHeight = texture2d->getHeight(level);
                textureLevelDepth = 1;
                texture = texture2d;
            }
        }
        break;

      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        {
            if (!isSubImage && width != height)
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }

            if (static_cast<GLuint>(width) > (caps.maxCubeMapTextureSize >> level))
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }

            gl::TextureCubeMap *textureCube = context->getTextureCubeMap();
            if (textureCube)
            {
                textureCompressed = textureCube->isCompressed(target, level);
                textureInternalFormat = textureCube->getInternalFormat(target, level);
                textureLevelWidth = textureCube->getWidth(target, level);
                textureLevelHeight = textureCube->getHeight(target, level);
                textureLevelDepth = 1;
                texture = textureCube;
            }
        }
        break;

      case GL_TEXTURE_3D:
        {
            if (static_cast<GLuint>(width) > (caps.max3DTextureSize >> level) ||
                static_cast<GLuint>(height) > (caps.max3DTextureSize >> level) ||
                static_cast<GLuint>(depth) > (caps.max3DTextureSize >> level))
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }

            gl::Texture3D *texture3d = context->getTexture3D();
            if (texture3d)
            {
                textureCompressed = texture3d->isCompressed(level);
                textureInternalFormat = texture3d->getInternalFormat(level);
                textureLevelWidth = texture3d->getWidth(level);
                textureLevelHeight = texture3d->getHeight(level);
                textureLevelDepth = texture3d->getDepth(level);
                texture = texture3d;
            }
        }
        break;

        case GL_TEXTURE_2D_ARRAY:
          {
              if (static_cast<GLuint>(width) > (caps.max2DTextureSize >> level) ||
                  static_cast<GLuint>(height) > (caps.max2DTextureSize >> level) ||
                  static_cast<GLuint>(depth) > (caps.maxArrayTextureLayers >> level))
              {
                  context->recordError(Error(GL_INVALID_VALUE));
                  return false;
              }

              gl::Texture2DArray *texture2darray = context->getTexture2DArray();
              if (texture2darray)
              {
                  textureCompressed = texture2darray->isCompressed(level);
                  textureInternalFormat = texture2darray->getInternalFormat(level);
                  textureLevelWidth = texture2darray->getWidth(level);
                  textureLevelHeight = texture2darray->getHeight(level);
                  textureLevelDepth = texture2darray->getLayers(level);
                  texture = texture2darray;
              }
          }
          break;

      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (!texture)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (texture->isImmutable() && !isSubImage)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    
    GLenum actualInternalFormat = isSubImage ? textureInternalFormat : internalformat;
    const gl::InternalFormat &actualFormatInfo = gl::GetInternalFormatInfo(actualInternalFormat);
    if (isCompressed)
    {
        if (!ValidCompressedImageSize(context, actualInternalFormat, width, height))
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }

        if (!actualFormatInfo.compressed)
        {
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }

        if (target == GL_TEXTURE_3D)
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }
    else
    {
        if (!ValidateTexImageFormatCombination(context, actualInternalFormat, format, type))
        {
            return false;
        }

        if (target == GL_TEXTURE_3D && (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL))
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    
    if (isSubImage)
    {
        if (isCompressed != textureCompressed)
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }

        if (isCompressed)
        {
            if ((width % 4 != 0 && width != textureLevelWidth) ||
                (height % 4 != 0 && height != textureLevelHeight))
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }
        }

        if (width == 0 || height == 0 || depth == 0)
        {
            return false;
        }

        if (xoffset < 0 || yoffset < 0 || zoffset < 0)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }

        if (std::numeric_limits<GLsizei>::max() - xoffset < width ||
            std::numeric_limits<GLsizei>::max() - yoffset < height ||
            std::numeric_limits<GLsizei>::max() - zoffset < depth)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }

        if (xoffset + width > textureLevelWidth ||
            yoffset + height > textureLevelHeight ||
            zoffset + depth > textureLevelDepth)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
    }

    
    gl::Buffer *pixelUnpackBuffer = context->getState().getTargetBuffer(GL_PIXEL_UNPACK_BUFFER);
    if (pixelUnpackBuffer != NULL)
    {
        
        
        size_t widthSize = static_cast<size_t>(width);
        size_t heightSize = static_cast<size_t>(height);
        size_t depthSize = static_cast<size_t>(depth);
        GLenum sizedFormat = GetSizedInternalFormat(actualInternalFormat, type);

        size_t pixelBytes = static_cast<size_t>(gl::GetInternalFormatInfo(sizedFormat).pixelBytes);

        if (!rx::IsUnsignedMultiplicationSafe(widthSize, heightSize) ||
            !rx::IsUnsignedMultiplicationSafe(widthSize * heightSize, depthSize) ||
            !rx::IsUnsignedMultiplicationSafe(widthSize * heightSize * depthSize, pixelBytes))
        {
            
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }

        size_t copyBytes = widthSize * heightSize * depthSize * pixelBytes;
        size_t offset = reinterpret_cast<size_t>(pixels);

        if (!rx::IsUnsignedAdditionSafe(offset, copyBytes) ||
            ((offset + copyBytes) > static_cast<size_t>(pixelUnpackBuffer->getSize())))
        {
            
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }

        
        
        size_t dataBytesPerPixel = static_cast<size_t>(gl::GetTypeInfo(type).bytes);

        if ((offset % dataBytesPerPixel) != 0)
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }

        
        if (pixelUnpackBuffer->isMapped())
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    return true;
}

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

static bool GetEffectiveInternalFormat(const InternalFormat &srcFormat, const InternalFormat &destFormat,
                                       GLenum *outEffectiveFormat)
{
    const EffectiveInternalFormatList *list = NULL;
    GLenum targetFormat = GL_NONE;

    if (destFormat.pixelBytes > 0)
    {
        static const EffectiveInternalFormatList sizedList = BuildSizedEffectiveInternalFormatList();
        list = &sizedList;
    }
    else
    {
        static const EffectiveInternalFormatList unsizedList = BuildUnsizedEffectiveInternalFormatList();
        list = &unsizedList;
        targetFormat = destFormat.format;
    }

    for (size_t curFormat = 0; curFormat < list->size(); ++curFormat)
    {
        const EffectiveInternalFormatInfo& formatInfo = list->at(curFormat);
        if ((formatInfo.mDestFormat == targetFormat) &&
            (formatInfo.mMinRedBits   <= srcFormat.redBits   && formatInfo.mMaxRedBits   >= srcFormat.redBits)   &&
            (formatInfo.mMinGreenBits <= srcFormat.greenBits && formatInfo.mMaxGreenBits >= srcFormat.greenBits) &&
            (formatInfo.mMinBlueBits  <= srcFormat.blueBits  && formatInfo.mMaxBlueBits  >= srcFormat.blueBits)  &&
            (formatInfo.mMinAlphaBits <= srcFormat.alphaBits && formatInfo.mMaxAlphaBits >= srcFormat.alphaBits))
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

    
    set.insert(CopyConversion(GL_ALPHA, GL_RGBA));
    set.insert(CopyConversion(GL_LUMINANCE, GL_RED));
    set.insert(CopyConversion(GL_LUMINANCE, GL_RG));
    set.insert(CopyConversion(GL_LUMINANCE, GL_RGB));
    set.insert(CopyConversion(GL_LUMINANCE, GL_RGBA));
    set.insert(CopyConversion(GL_LUMINANCE_ALPHA, GL_RGBA));
    set.insert(CopyConversion(GL_RED, GL_RED));
    set.insert(CopyConversion(GL_RED, GL_RG));
    set.insert(CopyConversion(GL_RED, GL_RGB));
    set.insert(CopyConversion(GL_RED, GL_RGBA));
    set.insert(CopyConversion(GL_RG, GL_RG));
    set.insert(CopyConversion(GL_RG, GL_RGB));
    set.insert(CopyConversion(GL_RG, GL_RGBA));
    set.insert(CopyConversion(GL_RGB, GL_RGB));
    set.insert(CopyConversion(GL_RGB, GL_RGBA));
    set.insert(CopyConversion(GL_RGBA, GL_RGBA));

    
    set.insert(CopyConversion(GL_ALPHA, GL_BGRA_EXT));
    set.insert(CopyConversion(GL_LUMINANCE, GL_BGRA_EXT));
    set.insert(CopyConversion(GL_LUMINANCE_ALPHA, GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RED, GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RG, GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RGB, GL_BGRA_EXT));
    set.insert(CopyConversion(GL_RGBA, GL_BGRA_EXT));

    set.insert(CopyConversion(GL_RED_INTEGER, GL_RED_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER, GL_RG_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER, GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER, GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER, GL_RG_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER, GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER, GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RGB_INTEGER, GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RGB_INTEGER, GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RGBA_INTEGER, GL_RGBA_INTEGER));

    return set;
}

static bool IsValidES3CopyTexImageCombination(GLenum textureInternalFormat, GLenum frameBufferInternalFormat, GLuint readBufferHandle)
{
    const InternalFormat &textureInternalFormatInfo = GetInternalFormatInfo(textureInternalFormat);
    const InternalFormat &framebufferInternalFormatInfo = GetInternalFormatInfo(frameBufferInternalFormat);

    static const CopyConversionSet conversionSet = BuildValidES3CopyTexImageCombinations();
    if (conversionSet.find(CopyConversion(textureInternalFormatInfo.format, framebufferInternalFormatInfo.format)) != conversionSet.end())
    {
        
        
        
        

        if ((textureInternalFormatInfo.colorEncoding == GL_SRGB) != (framebufferInternalFormatInfo.colorEncoding == GL_SRGB))
        {
            return false;
        }

        if (((textureInternalFormatInfo.componentType == GL_INT)          != (framebufferInternalFormatInfo.componentType == GL_INT         )) ||
            ((textureInternalFormatInfo.componentType == GL_UNSIGNED_INT) != (framebufferInternalFormatInfo.componentType == GL_UNSIGNED_INT)))
        {
            return false;
        }

        if ((textureInternalFormatInfo.componentType == GL_UNSIGNED_NORMALIZED ||
             textureInternalFormatInfo.componentType == GL_SIGNED_NORMALIZED ||
             textureInternalFormatInfo.componentType == GL_FLOAT) &&
            !(framebufferInternalFormatInfo.componentType == GL_UNSIGNED_NORMALIZED ||
              framebufferInternalFormatInfo.componentType == GL_SIGNED_NORMALIZED ||
              framebufferInternalFormatInfo.componentType == GL_FLOAT))
        {
            return false;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        const InternalFormat *sourceEffectiveFormat = NULL;
        if (readBufferHandle != 0)
        {
            
            if (framebufferInternalFormatInfo.pixelBytes > 0)
            {
                sourceEffectiveFormat = &framebufferInternalFormatInfo;
            }
            else
            {
                
                
                const FormatType &typeInfo = GetFormatTypeInfo(framebufferInternalFormatInfo.format, framebufferInternalFormatInfo.type);
                sourceEffectiveFormat = &GetInternalFormatInfo(typeInfo.internalFormat);
            }
        }
        else
        {
            
            
            if (framebufferInternalFormatInfo.colorEncoding == GL_LINEAR)
            {
                GLenum effectiveFormat;
                if (GetEffectiveInternalFormat(framebufferInternalFormatInfo, textureInternalFormatInfo, &effectiveFormat))
                {
                    sourceEffectiveFormat = &GetInternalFormatInfo(effectiveFormat);
                }
                else
                {
                    return false;
                }
            }
            else if (framebufferInternalFormatInfo.colorEncoding == GL_SRGB)
            {
                
                if ((textureInternalFormatInfo.pixelBytes > 0) &&
                    (framebufferInternalFormatInfo.redBits   >= 1 && framebufferInternalFormatInfo.redBits   <= 8) &&
                    (framebufferInternalFormatInfo.greenBits >= 1 && framebufferInternalFormatInfo.greenBits <= 8) &&
                    (framebufferInternalFormatInfo.blueBits  >= 1 && framebufferInternalFormatInfo.blueBits  <= 8) &&
                    (framebufferInternalFormatInfo.alphaBits >= 1 && framebufferInternalFormatInfo.alphaBits <= 8))
                {
                    sourceEffectiveFormat = &GetInternalFormatInfo(GL_SRGB8_ALPHA8);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                UNREACHABLE();
                return false;
            }
        }

        if (textureInternalFormatInfo.pixelBytes > 0)
        {
            
            
            if (textureInternalFormatInfo.redBits   != sourceEffectiveFormat->redBits   ||
                textureInternalFormatInfo.greenBits != sourceEffectiveFormat->greenBits ||
                textureInternalFormatInfo.blueBits  != sourceEffectiveFormat->blueBits  ||
                textureInternalFormatInfo.alphaBits != sourceEffectiveFormat->alphaBits)
            {
                return false;
            }
        }


        return true; 
                     
    }

    return false;
}

bool ValidateES3CopyTexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat,
                                       bool isSubImage, GLint xoffset, GLint yoffset, GLint zoffset,
                                       GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    GLenum textureInternalFormat;
    if (!ValidateCopyTexImageParametersBase(context, target, level, internalformat, isSubImage,
                                            xoffset, yoffset, zoffset, x, y, width, height,
                                            border, &textureInternalFormat))
    {
        return false;
    }

    gl::Framebuffer *framebuffer = context->getState().getReadFramebuffer();

    if (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    if (context->getState().getReadFramebuffer()->id() != 0 && framebuffer->getSamples() != 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    gl::FramebufferAttachment *source = framebuffer->getReadColorbuffer();
    GLenum colorbufferInternalFormat = source->getInternalFormat();

    if (isSubImage)
    {
        if (!IsValidES3CopyTexImageCombination(textureInternalFormat, colorbufferInternalFormat,
                                               context->getState().getReadFramebuffer()->id()))
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }
    else
    {
        if (!gl::IsValidES3CopyTexImageCombination(internalformat, colorbufferInternalFormat,
                                                context->getState().getReadFramebuffer()->id()))
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    
    return (width > 0 && height > 0);
}

bool ValidateES3TexStorageParameters(Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                     GLsizei width, GLsizei height, GLsizei depth)
{
    if (width < 1 || height < 1 || depth < 1 || levels < 1)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (levels > gl::log2(std::max(std::max(width, height), depth)) + 1)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::Caps &caps = context->getCaps();

    gl::Texture *texture = NULL;
    switch (target)
    {
      case GL_TEXTURE_2D:
        {
            texture = context->getTexture2D();

            if (static_cast<GLuint>(width) > caps.max2DTextureSize ||
                static_cast<GLuint>(height) > caps.max2DTextureSize)
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }
        }
        break;

      case GL_TEXTURE_CUBE_MAP:
        {
            texture = context->getTextureCubeMap();

            if (width != height)
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }

            if (static_cast<GLuint>(width) > caps.maxCubeMapTextureSize)
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }
        }
        break;

      case GL_TEXTURE_3D:
        {
            texture = context->getTexture3D();

            if (static_cast<GLuint>(width) > caps.max3DTextureSize ||
                static_cast<GLuint>(height) > caps.max3DTextureSize ||
                static_cast<GLuint>(depth) > caps.max3DTextureSize)
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }
        }
        break;

      case GL_TEXTURE_2D_ARRAY:
        {
            texture = context->getTexture2DArray();

            if (static_cast<GLuint>(width) > caps.max2DTextureSize ||
                static_cast<GLuint>(height) > caps.max2DTextureSize ||
                static_cast<GLuint>(depth) > caps.maxArrayTextureLayers)
            {
                context->recordError(Error(GL_INVALID_VALUE));
                return false;
            }
        }
        break;

      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (!texture || texture->id() == 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (texture->isImmutable())
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalformat);
    if (!formatInfo.textureSupport(context->getClientVersion(), context->getExtensions()))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (formatInfo.pixelBytes == 0)
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    return true;
}

bool ValidateFramebufferTextureLayer(Context *context, GLenum target, GLenum attachment,
                                     GLuint texture, GLint level, GLint layer)
{
    if (context->getClientVersion() < 3)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (layer < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (!ValidateFramebufferTextureBase(context, target, attachment, texture, level))
    {
        return false;
    }

    const gl::Caps &caps = context->getCaps();
    if (texture != 0)
    {
        gl::Texture *tex = context->getTexture(texture);
        ASSERT(tex);

        switch (tex->getTarget())
        {
          case GL_TEXTURE_2D_ARRAY:
            {
                if (level > gl::log2(caps.max2DTextureSize))
                {
                    context->recordError(Error(GL_INVALID_VALUE));
                    return false;
                }

                if (static_cast<GLuint>(layer) >= caps.maxArrayTextureLayers)
                {
                    context->recordError(Error(GL_INVALID_VALUE));
                    return false;
                }

                gl::Texture2DArray *texArray = static_cast<gl::Texture2DArray *>(tex);
                if (texArray->isCompressed(level))
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
            break;

          case GL_TEXTURE_3D:
            {
                if (level > gl::log2(caps.max3DTextureSize))
                {
                    context->recordError(Error(GL_INVALID_VALUE));
                    return false;
                }

                if (static_cast<GLuint>(layer) >= caps.max3DTextureSize)
                {
                    context->recordError(Error(GL_INVALID_VALUE));
                    return false;
                }

                gl::Texture3D *tex3d = static_cast<gl::Texture3D *>(tex);
                if (tex3d->isCompressed(level))
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
            break;

          default:
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    return true;
}

bool ValidES3ReadFormatType(Context *context, GLenum internalFormat, GLenum format, GLenum type)
{
    const gl::InternalFormat &internalFormatInfo = gl::GetInternalFormatInfo(internalFormat);

    switch (format)
    {
      case GL_RGBA:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
            break;
          case GL_UNSIGNED_INT_2_10_10_10_REV:
            if (internalFormat != GL_RGB10_A2)
            {
                return false;
            }
            break;
          case GL_FLOAT:
            if (internalFormatInfo.componentType != GL_FLOAT)
            {
                return false;
            }
            break;
          default:
            return false;
        }
        break;
      case GL_RGBA_INTEGER:
        switch (type)
        {
          case GL_INT:
            if (internalFormatInfo.componentType != GL_INT)
            {
                return false;
            }
            break;
          case GL_UNSIGNED_INT:
            if (internalFormatInfo.componentType != GL_UNSIGNED_INT)
            {
                return false;
            }
            break;
          default:
            return false;
        }
        break;
      case GL_BGRA_EXT:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
          case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
          case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
            break;
          default:
            return false;
        }
        break;
      case GL_RG_EXT:
      case GL_RED_EXT:
        if (!context->getExtensions().textureRG)
        {
            return false;
        }
        switch (type)
        {
        case GL_UNSIGNED_BYTE:
            break;
        default:
            return false;
        }
        break;
      default:
        return false;
    }
    return true;
}

bool ValidateInvalidateFramebufferParameters(Context *context, GLenum target, GLsizei numAttachments,
                                             const GLenum* attachments)
{
    bool defaultFramebuffer = false;

    switch (target)
    {
      case GL_DRAW_FRAMEBUFFER:
      case GL_FRAMEBUFFER:
        defaultFramebuffer = context->getState().getDrawFramebuffer()->id() == 0;
        break;
      case GL_READ_FRAMEBUFFER:
        defaultFramebuffer = context->getState().getReadFramebuffer()->id() == 0;
        break;
      default:
          context->recordError(Error(GL_INVALID_ENUM));
          return false;
    }

    for (int i = 0; i < numAttachments; ++i)
    {
        if (attachments[i] >= GL_COLOR_ATTACHMENT0 && attachments[i] <= GL_COLOR_ATTACHMENT15)
        {
            if (defaultFramebuffer)
            {
                context->recordError(Error(GL_INVALID_ENUM));
                return false;
            }

            if (attachments[i] >= GL_COLOR_ATTACHMENT0 + context->getCaps().maxColorAttachments)
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }
        }
        else
        {
            switch (attachments[i])
            {
              case GL_DEPTH_ATTACHMENT:
              case GL_STENCIL_ATTACHMENT:
              case GL_DEPTH_STENCIL_ATTACHMENT:
                if (defaultFramebuffer)
                {
                    context->recordError(Error(GL_INVALID_ENUM));
                    return false;
                }
                break;
              case GL_COLOR:
              case GL_DEPTH:
              case GL_STENCIL:
                if (!defaultFramebuffer)
                {
                    context->recordError(Error(GL_INVALID_ENUM));
                    return false;
                }
                break;
              default:
                context->recordError(Error(GL_INVALID_ENUM));
                return false;
            }
        }
    }

    return true;
}

bool ValidateClearBuffer(Context *context)
{
    if (context->getClientVersion() < 3)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::Framebuffer *fbo = context->getState().getDrawFramebuffer();
    if (!fbo || fbo->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    return true;
}

bool ValidateGetUniformuiv(Context *context, GLuint program, GLint location, GLuint* params)
{
    if (context->getClientVersion() < 3)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return ValidateGetUniformBase(context, program, location);
}

}

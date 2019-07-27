







#ifndef LIBGLESV2_VALIDATION_ES2_H
#define LIBGLESV2_VALIDATION_ES2_H

#include <GLES2/gl2.h>

namespace gl
{

class Context;

bool ValidateES2TexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat, bool isCompressed, bool isSubImage,
                                   GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                   GLint border, GLenum format, GLenum type, const GLvoid *pixels);

bool ValidateES2CopyTexImageParameters(Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                       GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                       GLint border);

bool ValidateES2TexStorageParameters(Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                     GLsizei width, GLsizei height);

bool ValidES2ReadFormatType(Context *context, GLenum format, GLenum type);

}

#endif 

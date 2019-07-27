







#ifndef LIBGLESV2_VALIDATION_ES2_H
#define LIBGLESV2_VALIDATION_ES2_H

namespace gl
{

class Context;

bool ValidateES2TexImageParameters(gl::Context *context, GLenum target, GLint level, GLenum internalformat, bool isCompressed, bool isSubImage,
                                   GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                   GLint border, GLenum format, GLenum type, const GLvoid *pixels);

bool ValidateES2CopyTexImageParameters(gl::Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                       GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                       GLint border);

bool ValidateES2TexStorageParameters(gl::Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                     GLsizei width, GLsizei height);

bool ValidateES2FramebufferTextureParameters(gl::Context *context, GLenum target, GLenum attachment,
                                             GLenum textarget, GLuint texture, GLint level);

bool ValidES2ReadFormatType(gl::Context *context, GLenum format, GLenum type);

}

#endif 

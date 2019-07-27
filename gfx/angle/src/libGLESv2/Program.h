








#ifndef LIBGLESV2_PROGRAM_H_
#define LIBGLESV2_PROGRAM_H_

#include <string>
#include <set>

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "libGLESv2/Constants.h"

namespace rx
{
class Renderer;
}

namespace gl
{
class ResourceManager;
class FragmentShader;
class VertexShader;
class ProgramBinary;
class Shader;

extern const char * const g_fakepath;

class AttributeBindings
{
  public:
    AttributeBindings();
    ~AttributeBindings();

    void bindAttributeLocation(GLuint index, const char *name);
    int getAttributeBinding(const std::string &name) const;

  private:
    std::set<std::string> mAttributeBinding[MAX_VERTEX_ATTRIBS];
};

class InfoLog
{
  public:
    InfoLog();
    ~InfoLog();

    int getLength() const;
    void getLog(GLsizei bufSize, GLsizei *length, char *infoLog);

    void appendSanitized(const char *message);
    void append(const char *info, ...);
    void reset();
  private:
    DISALLOW_COPY_AND_ASSIGN(InfoLog);
    char *mInfoLog;
};

class Program
{
  public:
    Program(rx::Renderer *renderer, ResourceManager *manager, GLuint handle);

    ~Program();

    bool attachShader(Shader *shader);
    bool detachShader(Shader *shader);
    int getAttachedShadersCount() const;

    void bindAttributeLocation(GLuint index, const char *name);

    bool link();
    bool isLinked();
    bool setProgramBinary(const void *binary, GLsizei length);
    ProgramBinary *getProgramBinary() const;

    int getInfoLogLength() const;
    void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog);
    void getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders);

    void getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    GLint getActiveAttributeCount();
    GLint getActiveAttributeMaxLength();

    void getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    GLint getActiveUniformCount();
    GLint getActiveUniformMaxLength();

    GLint getActiveUniformBlockCount();
    GLint getActiveUniformBlockMaxLength();

    void bindUniformBlock(GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    GLuint getUniformBlockBinding(GLuint uniformBlockIndex) const;

    void setTransformFeedbackVaryings(GLsizei count, const GLchar *const *varyings, GLenum bufferMode);
    void getTransformFeedbackVarying(GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) const;
    GLsizei getTransformFeedbackVaryingCount() const;
    GLsizei getTransformFeedbackVaryingMaxLength() const;
    GLenum getTransformFeedbackBufferMode() const;

    void addRef();
    void release();
    unsigned int getRefCount() const;
    void flagForDeletion();
    bool isFlaggedForDeletion() const;

    void validate();
    bool isValidated() const;

    GLint getProgramBinaryLength() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Program);

    void unlink(bool destroy = false);
    void resetUniformBlockBindings();

    FragmentShader *mFragmentShader;
    VertexShader *mVertexShader;

    AttributeBindings mAttributeBindings;

    GLuint mUniformBlockBindings[IMPLEMENTATION_MAX_COMBINED_SHADER_UNIFORM_BUFFERS];

    std::vector<std::string> mTransformFeedbackVaryings;
    GLuint mTransformFeedbackBufferMode;

    BindingPointer<ProgramBinary> mProgramBinary;
    bool mLinked;
    bool mDeleteStatus;   

    unsigned int mRefCount;

    ResourceManager *mResourceManager;
    rx::Renderer *mRenderer;
    const GLuint mHandle;

    InfoLog mInfoLog;
};
}

#endif   

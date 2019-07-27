










#ifndef LIBGLESV2_SHADER_H_
#define LIBGLESV2_SHADER_H_


#include <string>
#include <list>
#include <vector>

#include "angle_gl.h"
#include <GLSLANG/ShaderLang.h>

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"

namespace rx
{
class ShaderImpl;
}

namespace gl
{
class ResourceManager;

struct PackedVarying : public sh::Varying
{
    unsigned int registerIndex; 
    unsigned int columnIndex; 

    PackedVarying(const sh::Varying &varying)
      : sh::Varying(varying),
        registerIndex(GL_INVALID_INDEX),
        columnIndex(0)
    {}

    bool registerAssigned() const { return registerIndex != GL_INVALID_INDEX; }
    bool isBuiltIn() const { return name.compare(0, 3, "gl_") == 0; }

    void resetRegisterAssignment()
    {
        registerIndex = GL_INVALID_INDEX;
    }
};

class Shader
{
  public:
    Shader(ResourceManager *manager, rx::ShaderImpl *impl, GLenum type, GLuint handle);

    virtual ~Shader();

    GLenum getType() const { return mType; }
    GLuint getHandle() const;

    rx::ShaderImpl *getImplementation() { return mShader; }
    const rx::ShaderImpl *getImplementation() const { return mShader; }

    void deleteSource();
    void setSource(GLsizei count, const char *const *string, const GLint *length);
    int getInfoLogLength() const;
    void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog) const;
    int getSourceLength() const;
    void getSource(GLsizei bufSize, GLsizei *length, char *buffer) const;
    int getTranslatedSourceLength() const;
    void getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer) const;
    void getTranslatedSourceWithDebugInfo(GLsizei bufSize, GLsizei *length, char *buffer) const;

    void compile();
    bool isCompiled() const { return mCompiled; }

    void addRef();
    void release();
    unsigned int getRefCount() const;
    bool isFlaggedForDeletion() const;
    void flagForDeletion();

    const std::vector<gl::PackedVarying> &getVaryings() const;
    const std::vector<sh::Uniform> &getUniforms() const;
    const std::vector<sh::InterfaceBlock> &getInterfaceBlocks() const;
    const std::vector<sh::Attribute> &getActiveAttributes() const;
    const std::vector<sh::Attribute> &getActiveOutputVariables() const;

    std::vector<gl::PackedVarying> &getVaryings();
    std::vector<sh::Uniform> &getUniforms();
    std::vector<sh::InterfaceBlock> &getInterfaceBlocks();
    std::vector<sh::Attribute> &getActiveAttributes();
    std::vector<sh::Attribute> &getActiveOutputVariables();

  private:
    DISALLOW_COPY_AND_ASSIGN(Shader);

    static void getSourceImpl(const std::string &source, GLsizei bufSize, GLsizei *length, char *buffer);

    rx::ShaderImpl *mShader;
    const GLuint mHandle;
    const GLenum mType;
    std::string mSource;
    unsigned int mRefCount;     
    bool mDeleteStatus;         
    bool mCompiled;             

    ResourceManager *mResourceManager;
};

}

#endif   

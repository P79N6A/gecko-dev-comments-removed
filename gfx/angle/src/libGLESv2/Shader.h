










#ifndef LIBGLESV2_SHADER_H_
#define LIBGLESV2_SHADER_H_

#include "angle_gl.h"
#include <string>
#include <list>
#include <vector>

#include "common/shadervars.h"
#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"
#include "GLSLANG/ShaderLang.h"

namespace rx
{
class Renderer;
}

namespace gl
{
class ResourceManager;

struct PackedVarying : public sh::Varying
{
    unsigned int registerIndex; 

    PackedVarying(const sh::Varying &varying)
      : sh::Varying(varying),
        registerIndex(GL_INVALID_INDEX)
    {}

    bool registerAssigned() const { return registerIndex != GL_INVALID_INDEX; }

    void resetRegisterAssignment()
    {
        registerIndex = GL_INVALID_INDEX;
    }
};

class Shader
{
    friend class DynamicHLSL;

  public:
    Shader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle);

    virtual ~Shader();

    virtual GLenum getType() = 0;
    GLuint getHandle() const;

    void deleteSource();
    void setSource(GLsizei count, const char *const *string, const GLint *length);
    int getInfoLogLength() const;
    void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog) const;
    int getSourceLength() const;
    void getSource(GLsizei bufSize, GLsizei *length, char *buffer) const;
    int getTranslatedSourceLength() const;
    void getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer) const;
    const std::vector<sh::Uniform> &getUniforms() const;
    const std::vector<sh::InterfaceBlock> &getInterfaceBlocks() const;
    std::vector<PackedVarying> &getVaryings();

    virtual void compile() = 0;
    virtual void uncompile();
    bool isCompiled() const;
    const std::string &getHLSL() const;

    void addRef();
    void release();
    unsigned int getRefCount() const;
    bool isFlaggedForDeletion() const;
    void flagForDeletion();
    int getShaderVersion() const;
    void resetVaryingsRegisterAssignment();

    static void releaseCompiler();
    static ShShaderOutput getCompilerOutputType(GLenum shader);

    bool usesDepthRange() const { return mUsesDepthRange; }
    bool usesPointSize() const { return mUsesPointSize; }
    rx::D3DWorkaroundType getD3DWorkarounds() const;

  protected:
    void parseVaryings(void *compiler);

    void compileToHLSL(void *compiler);

    void getSourceImpl(const std::string &source, GLsizei bufSize, GLsizei *length, char *buffer) const;

    static bool compareVarying(const PackedVarying &x, const PackedVarying &y);

    const rx::Renderer *const mRenderer;

    std::vector<PackedVarying> mVaryings;

    bool mUsesMultipleRenderTargets;
    bool mUsesFragColor;
    bool mUsesFragData;
    bool mUsesFragCoord;
    bool mUsesFrontFacing;
    bool mUsesPointSize;
    bool mUsesPointCoord;
    bool mUsesDepthRange;
    bool mUsesFragDepth;
    int mShaderVersion;
    bool mUsesDiscardRewriting;
    bool mUsesNestedBreak;

    static void *mFragmentCompiler;
    static void *mVertexCompiler;

  private:
    DISALLOW_COPY_AND_ASSIGN(Shader);

    void initializeCompiler();

    const GLuint mHandle;
    unsigned int mRefCount;     
    bool mDeleteStatus;         

    std::string mSource;
    std::string mHlsl;
    std::string mInfoLog;
    std::vector<sh::Uniform> mActiveUniforms;
    std::vector<sh::InterfaceBlock> mActiveInterfaceBlocks;

    ResourceManager *mResourceManager;
};

class VertexShader : public Shader
{
    friend class DynamicHLSL;

  public:
    VertexShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle);

    ~VertexShader();

    virtual GLenum getType();
    virtual void compile();
    virtual void uncompile();
    int getSemanticIndex(const std::string &attributeName);

    const std::vector<sh::Attribute> &activeAttributes() const { return mActiveAttributes; }

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexShader);

    void parseAttributes();

    std::vector<sh::Attribute> mActiveAttributes;
};

class FragmentShader : public Shader
{
  public:
    FragmentShader(ResourceManager *manager,const rx::Renderer *renderer, GLuint handle);

    ~FragmentShader();

    virtual GLenum getType();
    virtual void compile();
    virtual void uncompile();
    const std::vector<sh::Attribute> &getOutputVariables() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(FragmentShader);

    std::vector<sh::Attribute> mActiveOutputVariables;
};
}

#endif   

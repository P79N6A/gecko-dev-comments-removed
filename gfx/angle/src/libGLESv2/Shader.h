










#ifndef LIBGLESV2_SHADER_H_
#define LIBGLESV2_SHADER_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <string>
#include <list>
#include <vector>

#include "compiler/CompilerUniform.h"
#include "common/angleutils.h"

namespace rx
{
class Renderer;
}

namespace gl
{
class ResourceManager;

struct Varying
{
    Varying(GLenum type, const std::string &name, int size, bool array)
        : type(type), name(name), size(size), array(array), reg(-1), col(-1)
    {
    }

    GLenum type;
    std::string name;
    int size;   
    bool array;

    int reg;    
    int col;    
};

typedef std::list<Varying> VaryingList;

class Shader
{
    friend class ProgramBinary;

  public:
    Shader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle);

    virtual ~Shader();

    virtual GLenum getType() = 0;
    GLuint getHandle() const;

    void deleteSource();
    void setSource(GLsizei count, const char **string, const GLint *length);
    int getInfoLogLength() const;
    void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog);
    int getSourceLength() const;
    void getSource(GLsizei bufSize, GLsizei *length, char *buffer);
    int getTranslatedSourceLength() const;
    void getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer);
    const sh::ActiveUniforms &getUniforms();

    virtual void compile() = 0;
    virtual void uncompile();
    bool isCompiled();
    const char *getHLSL();

    void addRef();
    void release();
    unsigned int getRefCount() const;
    bool isFlaggedForDeletion() const;
    void flagForDeletion();

    static void releaseCompiler();

  protected:
    void parseVaryings();
    void resetVaryingsRegisterAssignment();

    void compileToHLSL(void *compiler);

    void getSourceImpl(char *source, GLsizei bufSize, GLsizei *length, char *buffer);

    static GLenum parseType(const std::string &type);
    static bool compareVarying(const Varying &x, const Varying &y);

    const rx::Renderer *const mRenderer;

    VaryingList mVaryings;

    bool mUsesMultipleRenderTargets;
    bool mUsesFragColor;
    bool mUsesFragData;
    bool mUsesFragCoord;
    bool mUsesFrontFacing;
    bool mUsesPointSize;
    bool mUsesPointCoord;
    bool mUsesDepthRange;
    bool mUsesFragDepth;

    static void *mFragmentCompiler;
    static void *mVertexCompiler;

  private:
    DISALLOW_COPY_AND_ASSIGN(Shader);

    void initializeCompiler();

    const GLuint mHandle;
    unsigned int mRefCount;     
    bool mDeleteStatus;         

    char *mSource;
    char *mHlsl;
    char *mInfoLog;
    sh::ActiveUniforms mActiveUniforms;

    ResourceManager *mResourceManager;
};

struct Attribute
{
    Attribute() : type(GL_NONE), name("")
    {
    }

    Attribute(GLenum type, const std::string &name) : type(type), name(name)
    {
    }

    GLenum type;
    std::string name;
};

typedef std::vector<Attribute> AttributeArray;

class VertexShader : public Shader
{
    friend class ProgramBinary;

  public:
    VertexShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle);

    ~VertexShader();

    virtual GLenum getType();
    virtual void compile();
    virtual void uncompile();
    int getSemanticIndex(const std::string &attributeName);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexShader);

    void parseAttributes();

    AttributeArray mAttributes;
};

class FragmentShader : public Shader
{
  public:
    FragmentShader(ResourceManager *manager,const rx::Renderer *renderer, GLuint handle);

    ~FragmentShader();

    virtual GLenum getType();
    virtual void compile();

  private:
    DISALLOW_COPY_AND_ASSIGN(FragmentShader);
};
}

#endif   

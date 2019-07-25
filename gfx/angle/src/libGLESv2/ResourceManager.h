








#ifndef LIBGLESV2_RESOURCEMANAGER_H_
#define LIBGLESV2_RESOURCEMANAGER_H_

#define GL_APICALL
#include <GLES2/gl2.h>

#include <map>

#include "common/angleutils.h"

namespace gl
{
class Buffer;
class Shader;
class Program;
class Texture;
class Renderbuffer;

enum SamplerType
{
    SAMPLER_2D,
    SAMPLER_CUBE,

    SAMPLER_TYPE_COUNT
};

class ResourceManager
{
  public:
    ResourceManager();
    ~ResourceManager();

    void addRef();
    void release();

    GLuint createBuffer();
    GLuint createShader(GLenum type);
    GLuint createProgram();
    GLuint createTexture();
    GLuint createRenderbuffer();

    void deleteBuffer(GLuint buffer);
    void deleteShader(GLuint shader);
    void deleteProgram(GLuint program);
    void deleteTexture(GLuint texture);
    void deleteRenderbuffer(GLuint renderbuffer);

    Buffer *getBuffer(GLuint handle);
    Shader *getShader(GLuint handle);
    Program *getProgram(GLuint handle);
    Texture *getTexture(GLuint handle);
    Renderbuffer *getRenderbuffer(GLuint handle);
    
    void setRenderbuffer(GLuint handle, Renderbuffer *renderbuffer);

    void checkBufferAllocation(unsigned int buffer);
    void checkTextureAllocation(GLuint texture, SamplerType type);
    void checkRenderbufferAllocation(GLuint renderbuffer);

  private:
    DISALLOW_COPY_AND_ASSIGN(ResourceManager);

    std::size_t mRefCount;

    typedef std::map<GLuint, Buffer*> BufferMap;
    BufferMap mBufferMap;

    typedef std::map<GLuint, Shader*> ShaderMap;
    ShaderMap mShaderMap;

    typedef std::map<GLuint, Program*> ProgramMap;
    ProgramMap mProgramMap;

    typedef std::map<GLuint, Texture*> TextureMap;
    TextureMap mTextureMap;

    typedef std::map<GLuint, Renderbuffer*> RenderbufferMap;
    RenderbufferMap mRenderbufferMap;
};

}

#endif 

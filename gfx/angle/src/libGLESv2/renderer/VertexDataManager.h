








#ifndef LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_
#define LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_

#include "libGLESv2/Constants.h"
#include "libGLESv2/VertexAttribute.h"
#include "common/angleutils.h"

namespace gl
{
class VertexAttribute;
class ProgramBinary;
struct VertexAttribCurrentValueData;
}

namespace rx
{
class BufferStorage;
class StreamingVertexBufferInterface;
class VertexBuffer;
class Renderer;

struct TranslatedAttribute
{
    bool active;

    const gl::VertexAttribute *attribute;
    GLenum currentValueType;
    unsigned int offset;
    unsigned int stride;   

    VertexBuffer *vertexBuffer;
    BufferStorage *storage;
    unsigned int serial;
    unsigned int divisor;
};

class VertexDataManager
{
  public:
    VertexDataManager(rx::Renderer *renderer);
    virtual ~VertexDataManager();

    GLenum prepareVertexData(const gl::VertexAttribute attribs[], const gl::VertexAttribCurrentValueData currentValues[],
                             gl::ProgramBinary *programBinary, GLint start, GLsizei count, TranslatedAttribute *outAttribs, GLsizei instances);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexDataManager);

    rx::Renderer *const mRenderer;

    StreamingVertexBufferInterface *mStreamingBuffer;

    gl::VertexAttribCurrentValueData mCurrentValue[gl::MAX_VERTEX_ATTRIBS];

    StreamingVertexBufferInterface *mCurrentValueBuffer[gl::MAX_VERTEX_ATTRIBS];
    std::size_t mCurrentValueOffsets[gl::MAX_VERTEX_ATTRIBS];
};

}

#endif   

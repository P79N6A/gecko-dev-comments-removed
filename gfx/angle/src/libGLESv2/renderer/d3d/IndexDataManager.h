








#ifndef LIBGLESV2_INDEXDATAMANAGER_H_
#define LIBGLESV2_INDEXDATAMANAGER_H_

#include "common/angleutils.h"
#include "common/mathutil.h"
#include "libGLESv2/Error.h"

#include <GLES2/gl2.h>

namespace
{
    enum { INITIAL_INDEX_BUFFER_SIZE = 4096 * sizeof(GLuint) };
}

namespace gl
{
class Buffer;
}

namespace rx
{
class IndexBufferInterface;
class StaticIndexBufferInterface;
class StreamingIndexBufferInterface;
class IndexBuffer;
class BufferD3D;
class Renderer;

struct TranslatedIndexData
{
    RangeUI indexRange;
    unsigned int startIndex;
    unsigned int startOffset;   

    IndexBuffer *indexBuffer;
    BufferD3D *storage;
    GLenum indexType;
    unsigned int serial;
};

class IndexDataManager
{
  public:
    explicit IndexDataManager(Renderer *renderer);
    virtual ~IndexDataManager();

    gl::Error prepareIndexData(GLenum type, GLsizei count, gl::Buffer *arrayElementBuffer, const GLvoid *indices, TranslatedIndexData *translated);

  private:
     gl::Error getStreamingIndexBuffer(GLenum destinationIndexType, IndexBufferInterface **outBuffer);

    DISALLOW_COPY_AND_ASSIGN(IndexDataManager);

    Renderer *const mRenderer;

    StreamingIndexBufferInterface *mStreamingBufferShort;
    StreamingIndexBufferInterface *mStreamingBufferInt;
};

}

#endif   

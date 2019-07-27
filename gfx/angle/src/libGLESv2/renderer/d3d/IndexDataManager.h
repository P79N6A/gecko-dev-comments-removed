








#ifndef LIBGLESV2_INDEXDATAMANAGER_H_
#define LIBGLESV2_INDEXDATAMANAGER_H_

#include "common/angleutils.h"

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
class StaticIndexBufferInterface;
class StreamingIndexBufferInterface;
class IndexBuffer;
class BufferD3D;
class Renderer;

struct TranslatedIndexData
{
    unsigned int minIndex;
    unsigned int maxIndex;
    unsigned int startIndex;
    unsigned int startOffset;   

    IndexBuffer *indexBuffer;
    BufferD3D *storage;
    unsigned int serial;
};

class IndexDataManager
{
  public:
    explicit IndexDataManager(Renderer *renderer);
    virtual ~IndexDataManager();

    GLenum prepareIndexData(GLenum type, GLsizei count, gl::Buffer *arrayElementBuffer, const GLvoid *indices, TranslatedIndexData *translated);
    StaticIndexBufferInterface *getCountingIndices(GLsizei count);

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexDataManager);

    Renderer *const mRenderer;

    StreamingIndexBufferInterface *mStreamingBufferShort;
    StreamingIndexBufferInterface *mStreamingBufferInt;
    StaticIndexBufferInterface *mCountingBuffer;
};

}

#endif   

#include "precompiled.h"









#include "libGLESv2/renderer/d3d/IndexDataManager.h"
#include "libGLESv2/renderer/d3d/BufferD3D.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/main.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/renderer/d3d/IndexBuffer.h"

namespace rx
{

IndexDataManager::IndexDataManager(Renderer *renderer) : mRenderer(renderer)
{
    mStreamingBufferShort = new StreamingIndexBufferInterface(mRenderer);
    if (!mStreamingBufferShort->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_SHORT))
    {
        delete mStreamingBufferShort;
        mStreamingBufferShort = NULL;
    }

    mStreamingBufferInt = new StreamingIndexBufferInterface(mRenderer);
    if (!mStreamingBufferInt->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_INT))
    {
        delete mStreamingBufferInt;
        mStreamingBufferInt = NULL;
    }

    if (!mStreamingBufferShort)
    {
        
        delete mStreamingBufferInt;
        mStreamingBufferInt = NULL;

        ERR("Failed to allocate the streaming index buffer(s).");
    }

    mCountingBuffer = NULL;
}

IndexDataManager::~IndexDataManager()
{
    delete mStreamingBufferShort;
    delete mStreamingBufferInt;
    delete mCountingBuffer;
}

static void convertIndices(GLenum type, const void *input, GLsizei count, void *output)
{
    if (type == GL_UNSIGNED_BYTE)
    {
        const GLubyte *in = static_cast<const GLubyte*>(input);
        GLushort *out = static_cast<GLushort*>(output);

        for (GLsizei i = 0; i < count; i++)
        {
            out[i] = in[i];
        }
    }
    else if (type == GL_UNSIGNED_INT)
    {
        memcpy(output, input, count * sizeof(GLuint));
    }
    else if (type == GL_UNSIGNED_SHORT)
    {
        memcpy(output, input, count * sizeof(GLushort));
    }
    else UNREACHABLE();
}

template <class IndexType>
static void computeRange(const IndexType *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
    *minIndex = indices[0];
    *maxIndex = indices[0];

    for (GLsizei i = 0; i < count; i++)
    {
        if (*minIndex > indices[i]) *minIndex = indices[i];
        if (*maxIndex < indices[i]) *maxIndex = indices[i];
    }
}

static void computeRange(GLenum type, const GLvoid *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
    if (type == GL_UNSIGNED_BYTE)
    {
        computeRange(static_cast<const GLubyte*>(indices), count, minIndex, maxIndex);
    }
    else if (type == GL_UNSIGNED_INT)
    {
        computeRange(static_cast<const GLuint*>(indices), count, minIndex, maxIndex);
    }
    else if (type == GL_UNSIGNED_SHORT)
    {
        computeRange(static_cast<const GLushort*>(indices), count, minIndex, maxIndex);
    }
    else UNREACHABLE();
}

GLenum IndexDataManager::prepareIndexData(GLenum type, GLsizei count, gl::Buffer *buffer, const GLvoid *indices, TranslatedIndexData *translated)
{
    if (!mStreamingBufferShort)
    {
        return GL_OUT_OF_MEMORY;
    }

    GLenum destinationIndexType = (type == GL_UNSIGNED_INT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    unsigned int offset = 0;
    bool alignedOffset = false;

    BufferD3D *storage = NULL;

    if (buffer != NULL)
    {
        if (reinterpret_cast<uintptr_t>(indices) > std::numeric_limits<unsigned int>::max())
        {
            return GL_OUT_OF_MEMORY;
        }
        offset = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(indices));

        storage = BufferD3D::makeBufferD3D(buffer->getImplementation());

        switch (type)
        {
          case GL_UNSIGNED_BYTE:  alignedOffset = (offset % sizeof(GLubyte) == 0);  break;
          case GL_UNSIGNED_SHORT: alignedOffset = (offset % sizeof(GLushort) == 0); break;
          case GL_UNSIGNED_INT:   alignedOffset = (offset % sizeof(GLuint) == 0);   break;
          default: UNREACHABLE(); alignedOffset = false;
        }

        unsigned int typeSize = gl::GetTypeBytes(type);

        
        if (static_cast<unsigned int>(count) > (std::numeric_limits<unsigned int>::max() / typeSize) ||
            typeSize * static_cast<unsigned int>(count) + offset < offset)
        {
            return GL_OUT_OF_MEMORY;
        }

        if (typeSize * static_cast<unsigned int>(count) + offset > storage->getSize())
        {
            return GL_INVALID_OPERATION;
        }

        indices = static_cast<const GLubyte*>(storage->getData()) + offset;
    }

    StreamingIndexBufferInterface *streamingBuffer = (type == GL_UNSIGNED_INT) ? mStreamingBufferInt : mStreamingBufferShort;

    StaticIndexBufferInterface *staticBuffer = storage ? storage->getStaticIndexBuffer() : NULL;
    IndexBufferInterface *indexBuffer = streamingBuffer;
    bool directStorage = alignedOffset && storage && storage->supportsDirectBinding() &&
                         destinationIndexType == type;
    unsigned int streamOffset = 0;

    if (directStorage)
    {
        indexBuffer = streamingBuffer;
        streamOffset = offset;

        if (!storage->getIndexRangeCache()->findRange(type, offset, count, &translated->minIndex,
                                                     &translated->maxIndex, NULL))
        {
            computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);
            storage->getIndexRangeCache()->addRange(type, offset, count, translated->minIndex,
                                                   translated->maxIndex, offset);
        }
    }
    else if (staticBuffer && staticBuffer->getBufferSize() != 0 && staticBuffer->getIndexType() == type && alignedOffset)
    {
        indexBuffer = staticBuffer;

        if (!staticBuffer->getIndexRangeCache()->findRange(type, offset, count, &translated->minIndex,
                                                           &translated->maxIndex, &streamOffset))
        {
            streamOffset = (offset / gl::GetTypeBytes(type)) * gl::GetTypeBytes(destinationIndexType);
            computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);
            staticBuffer->getIndexRangeCache()->addRange(type, offset, count, translated->minIndex,
                                                         translated->maxIndex, streamOffset);
        }
    }
    else
    {
        unsigned int convertCount = count;

        if (staticBuffer)
        {
            if (staticBuffer->getBufferSize() == 0 && alignedOffset)
            {
                indexBuffer = staticBuffer;
                convertCount = storage->getSize() / gl::GetTypeBytes(type);
            }
            else
            {
                storage->invalidateStaticData();
                staticBuffer = NULL;
            }
        }

        if (!indexBuffer)
        {
            ERR("No valid index buffer.");
            return GL_INVALID_OPERATION;
        }

        unsigned int indexTypeSize = gl::GetTypeBytes(destinationIndexType);
        if (convertCount > std::numeric_limits<unsigned int>::max() / indexTypeSize)
        {
            ERR("Reserving %u indicies of %u bytes each exceeds the maximum buffer size.", convertCount, indexTypeSize);
            return GL_OUT_OF_MEMORY;
        }

        unsigned int bufferSizeRequired = convertCount * indexTypeSize;
        if (!indexBuffer->reserveBufferSpace(bufferSizeRequired, type))
        {
            ERR("Failed to reserve %u bytes in an index buffer.", bufferSizeRequired);
            return GL_OUT_OF_MEMORY;
        }

        void* output = NULL;
        if (!indexBuffer->mapBuffer(bufferSizeRequired, &output, &streamOffset))
        {
            ERR("Failed to map index buffer.");
            return GL_OUT_OF_MEMORY;
        }

        convertIndices(type, staticBuffer ? storage->getData() : indices, convertCount, output);

        if (!indexBuffer->unmapBuffer())
        {
            ERR("Failed to unmap index buffer.");
            return GL_OUT_OF_MEMORY;
        }

        computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);

        if (staticBuffer)
        {
            streamOffset = (offset / gl::GetTypeBytes(type)) * gl::GetTypeBytes(destinationIndexType);
            staticBuffer->getIndexRangeCache()->addRange(type, offset, count, translated->minIndex,
                                                         translated->maxIndex, streamOffset);
        }
    }

    translated->storage = directStorage ? storage : NULL;
    translated->indexBuffer = indexBuffer->getIndexBuffer();
    translated->serial = directStorage ? storage->getSerial() : indexBuffer->getSerial();
    translated->startIndex = streamOffset / gl::GetTypeBytes(destinationIndexType);
    translated->startOffset = streamOffset;

    if (storage)
    {
        storage->promoteStaticUsage(count * gl::GetTypeBytes(type));
    }

    return GL_NO_ERROR;
}

StaticIndexBufferInterface *IndexDataManager::getCountingIndices(GLsizei count)
{
    if (count <= 65536)   
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned short);

        if (!mCountingBuffer || mCountingBuffer->getBufferSize() < spaceNeeded)
        {
            delete mCountingBuffer;
            mCountingBuffer = new StaticIndexBufferInterface(mRenderer);
            mCountingBuffer->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_SHORT);

            void* mappedMemory = NULL;
            if (!mCountingBuffer->mapBuffer(spaceNeeded, &mappedMemory, NULL))
            {
                ERR("Failed to map counting buffer.");
                return NULL;
            }

            unsigned short *data = reinterpret_cast<unsigned short*>(mappedMemory);
            for(int i = 0; i < count; i++)
            {
                data[i] = i;
            }

            if (!mCountingBuffer->unmapBuffer())
            {
                ERR("Failed to unmap counting buffer.");
                return NULL;
            }
        }
    }
    else if (mStreamingBufferInt)   
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned int);

        if (!mCountingBuffer || mCountingBuffer->getBufferSize() < spaceNeeded)
        {
            delete mCountingBuffer;
            mCountingBuffer = new StaticIndexBufferInterface(mRenderer);
            mCountingBuffer->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_INT);

            void* mappedMemory = NULL;
            if (!mCountingBuffer->mapBuffer(spaceNeeded, &mappedMemory, NULL))
            {
                ERR("Failed to map counting buffer.");
                return NULL;
            }

            unsigned int *data = reinterpret_cast<unsigned int*>(mappedMemory);
            for(int i = 0; i < count; i++)
            {
                data[i] = i;
            }

            if (!mCountingBuffer->unmapBuffer())
            {
                ERR("Failed to unmap counting buffer.");
                return NULL;
            }
        }
    }
    else
    {
        return NULL;
    }

    return mCountingBuffer;
}

}

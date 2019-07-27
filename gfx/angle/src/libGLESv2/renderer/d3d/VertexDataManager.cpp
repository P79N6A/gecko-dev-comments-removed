#include "precompiled.h"









#include "libGLESv2/renderer/d3d/VertexDataManager.h"
#include "libGLESv2/renderer/d3d/BufferD3D.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/renderer/d3d/VertexBuffer.h"
#include "libGLESv2/renderer/Renderer.h"

namespace
{
    enum { INITIAL_STREAM_BUFFER_SIZE = 1024*1024 };
    
    enum { CONSTANT_VERTEX_BUFFER_SIZE = 4096 };
}

namespace rx
{

static int ElementsInBuffer(const gl::VertexAttribute &attrib, unsigned int size)
{
    
    if (size > static_cast<unsigned int>(std::numeric_limits<int>::max()))
    {
        size = static_cast<unsigned int>(std::numeric_limits<int>::max());
    }

    GLsizei stride = ComputeVertexAttributeStride(attrib);
    return (size - attrib.offset % stride + (stride - ComputeVertexAttributeTypeSize(attrib))) / stride;
}

static int StreamingBufferElementCount(const gl::VertexAttribute &attrib, int vertexDrawCount, int instanceDrawCount)
{
    
    
    
    
    if (instanceDrawCount > 0 && attrib.divisor > 0)
    {
        return instanceDrawCount / attrib.divisor;
    }

    return vertexDrawCount;
}

VertexDataManager::VertexDataManager(Renderer *renderer) : mRenderer(renderer)
{
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mCurrentValue[i].FloatValues[0] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].FloatValues[1] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].FloatValues[2] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].FloatValues[3] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].Type = GL_FLOAT;
        mCurrentValueBuffer[i] = NULL;
        mCurrentValueOffsets[i] = 0;
    }

    mStreamingBuffer = new StreamingVertexBufferInterface(renderer, INITIAL_STREAM_BUFFER_SIZE);

    if (!mStreamingBuffer)
    {
        ERR("Failed to allocate the streaming vertex buffer.");
    }
}

VertexDataManager::~VertexDataManager()
{
    delete mStreamingBuffer;

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        delete mCurrentValueBuffer[i];
    }
}

GLenum VertexDataManager::prepareVertexData(const gl::VertexAttribute attribs[], const gl::VertexAttribCurrentValueData currentValues[],
                                            gl::ProgramBinary *programBinary, GLint start, GLsizei count, TranslatedAttribute *translated, GLsizei instances)
{
    if (!mStreamingBuffer)
    {
        return GL_OUT_OF_MEMORY;
    }

    for (int attributeIndex = 0; attributeIndex < gl::MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        translated[attributeIndex].active = (programBinary->getSemanticIndex(attributeIndex) != -1);
    }

    
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].enabled)
        {
            gl::Buffer *buffer = attribs[i].buffer.get();

            if (buffer)
            {
                BufferD3D *bufferImpl = BufferD3D::makeBufferD3D(buffer->getImplementation());
                StaticVertexBufferInterface *staticBuffer = bufferImpl->getStaticVertexBuffer();

                if (staticBuffer && staticBuffer->getBufferSize() > 0 && !staticBuffer->lookupAttribute(attribs[i], NULL) &&
                    !staticBuffer->directStoragePossible(attribs[i], currentValues[i]))
                {
                    bufferImpl->invalidateStaticData();
                }
            }
        }
    }

    
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].enabled)
        {
            gl::Buffer *buffer = attribs[i].buffer.get();
            if (buffer)
            {
                BufferD3D *bufferImpl = BufferD3D::makeBufferD3D(buffer->getImplementation());
                StaticVertexBufferInterface *staticBuffer = bufferImpl->getStaticVertexBuffer();
                VertexBufferInterface *vertexBuffer = staticBuffer ? staticBuffer : static_cast<VertexBufferInterface*>(mStreamingBuffer);

                if (!vertexBuffer->directStoragePossible(attribs[i], currentValues[i]))
                {
                    if (staticBuffer)
                    {
                        if (staticBuffer->getBufferSize() == 0)
                        {
                            int totalCount = ElementsInBuffer(attribs[i], bufferImpl->getSize());
                            if (!staticBuffer->reserveVertexSpace(attribs[i], totalCount, 0))
                            {
                                return GL_OUT_OF_MEMORY;
                            }
                        }
                    }
                    else
                    {
                        int totalCount = StreamingBufferElementCount(attribs[i], count, instances);

                        
                        
                        if (bufferImpl && ElementsInBuffer(attribs[i], bufferImpl->getSize()) < totalCount)
                        {
                            return GL_INVALID_OPERATION;
                        }

                        if (!mStreamingBuffer->reserveVertexSpace(attribs[i], totalCount, instances))
                        {
                            return GL_OUT_OF_MEMORY;
                        }
                    }
                }
            }
        }
    }

    
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active)
        {
            if (attribs[i].enabled)
            {
                gl::Buffer *buffer = attribs[i].buffer.get();

                if (!buffer && attribs[i].pointer == NULL)
                {
                    
                    ERR("An enabled vertex array has no buffer and no pointer.");
                    return GL_INVALID_OPERATION;
                }

                BufferD3D *storage = buffer ? BufferD3D::makeBufferD3D(buffer->getImplementation()) : NULL;
                StaticVertexBufferInterface *staticBuffer = storage ? storage->getStaticVertexBuffer() : NULL;
                VertexBufferInterface *vertexBuffer = staticBuffer ? staticBuffer : static_cast<VertexBufferInterface*>(mStreamingBuffer);
                bool directStorage = vertexBuffer->directStoragePossible(attribs[i], currentValues[i]);

                unsigned int streamOffset = 0;
                unsigned int outputElementSize = 0;

                if (directStorage)
                {
                    outputElementSize = ComputeVertexAttributeStride(attribs[i]);
                    streamOffset = attribs[i].offset + outputElementSize * start;
                }
                else if (staticBuffer)
                {
                    if (!staticBuffer->getVertexBuffer()->getSpaceRequired(attribs[i], 1, 0, &outputElementSize))
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    if (!staticBuffer->lookupAttribute(attribs[i], &streamOffset))
                    {
                        
                        int totalCount = ElementsInBuffer(attribs[i], storage->getSize());
                        int startIndex = attribs[i].offset / ComputeVertexAttributeStride(attribs[i]);

                        if (!staticBuffer->storeVertexAttributes(attribs[i], currentValues[i], -startIndex, totalCount,
                                                                 0, &streamOffset))
                        {
                            return GL_OUT_OF_MEMORY;
                        }
                    }

                    unsigned int firstElementOffset = (attribs[i].offset / ComputeVertexAttributeStride(attribs[i])) * outputElementSize;
                    unsigned int startOffset = (instances == 0 || attribs[i].divisor == 0) ? start * outputElementSize : 0;
                    if (streamOffset + firstElementOffset + startOffset < streamOffset)
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    streamOffset += firstElementOffset + startOffset;
                }
                else
                {
                    int totalCount = StreamingBufferElementCount(attribs[i], count, instances);
                    if (!mStreamingBuffer->getVertexBuffer()->getSpaceRequired(attribs[i], 1, 0, &outputElementSize) ||
                        !mStreamingBuffer->storeVertexAttributes(attribs[i], currentValues[i], start, totalCount, instances,
                                                                 &streamOffset))
                    {
                        return GL_OUT_OF_MEMORY;
                    }
                }

                translated[i].storage = directStorage ? storage : NULL;
                translated[i].vertexBuffer = vertexBuffer->getVertexBuffer();
                translated[i].serial = directStorage ? storage->getSerial() : vertexBuffer->getSerial();
                translated[i].divisor = attribs[i].divisor;

                translated[i].attribute = &attribs[i];
                translated[i].currentValueType = currentValues[i].Type;
                translated[i].stride = outputElementSize;
                translated[i].offset = streamOffset;
            }
            else
            {
                if (!mCurrentValueBuffer[i])
                {
                    mCurrentValueBuffer[i] = new StreamingVertexBufferInterface(mRenderer, CONSTANT_VERTEX_BUFFER_SIZE);
                }

                StreamingVertexBufferInterface *buffer = mCurrentValueBuffer[i];

                if (mCurrentValue[i] != currentValues[i])
                {
                    if (!buffer->reserveVertexSpace(attribs[i], 1, 0))
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    unsigned int streamOffset;
                    if (!buffer->storeVertexAttributes(attribs[i], currentValues[i], 0, 1, 0, &streamOffset))
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    mCurrentValue[i] = currentValues[i];
                    mCurrentValueOffsets[i] = streamOffset;
                }

                translated[i].storage = NULL;
                translated[i].vertexBuffer = mCurrentValueBuffer[i]->getVertexBuffer();
                translated[i].serial = mCurrentValueBuffer[i]->getSerial();
                translated[i].divisor = 0;

                translated[i].attribute = &attribs[i];
                translated[i].currentValueType = currentValues[i].Type;
                translated[i].stride = 0;
                translated[i].offset = mCurrentValueOffsets[i];
            }
        }
    }

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].enabled)
        {
            gl::Buffer *buffer = attribs[i].buffer.get();

            if (buffer)
            {
                BufferD3D *bufferImpl = BufferD3D::makeBufferD3D(buffer->getImplementation());
                bufferImpl->promoteStaticUsage(count * ComputeVertexAttributeTypeSize(attribs[i]));
            }
        }
    }

    return GL_NO_ERROR;
}

}

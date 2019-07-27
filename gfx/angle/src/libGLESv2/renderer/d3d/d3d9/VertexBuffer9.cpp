







#include "libGLESv2/renderer/d3d/d3d9/VertexBuffer9.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/renderer/vertexconversion.h"
#include "libGLESv2/renderer/BufferImpl.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/Buffer.h"

namespace rx
{

VertexBuffer9::VertexBuffer9(rx::Renderer9 *renderer) : mRenderer(renderer)
{
    mVertexBuffer = NULL;
    mBufferSize = 0;
    mDynamicUsage = false;
}

VertexBuffer9::~VertexBuffer9()
{
    SafeRelease(mVertexBuffer);
}

bool VertexBuffer9::initialize(unsigned int size, bool dynamicUsage)
{
    SafeRelease(mVertexBuffer);

    updateSerial();

    if (size > 0)
    {
        DWORD flags = D3DUSAGE_WRITEONLY;
        if (dynamicUsage)
        {
            flags |= D3DUSAGE_DYNAMIC;
        }

        HRESULT result = mRenderer->createVertexBuffer(size, flags, &mVertexBuffer);

        if (FAILED(result))
        {
            ERR("Out of memory allocating a vertex buffer of size %lu.", size);
            return false;
        }
    }

    mBufferSize = size;
    mDynamicUsage = dynamicUsage;
    return true;
}

VertexBuffer9 *VertexBuffer9::makeVertexBuffer9(VertexBuffer *vertexBuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(VertexBuffer9*, vertexBuffer));
    return static_cast<VertexBuffer9*>(vertexBuffer);
}

bool VertexBuffer9::storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                          GLint start, GLsizei count, GLsizei instances, unsigned int offset)
{
    if (mVertexBuffer)
    {
        gl::Buffer *buffer = attrib.buffer.get();

        int inputStride = gl::ComputeVertexAttributeStride(attrib);
        int elementSize = gl::ComputeVertexAttributeTypeSize(attrib);

        DWORD lockFlags = mDynamicUsage ? D3DLOCK_NOOVERWRITE : 0;

        uint8_t *mapPtr = NULL;

        unsigned int mapSize;
        if (!spaceRequired(attrib, count, instances, &mapSize))
        {
            return false;
        }

        HRESULT result = mVertexBuffer->Lock(offset, mapSize, reinterpret_cast<void**>(&mapPtr), lockFlags);

        if (FAILED(result))
        {
            ERR("Lock failed with error 0x%08x", result);
            return false;
        }

        const uint8_t *input = NULL;
        if (attrib.enabled)
        {
            if (buffer)
            {
                BufferImpl *storage = buffer->getImplementation();
                input = static_cast<const uint8_t*>(storage->getData()) + static_cast<int>(attrib.offset);
            }
            else
            {
                input = static_cast<const uint8_t*>(attrib.pointer);
            }
        }
        else
        {
            input = reinterpret_cast<const uint8_t*>(currentValue.FloatValues);
        }

        if (instances == 0 || attrib.divisor == 0)
        {
            input += inputStride * start;
        }

        gl::VertexFormat vertexFormat(attrib, currentValue.Type);
        const d3d9::VertexFormat &d3dVertexInfo = d3d9::GetVertexFormatInfo(mRenderer->getCapsDeclTypes(), vertexFormat);
        bool needsConversion = (d3dVertexInfo.conversionType & VERTEX_CONVERT_CPU) > 0;

        if (!needsConversion && inputStride == elementSize)
        {
            size_t copySize = static_cast<size_t>(count) * static_cast<size_t>(inputStride);
            memcpy(mapPtr, input, copySize);
        }
        else
        {
            d3dVertexInfo.copyFunction(input, inputStride, count, mapPtr);
        }

        mVertexBuffer->Unlock();

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

bool VertexBuffer9::getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances,
                                     unsigned int *outSpaceRequired) const
{
    return spaceRequired(attrib, count, instances, outSpaceRequired);
}

unsigned int VertexBuffer9::getBufferSize() const
{
    return mBufferSize;
}

bool VertexBuffer9::setBufferSize(unsigned int size)
{
    if (size > mBufferSize)
    {
        return initialize(size, mDynamicUsage);
    }
    else
    {
        return true;
    }
}

bool VertexBuffer9::discard()
{
    if (mVertexBuffer)
    {
        void *dummy;
        HRESULT result;

        result = mVertexBuffer->Lock(0, 1, &dummy, D3DLOCK_DISCARD);
        if (FAILED(result))
        {
            ERR("Discard lock failed with error 0x%08x", result);
            return false;
        }

        result = mVertexBuffer->Unlock();
        if (FAILED(result))
        {
            ERR("Discard unlock failed with error 0x%08x", result);
            return false;
        }

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

IDirect3DVertexBuffer9 * VertexBuffer9::getBuffer() const
{
    return mVertexBuffer;
}

bool VertexBuffer9::spaceRequired(const gl::VertexAttribute &attrib, std::size_t count, GLsizei instances,
                                  unsigned int *outSpaceRequired) const
{
    gl::VertexFormat vertexFormat(attrib, GL_FLOAT);
    const d3d9::VertexFormat &d3d9VertexInfo = d3d9::GetVertexFormatInfo(mRenderer->getCapsDeclTypes(), vertexFormat);

    if (attrib.enabled)
    {
        unsigned int elementCount = 0;
        if (instances == 0 || attrib.divisor == 0)
        {
            elementCount = count;
        }
        else
        {
            
            elementCount = rx::UnsignedCeilDivide(static_cast<unsigned int>(instances), attrib.divisor);
        }

        if (d3d9VertexInfo.outputElementSize <= std::numeric_limits<unsigned int>::max() / elementCount)
        {
            if (outSpaceRequired)
            {
                *outSpaceRequired = d3d9VertexInfo.outputElementSize * elementCount;
            }
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        const unsigned int elementSize = 4;
        if (outSpaceRequired)
        {
            *outSpaceRequired = elementSize * 4;
        }
        return true;
    }
}

}

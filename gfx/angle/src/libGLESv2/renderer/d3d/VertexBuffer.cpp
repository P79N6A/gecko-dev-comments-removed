#include "precompiled.h"









#include "libGLESv2/renderer/d3d/VertexBuffer.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/renderer/d3d/BufferD3D.h"
#include "common/mathutil.h"

namespace rx
{

unsigned int VertexBuffer::mNextSerial = 1;

VertexBuffer::VertexBuffer()
{
    updateSerial();
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::updateSerial()
{
    mSerial = mNextSerial++;
}

unsigned int VertexBuffer::getSerial() const
{
    return mSerial;
}

VertexBufferInterface::VertexBufferInterface(rx::Renderer *renderer, bool dynamic) : mRenderer(renderer)
{
    mDynamic = dynamic;
    mWritePosition = 0;
    mReservedSpace = 0;

    mVertexBuffer = renderer->createVertexBuffer();
}

VertexBufferInterface::~VertexBufferInterface()
{
    delete mVertexBuffer;
}

unsigned int VertexBufferInterface::getSerial() const
{
    return mVertexBuffer->getSerial();
}

unsigned int VertexBufferInterface::getBufferSize() const
{
    return mVertexBuffer->getBufferSize();
}

bool VertexBufferInterface::setBufferSize(unsigned int size)
{
    if (mVertexBuffer->getBufferSize() == 0)
    {
        return mVertexBuffer->initialize(size, mDynamic);
    }
    else
    {
        return mVertexBuffer->setBufferSize(size);
    }
}

unsigned int VertexBufferInterface::getWritePosition() const
{
    return mWritePosition;
}

void VertexBufferInterface::setWritePosition(unsigned int writePosition)
{
    mWritePosition = writePosition;
}

bool VertexBufferInterface::discard()
{
    return mVertexBuffer->discard();
}

bool VertexBufferInterface::storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                                  GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset)
{
    unsigned int spaceRequired;
    if (!mVertexBuffer->getSpaceRequired(attrib, count, instances, &spaceRequired))
    {
        return false;
    }

    if (mWritePosition + spaceRequired < mWritePosition)
    {
        return false;
    }

    if (!reserveSpace(mReservedSpace))
    {
        return false;
    }
    mReservedSpace = 0;

    if (!mVertexBuffer->storeVertexAttributes(attrib, currentValue, start, count, instances, mWritePosition))
    {
        return false;
    }

    if (outStreamOffset)
    {
        *outStreamOffset = mWritePosition;
    }

    mWritePosition += spaceRequired;

    
    mWritePosition = rx::roundUp(mWritePosition, 16u);

    return true;
}

bool VertexBufferInterface::reserveVertexSpace(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances)
{
    unsigned int requiredSpace;
    if (!mVertexBuffer->getSpaceRequired(attrib, count, instances, &requiredSpace))
    {
        return false;
    }

    
    if (mReservedSpace + requiredSpace < mReservedSpace)
    {
         return false;
    }

    mReservedSpace += requiredSpace;

    
    mReservedSpace = rx::roundUp(mReservedSpace, 16u);

    return true;
}

VertexBuffer* VertexBufferInterface::getVertexBuffer() const
{
    return mVertexBuffer;
}

bool VertexBufferInterface::directStoragePossible(const gl::VertexAttribute &attrib,
                                                  const gl::VertexAttribCurrentValueData &currentValue) const
{
    gl::Buffer *buffer = attrib.buffer.get();
    BufferD3D *storage = buffer ? BufferD3D::makeBufferD3D(buffer->getImplementation()) : NULL;

    if (!storage || !storage->supportsDirectBinding())
    {
        return false;
    }

    
    
    
    size_t alignment = 4;
    bool requiresConversion = false;

    if (attrib.type != GL_FLOAT)
    {
        gl::VertexFormat vertexFormat(attrib, currentValue.Type);

        unsigned int outputElementSize;
        getVertexBuffer()->getSpaceRequired(attrib, 1, 0, &outputElementSize);
        alignment = std::min<size_t>(outputElementSize, 4);

        requiresConversion = (mRenderer->getVertexConversionType(vertexFormat) & VERTEX_CONVERT_CPU) != 0;
    }

    bool isAligned = (static_cast<size_t>(ComputeVertexAttributeStride(attrib)) % alignment == 0) &&
                     (static_cast<size_t>(attrib.offset) % alignment == 0);

    return !requiresConversion && isAligned;
}

StreamingVertexBufferInterface::StreamingVertexBufferInterface(rx::Renderer *renderer, std::size_t initialSize) : VertexBufferInterface(renderer, true)
{
    setBufferSize(initialSize);
}

StreamingVertexBufferInterface::~StreamingVertexBufferInterface()
{
}

bool StreamingVertexBufferInterface::reserveSpace(unsigned int size)
{
    bool result = true;
    unsigned int curBufferSize = getBufferSize();
    if (size > curBufferSize)
    {
        result = setBufferSize(std::max(size, 3 * curBufferSize / 2));
        setWritePosition(0);
    }
    else if (getWritePosition() + size > curBufferSize)
    {
        if (!discard())
        {
            return false;
        }
        setWritePosition(0);
    }

    return result;
}

StaticVertexBufferInterface::StaticVertexBufferInterface(rx::Renderer *renderer) : VertexBufferInterface(renderer, false)
{
}

StaticVertexBufferInterface::~StaticVertexBufferInterface()
{
}

bool StaticVertexBufferInterface::lookupAttribute(const gl::VertexAttribute &attrib, unsigned int *outStreamOffset)
{
    for (unsigned int element = 0; element < mCache.size(); element++)
    {
        if (mCache[element].type == attrib.type &&
            mCache[element].size == attrib.size &&
            mCache[element].stride == ComputeVertexAttributeStride(attrib) &&
            mCache[element].normalized == attrib.normalized &&
            mCache[element].pureInteger == attrib.pureInteger)
        {
            size_t offset = (static_cast<size_t>(attrib.offset) % ComputeVertexAttributeStride(attrib));
            if (mCache[element].attributeOffset == offset)
            {
                if (outStreamOffset)
                {
                    *outStreamOffset = mCache[element].streamOffset;
                }
                return true;
            }
        }
    }

    return false;
}

bool StaticVertexBufferInterface::reserveSpace(unsigned int size)
{
    unsigned int curSize = getBufferSize();
    if (curSize == 0)
    {
        setBufferSize(size);
        return true;
    }
    else if (curSize >= size)
    {
        return true;
    }
    else
    {
        UNREACHABLE();   
        return false;
    }
}

bool StaticVertexBufferInterface::storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                                       GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset)
{
    unsigned int streamOffset;
    if (VertexBufferInterface::storeVertexAttributes(attrib, currentValue, start, count, instances, &streamOffset))
    {
        size_t attributeOffset = static_cast<size_t>(attrib.offset) % ComputeVertexAttributeStride(attrib);
        VertexElement element = { attrib.type, attrib.size, ComputeVertexAttributeStride(attrib), attrib.normalized, attrib.pureInteger, attributeOffset, streamOffset };
        mCache.push_back(element);

        if (outStreamOffset)
        {
            *outStreamOffset = streamOffset;
        }

        return true;
    }
    else
    {
        return false;
    }
}

}

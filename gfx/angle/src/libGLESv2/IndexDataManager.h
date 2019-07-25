








#ifndef LIBGLESV2_INDEXDATAMANAGER_H_
#define LIBGLESV2_INDEXDATAMANAGER_H_

#include <vector>
#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "libGLESv2/Context.h"

namespace gl
{

struct TranslatedIndexData
{
    UINT minIndex;
    UINT maxIndex;
    UINT startIndex;

    IDirect3DIndexBuffer9 *indexBuffer;
    unsigned int serial;
};

class IndexBuffer
{
  public:
    IndexBuffer(IDirect3DDevice9 *device, UINT size, D3DFORMAT format);
    virtual ~IndexBuffer();

    UINT size() const { return mBufferSize; }
    virtual void *map(UINT requiredSpace, UINT *offset) = 0;
    void unmap();
    virtual void reserveSpace(UINT requiredSpace, GLenum type) = 0;

    IDirect3DIndexBuffer9 *getBuffer() const;
    unsigned int getSerial() const;

  protected:
    IDirect3DDevice9 *const mDevice;

    IDirect3DIndexBuffer9 *mIndexBuffer;
    UINT mBufferSize;

    unsigned int mSerial;
    static unsigned int issueSerial();
    static unsigned int mCurrentSerial;

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBuffer);
};

class StreamingIndexBuffer : public IndexBuffer
{
  public:
    StreamingIndexBuffer(IDirect3DDevice9 *device, UINT initialSize, D3DFORMAT format);
    ~StreamingIndexBuffer();

    virtual void *map(UINT requiredSpace, UINT *offset);
    virtual void reserveSpace(UINT requiredSpace, GLenum type);

  private:
    UINT mWritePosition;
};

class StaticIndexBuffer : public IndexBuffer
{
  public:
    explicit StaticIndexBuffer(IDirect3DDevice9 *device);
    ~StaticIndexBuffer();

    virtual void *map(UINT requiredSpace, UINT *offset);
    virtual void reserveSpace(UINT requiredSpace, GLenum type);

    bool lookupType(GLenum type);
    UINT lookupRange(intptr_t offset, GLsizei count, UINT *minIndex, UINT *maxIndex);   
    void addRange(intptr_t offset, GLsizei count, UINT minIndex, UINT maxIndex, UINT streamOffset);

  private:
    GLenum mCacheType;
    
    struct IndexRange
    {
        intptr_t offset;
        GLsizei count;

        bool operator<(const IndexRange& rhs) const
        {
            if (offset != rhs.offset)
            {
                return offset < rhs.offset;
            }
            if (count != rhs.count)
            {
                return count < rhs.count;
            }
            return false;
        }
    };

    struct IndexResult
    {
        UINT minIndex;
        UINT maxIndex;
        UINT streamOffset;
    };

    std::map<IndexRange, IndexResult> mCache;
};

class IndexDataManager
{
  public:
    IndexDataManager(Context *context, IDirect3DDevice9 *evice);
    virtual ~IndexDataManager();

    GLenum prepareIndexData(GLenum type, GLsizei count, Buffer *arrayElementBuffer, const void *indices, TranslatedIndexData *translated);

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexDataManager);

    std::size_t typeSize(GLenum type) const;
    std::size_t indexSize(D3DFORMAT format) const;

    IDirect3DDevice9 *const mDevice;

    StreamingIndexBuffer *mStreamingBufferShort;
    StreamingIndexBuffer *mStreamingBufferInt;
};

}

#endif   

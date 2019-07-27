







#ifndef LIBGLESV2_RENDERER_BUFFER11_H_
#define LIBGLESV2_RENDERER_BUFFER11_H_

#include "libGLESv2/renderer/d3d/BufferD3D.h"
#include "libGLESv2/renderer/d3d/MemoryBuffer.h"
#include "libGLESv2/angletypes.h"

namespace rx
{
class Renderer11;

enum BufferUsage
{
    BUFFER_USAGE_STAGING,
    BUFFER_USAGE_VERTEX,
    BUFFER_USAGE_TRANSFORM_FEEDBACK,
    BUFFER_USAGE_INDEX,
    BUFFER_USAGE_PIXEL_UNPACK,
    BUFFER_USAGE_PIXEL_PACK,
    BUFFER_USAGE_UNIFORM,

    
    BUFFER_USAGE_VERTEX_DYNAMIC,
    BUFFER_USAGE_INDEX_DYNAMIC
};

struct PackPixelsParams
{
    PackPixelsParams();
    PackPixelsParams(const gl::Rectangle &area, GLenum format, GLenum type, GLuint outputPitch,
                     const gl::PixelPackState &pack, ptrdiff_t offset);

    gl::Rectangle area;
    GLenum format;
    GLenum type;
    GLuint outputPitch;
    gl::Buffer *packBuffer;
    gl::PixelPackState pack;
    ptrdiff_t offset;
};

typedef size_t DataRevision;

class Buffer11 : public BufferD3D
{
  public:
    Buffer11(rx::Renderer11 *renderer);
    virtual ~Buffer11();

    static Buffer11 *makeBuffer11(BufferImpl *buffer);

    ID3D11Buffer *getBuffer(BufferUsage usage);
    ID3D11ShaderResourceView *getSRV(DXGI_FORMAT srvFormat);
    bool isMapped() const { return mMappedStorage != NULL; }
    void packPixels(ID3D11Texture2D *srcTexure, UINT srcSubresource, const PackPixelsParams &params);

    
    virtual size_t getSize() const { return mSize; }
    virtual bool supportsDirectBinding() const { return true; }
    virtual Renderer* getRenderer();

    
    virtual void setData(const void* data, size_t size, GLenum usage);
    virtual void *getData();
    virtual void setSubData(const void* data, size_t size, size_t offset);
    virtual void copySubData(BufferImpl* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size);
    virtual GLvoid* map(size_t offset, size_t length, GLbitfield access);
    virtual void unmap();
    virtual void markTransformFeedbackUsage();

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer11);

    class BufferStorage11;
    class NativeBuffer11;
    class PackStorage11;

    void markBufferUsage();
    NativeBuffer11 *getStagingBuffer();
    PackStorage11 *getPackStorage();

    BufferStorage11 *getBufferStorage(BufferUsage usage);
    BufferStorage11 *getLatestBufferStorage() const;

    rx::Renderer11 *mRenderer;
    size_t mSize;

    BufferStorage11 *mMappedStorage;

    std::map<BufferUsage, BufferStorage11*> mBufferStorages;

    typedef std::pair<ID3D11Buffer *, ID3D11ShaderResourceView *> BufferSRVPair;
    std::map<DXGI_FORMAT, BufferSRVPair> mBufferResourceViews;

    MemoryBuffer mResolvedData;
    DataRevision mResolvedDataRevision;
    unsigned int mReadUsageCount;

    MemoryBuffer mDynamicData;
    bool mDynamicUsage;
    Range<size_t> mDynamicDirtyRange;
};

}

#endif 

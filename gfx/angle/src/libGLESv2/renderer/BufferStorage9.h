







#ifndef LIBGLESV2_RENDERER_BUFFERSTORAGE9_H_
#define LIBGLESV2_RENDERER_BUFFERSTORAGE9_H_

#include "libGLESv2/renderer/BufferStorage.h"

namespace rx
{

class BufferStorage9 : public BufferStorage
{
  public:
    BufferStorage9();
    virtual ~BufferStorage9();

    static BufferStorage9 *makeBufferStorage9(BufferStorage *bufferStorage);

    virtual void *getData();
    virtual void setData(const void* data, unsigned int size, unsigned int offset);
    virtual void clear();
    virtual unsigned int getSize() const;
    virtual bool supportsDirectBinding() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(BufferStorage9);

    void *mMemory;
    unsigned int mAllocatedSize;

    unsigned int mSize;
};

}

#endif 

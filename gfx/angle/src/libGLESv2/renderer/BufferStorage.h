







#ifndef LIBGLESV2_RENDERER_BUFFERSTORAGE_H_
#define LIBGLESV2_RENDERER_BUFFERSTORAGE_H_

#include "common/angleutils.h"

namespace rx
{

class BufferStorage
{
  public:
    BufferStorage();
    virtual ~BufferStorage();

    
    virtual void *getData() = 0;
    virtual void setData(const void* data, unsigned int size, unsigned int offset) = 0;
    virtual void clear() = 0;
    virtual unsigned int getSize() const = 0;
    virtual bool supportsDirectBinding() const = 0;
    virtual void markBufferUsage();
    unsigned int getSerial() const;

  protected:
    void updateSerial();

  private:
    DISALLOW_COPY_AND_ASSIGN(BufferStorage);

    unsigned int mSerial;
    static unsigned int mNextSerial;
};

}

#endif 

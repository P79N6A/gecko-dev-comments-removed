





#ifndef LIBGLESV2_RENDERER_D3D_MEMORYBUFFER_H_
#define LIBGLESV2_RENDERER_D3D_MEMORYBUFFER_H_

#include <cstddef>
#include <cstdint>

namespace rx
{

class MemoryBuffer
{
  public:
    MemoryBuffer();
    ~MemoryBuffer();

    bool resize(size_t size);
    size_t size() const;
    void clear();

    const uint8_t *data() const;
    uint8_t *data();

    bool empty() const { return (mSize == 0); }

  private:
    size_t mSize;
    uint8_t *mData;
};

}

#endif 






#ifndef WEBGLELEMENTARRAYCACHE_H
#define WEBGLELEMENTARRAYCACHE_H

#include "mozilla/MemoryReporting.h"
#include <stdint.h>
#include "nscore.h"
#include "GLDefs.h"

namespace mozilla {

template<typename T>
struct WebGLElementArrayCacheTree;












class WebGLElementArrayCache {

public:
  bool BufferData(const void* ptr, size_t byteSize);
  void BufferSubData(size_t pos, const void* ptr, size_t updateByteSize);

  bool Validate(GLenum type, uint32_t maxAllowed, size_t first, size_t count,
                uint32_t* out_upperBound = nullptr);

  template<typename T>
  T Element(size_t i) const { return Elements<T>()[i]; }

  WebGLElementArrayCache()
    : mUntypedData(nullptr)
    , mByteSize(0)
    , mUint8Tree(nullptr)
    , mUint16Tree(nullptr)
    , mUint32Tree(nullptr)
  {}

  ~WebGLElementArrayCache();

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:

  template<typename T>
  bool Validate(uint32_t maxAllowed, size_t first, size_t count,
                uint32_t* out_upperBound);

  size_t ByteSize() const {
    return mByteSize;
  }

  template<typename T>
  const T* Elements() const { return static_cast<const T*>(mUntypedData); }
  template<typename T>
  T* Elements() { return static_cast<T*>(mUntypedData); }

  void UpdateTrees(size_t firstByte, size_t lastByte);

  template<typename T>
  friend struct WebGLElementArrayCacheTree;
  template<typename T>
  friend struct TreeForType;

  void* mUntypedData;
  size_t mByteSize;
  WebGLElementArrayCacheTree<uint8_t>* mUint8Tree;
  WebGLElementArrayCacheTree<uint16_t>* mUint16Tree;
  WebGLElementArrayCacheTree<uint32_t>* mUint32Tree;
};


} 

#endif 

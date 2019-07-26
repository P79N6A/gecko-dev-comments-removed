




#ifndef WEBGLELEMENTARRAYCACHE_H
#define WEBGLELEMENTARRAYCACHE_H

#include "mozilla/StandardInteger.h"
#include "nscore.h"
#include "GLDefs.h"

namespace mozilla {

template<typename T>
struct WebGLElementArrayCacheTree;












class WebGLElementArrayCache {

public:
  bool BufferData(const void* ptr, size_t byteSize);
  void BufferSubData(size_t pos, const void* ptr, size_t updateByteSize);

  bool Validate(GLenum type, uint32_t maxAllowed, size_t first, size_t count);

  template<typename T>
  T Element(size_t i) const { return Elements<T>()[i]; }

  WebGLElementArrayCache()
    : mUntypedData(nullptr)
    , mByteSize(0)
    , mUint8Tree(nullptr)
    , mUint16Tree(nullptr)
  {}

  ~WebGLElementArrayCache();

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:

  template<typename T>
  bool Validate(uint32_t maxAllowed, size_t first, size_t count);

  size_t ByteSize() const {
    return mByteSize;
  }

  template<typename T>
  const T* Elements() const { return static_cast<const T*>(mUntypedData); }
  template<typename T>
  T* Elements() { return static_cast<T*>(mUntypedData); }

  void InvalidateTrees(size_t firstByte, size_t lastByte);

  template<typename T>
  friend struct WebGLElementArrayCacheTree;
  template<typename T>
  friend struct TreeForType;

  void* mUntypedData;
  size_t mByteSize;
  WebGLElementArrayCacheTree<uint8_t>* mUint8Tree;
  WebGLElementArrayCacheTree<uint16_t>* mUint16Tree;
};


} 

#endif 

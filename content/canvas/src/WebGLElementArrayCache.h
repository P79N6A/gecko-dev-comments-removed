




#ifndef WEBGLELEMENTARRAYCACHE_H
#define WEBGLELEMENTARRAYCACHE_H

#include "mozilla/MemoryReporting.h"
#include "mozilla/Scoped.h"
#include "nsTArray.h"
#include <stdint.h>
#include "nscore.h"
#include "GLDefs.h"

namespace mozilla {

template<typename T>
struct WebGLElementArrayCacheTree;












class WebGLElementArrayCache {

public:
  bool BufferData(const void* ptr, size_t byteLength);
  bool BufferSubData(size_t pos, const void* ptr, size_t updateByteSize);

  bool Validate(GLenum type, uint32_t maxAllowed, size_t first, size_t count,
                uint32_t* out_upperBound = nullptr);

  template<typename T>
  T Element(size_t i) const { return Elements<T>()[i]; }

  WebGLElementArrayCache();

  ~WebGLElementArrayCache();

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:

  template<typename T>
  bool Validate(uint32_t maxAllowed, size_t first, size_t count,
                uint32_t* out_upperBound);

  template<typename T>
  const T* Elements() const { return reinterpret_cast<const T*>(mBytes.Elements()); }
  template<typename T>
  T* Elements() { return reinterpret_cast<T*>(mBytes.Elements()); }

  bool UpdateTrees(size_t firstByte, size_t lastByte);

  template<typename T>
  friend struct WebGLElementArrayCacheTree;
  template<typename T>
  friend struct TreeForType;

  FallibleTArray<uint8_t> mBytes;
  ScopedDeletePtr<WebGLElementArrayCacheTree<uint8_t>> mUint8Tree;
  ScopedDeletePtr<WebGLElementArrayCacheTree<uint16_t>> mUint16Tree;
  ScopedDeletePtr<WebGLElementArrayCacheTree<uint32_t>> mUint32Tree;
};


} 

#endif 

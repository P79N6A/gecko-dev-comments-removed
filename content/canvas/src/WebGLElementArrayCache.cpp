




#include "WebGLElementArrayCache.h"

#include "nsTArray.h"
#include "mozilla/Assertions.h"
#include "mozilla/MemoryReporting.h"

#include <cstdlib>
#include <cstring>
#include <limits>
#include <algorithm>

namespace mozilla {

static void
SetUpperBound(uint32_t* out_upperBound, uint32_t newBound)
{
  if (!out_upperBound)
      return;

  *out_upperBound = newBound;
}

static void
UpdateUpperBound(uint32_t* out_upperBound, uint32_t newBound)
{
  if (!out_upperBound)
      return;

  *out_upperBound = std::max(*out_upperBound, newBound);
}



































































































template<typename T>
struct WebGLElementArrayCacheTree
{
  
  
  
  static const size_t sSkippedBottomTreeLevels = 3;
  static const size_t sElementsPerLeaf = 1 << sSkippedBottomTreeLevels;
  static const size_t sElementsPerLeafMask = sElementsPerLeaf - 1; 

private:
  WebGLElementArrayCache& mParent;
  FallibleTArray<T> mTreeData;
  size_t mNumLeaves;
  size_t mParentByteSize;

public:
  WebGLElementArrayCacheTree(WebGLElementArrayCache& p)
    : mParent(p)
    , mNumLeaves(0)
    , mParentByteSize(0)
  {
    if (mParent.ByteSize()) {
      Update(0, mParent.ByteSize() - 1);
    }
  }

  T GlobalMaximum() const {
    return mTreeData[1];
  }

  
  
  static size_t ParentNode(size_t treeIndex) {
    MOZ_ASSERT(treeIndex > 1);
    return treeIndex >> 1;
  }

  static bool IsRightNode(size_t treeIndex) {
    MOZ_ASSERT(treeIndex > 1);
    return treeIndex & 1;
  }

  static bool IsLeftNode(size_t treeIndex) {
    MOZ_ASSERT(treeIndex > 1);
    return !IsRightNode(treeIndex);
  }

  static size_t SiblingNode(size_t treeIndex) {
    MOZ_ASSERT(treeIndex > 1);
    return treeIndex ^ 1;
  }

  static size_t LeftChildNode(size_t treeIndex) {
    MOZ_ASSERT(treeIndex);
    return treeIndex << 1;
  }

  static size_t RightChildNode(size_t treeIndex) {
    MOZ_ASSERT(treeIndex);
    return SiblingNode(LeftChildNode(treeIndex));
  }

  static size_t LeftNeighborNode(size_t treeIndex, size_t distance = 1) {
    MOZ_ASSERT(treeIndex > 1);
    return treeIndex - distance;
  }

  static size_t RightNeighborNode(size_t treeIndex, size_t distance = 1) {
    MOZ_ASSERT(treeIndex > 1);
    return treeIndex + distance;
  }

  size_t LeafForElement(size_t element) {
    size_t leaf = element / sElementsPerLeaf;
    MOZ_ASSERT(leaf < mNumLeaves);
    return leaf;
  }

  size_t LeafForByte(size_t byte) {
    return LeafForElement(byte / sizeof(T));
  }

  
  size_t TreeIndexForLeaf(size_t leaf) {
    
    
    return leaf + mNumLeaves;
  }

  static size_t LastElementUnderSameLeaf(size_t element) {
    return element | sElementsPerLeafMask;
  }

  static size_t FirstElementUnderSameLeaf(size_t element) {
    return element & ~sElementsPerLeafMask;
  }

  static size_t NextMultipleOfElementsPerLeaf(size_t numElements) {
    MOZ_ASSERT(numElements >= 1);
    return ((numElements - 1) | sElementsPerLeafMask) + 1;
  }

  bool Validate(T maxAllowed, size_t firstLeaf, size_t lastLeaf,
                uint32_t* out_upperBound)
  {
    size_t firstTreeIndex = TreeIndexForLeaf(firstLeaf);
    size_t lastTreeIndex  = TreeIndexForLeaf(lastLeaf);

    while (true) {
      
      
      MOZ_ASSERT(firstTreeIndex <= lastTreeIndex);

      
      if (lastTreeIndex == firstTreeIndex) {
        const T& curData = mTreeData[firstTreeIndex];
        UpdateUpperBound(out_upperBound, curData);
        return curData <= maxAllowed;
      }

      
      
      if (IsRightNode(firstTreeIndex)) {
        const T& curData = mTreeData[firstTreeIndex];
        UpdateUpperBound(out_upperBound, curData);
        if (curData > maxAllowed)
          return false;
        firstTreeIndex = RightNeighborNode(firstTreeIndex);
      }

      
      
      if (IsLeftNode(lastTreeIndex)) {
        const T& curData = mTreeData[lastTreeIndex];
        UpdateUpperBound(out_upperBound, curData);
        if (curData > maxAllowed)
          return false;
        lastTreeIndex = LeftNeighborNode(lastTreeIndex);
      }

      
      
      
      
      
      if (lastTreeIndex == LeftNeighborNode(firstTreeIndex)) {
        return true;
      }

      
      firstTreeIndex = ParentNode(firstTreeIndex);
      lastTreeIndex = ParentNode(lastTreeIndex);
    }
  }

  template<typename U>
  static U NextPowerOfTwo(U x) {
    U result = 1;
    while (result < x)
      result <<= 1;
    MOZ_ASSERT(result >= x);
    MOZ_ASSERT((result & (result - 1)) == 0);
    return result;
  }

  bool Update(size_t firstByte, size_t lastByte);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + mTreeData.SizeOfExcludingThis(aMallocSizeOf);
  }
};



template<typename T>
struct TreeForType {};

template<>
struct TreeForType<uint8_t>
{
  static WebGLElementArrayCacheTree<uint8_t>*& Run(WebGLElementArrayCache *b) { return b->mUint8Tree; }
};

template<>
struct TreeForType<uint16_t>
{
  static WebGLElementArrayCacheTree<uint16_t>*& Run(WebGLElementArrayCache *b) { return b->mUint16Tree; }
};

template<>
struct TreeForType<uint32_t>
{
  static WebGLElementArrayCacheTree<uint32_t>*& Run(WebGLElementArrayCache *b) { return b->mUint32Tree; }
};



template<typename T>
bool WebGLElementArrayCacheTree<T>::Update(size_t firstByte, size_t lastByte)
{
  MOZ_ASSERT(firstByte <= lastByte);
  MOZ_ASSERT(lastByte < mParent.ByteSize());

  
  if (mParentByteSize != mParent.ByteSize())
  {
    size_t numberOfElements = mParent.ByteSize() / sizeof(T);
    if (numberOfElements == 0) {
      mParentByteSize = mParent.ByteSize();
      return true;
    }

    size_t requiredNumLeaves = (numberOfElements + sElementsPerLeaf - 1) / sElementsPerLeaf;
    size_t oldNumLeaves = mNumLeaves;
    mNumLeaves = NextPowerOfTwo(requiredNumLeaves);
    if (mNumLeaves != oldNumLeaves) {
      
      if (!mTreeData.SetLength(2 * mNumLeaves)) {
        return false;
      }
      
      
      memset(mTreeData.Elements(), 0, mTreeData.Length() * sizeof(T));
      firstByte = 0;
      lastByte = mParent.ByteSize() - 1;
    }

    mParentByteSize = mParent.ByteSize();
  }

  lastByte = std::min(lastByte, mNumLeaves * sElementsPerLeaf * sizeof(T) - 1);
  if (firstByte > lastByte) {
    return true;
  }

  size_t firstLeaf = LeafForByte(firstByte);
  size_t lastLeaf = LeafForByte(lastByte);

  MOZ_ASSERT(firstLeaf <= lastLeaf && lastLeaf < mNumLeaves);

  size_t firstTreeIndex = TreeIndexForLeaf(firstLeaf);
  size_t lastTreeIndex = TreeIndexForLeaf(lastLeaf);

  
  
  
  
  {
    
    size_t treeIndex = firstTreeIndex;
    
    size_t srcIndex = firstLeaf * sElementsPerLeaf;
    size_t numberOfElements = mParentByteSize / sizeof(T);
    while (treeIndex <= lastTreeIndex) {
      T m = 0;
      size_t a = srcIndex;
      size_t srcIndexNextLeaf = std::min(a + sElementsPerLeaf, numberOfElements);
      for (; srcIndex < srcIndexNextLeaf; srcIndex++) {
        m = std::max(m, mParent.Element<T>(srcIndex));
      }
      mTreeData[treeIndex] = m;
      treeIndex++;
    }
  }

  
  
  while (firstTreeIndex > 1) {
    
    firstTreeIndex = ParentNode(firstTreeIndex);
    lastTreeIndex = ParentNode(lastTreeIndex);

    
    if (firstTreeIndex == lastTreeIndex) {
      mTreeData[firstTreeIndex] = std::max(mTreeData[LeftChildNode(firstTreeIndex)], mTreeData[RightChildNode(firstTreeIndex)]);
      continue;
    }

    
    size_t child = LeftChildNode(firstTreeIndex);
    size_t parent = firstTreeIndex;

    
    
    const int unrollSize = 8;
    while (RightNeighborNode(parent, unrollSize - 1) <= lastTreeIndex)
    {
      for (int unroll = 0; unroll < unrollSize; unroll++)
      {
        T a = mTreeData[child];
        child = RightNeighborNode(child);
        T b = mTreeData[child];
        child = RightNeighborNode(child);
        mTreeData[parent] = std::max(a, b);
        parent = RightNeighborNode(parent);
      }
    }
    
    while (parent <= lastTreeIndex)
    {
      T a = mTreeData[child];
      child = RightNeighborNode(child);
      T b = mTreeData[child];
      child = RightNeighborNode(child);
      mTreeData[parent] = std::max(a, b);
      parent = RightNeighborNode(parent);
    }
  }

  return true;
}

WebGLElementArrayCache::~WebGLElementArrayCache() {
  delete mUint8Tree;
  delete mUint16Tree;
  delete mUint32Tree;
  free(mUntypedData);
}

bool WebGLElementArrayCache::BufferData(const void* ptr, size_t byteSize) {
  if (byteSize == 0) {
    mByteSize = 0;
    free(mUntypedData);
    mUntypedData = nullptr;
    return true;
  }
  if (byteSize != mByteSize) {
    void* newUntypedData = realloc(mUntypedData, byteSize);
    if (!newUntypedData)
      return false;
    mByteSize = byteSize;
    mUntypedData = newUntypedData;
  }

  BufferSubData(0, ptr, byteSize);
  return true;
}

bool WebGLElementArrayCache::BufferSubData(size_t pos, const void* ptr, size_t updateByteSize) {
  if (!updateByteSize) return true;
  if (ptr)
      memcpy(static_cast<uint8_t*>(mUntypedData) + pos, ptr, updateByteSize);
  else
      memset(static_cast<uint8_t*>(mUntypedData) + pos, 0, updateByteSize);
  return UpdateTrees(pos, pos + updateByteSize - 1);
}

bool WebGLElementArrayCache::UpdateTrees(size_t firstByte, size_t lastByte)
{
  bool result = true;
  if (mUint8Tree)
    result &= mUint8Tree->Update(firstByte, lastByte);
  if (mUint16Tree)
    result &= mUint16Tree->Update(firstByte, lastByte);
  if (mUint32Tree)
    result &= mUint32Tree->Update(firstByte, lastByte);
  return result;
}

template<typename T>
bool
WebGLElementArrayCache::Validate(uint32_t maxAllowed, size_t firstElement,
                                 size_t countElements, uint32_t* out_upperBound)
{
  SetUpperBound(out_upperBound, 0);

  
  uint32_t maxTSize = std::numeric_limits<T>::max();
  if (maxAllowed >= maxTSize) {
    SetUpperBound(out_upperBound, maxTSize);
    return true;
  }

  T maxAllowedT(maxAllowed);

  
  
  MOZ_ASSERT(uint32_t(maxAllowedT) == maxAllowed);

  if (!mByteSize || !countElements)
    return true;

  WebGLElementArrayCacheTree<T>*& tree = TreeForType<T>::Run(this);
  if (!tree) {
    tree = new WebGLElementArrayCacheTree<T>(*this);
  }

  size_t lastElement = firstElement + countElements - 1;

  
  
  T globalMax = tree->GlobalMaximum();
  if (globalMax <= maxAllowedT)
  {
    SetUpperBound(out_upperBound, globalMax);
    return true;
  }

  const T* elements = Elements<T>();

  
  
  size_t firstElementAdjustmentEnd = std::min(lastElement,
                                            tree->LastElementUnderSameLeaf(firstElement));
  while (firstElement <= firstElementAdjustmentEnd) {
    const T& curData = elements[firstElement];
    UpdateUpperBound(out_upperBound, curData);
    if (curData > maxAllowedT)
      return false;
    firstElement++;
  }
  size_t lastElementAdjustmentEnd = std::max(firstElement,
                                           tree->FirstElementUnderSameLeaf(lastElement));
  while (lastElement >= lastElementAdjustmentEnd) {
    const T& curData = elements[lastElement];
    UpdateUpperBound(out_upperBound, curData);
    if (curData > maxAllowedT)
      return false;
    lastElement--;
  }

  
  if (firstElement > lastElement)
    return true;

  
  return tree->Validate(maxAllowedT,
                        tree->LeafForElement(firstElement),
                        tree->LeafForElement(lastElement),
                        out_upperBound);
}

bool
WebGLElementArrayCache::Validate(GLenum type, uint32_t maxAllowed,
                                 size_t firstElement, size_t countElements,
                                 uint32_t* out_upperBound)
{
  if (type == LOCAL_GL_UNSIGNED_BYTE)
    return Validate<uint8_t>(maxAllowed, firstElement, countElements, out_upperBound);
  if (type == LOCAL_GL_UNSIGNED_SHORT)
    return Validate<uint16_t>(maxAllowed, firstElement, countElements, out_upperBound);
  if (type == LOCAL_GL_UNSIGNED_INT)
    return Validate<uint32_t>(maxAllowed, firstElement, countElements, out_upperBound);

  MOZ_ASSERT(false, "Invalid type.");
  return false;
}

size_t
WebGLElementArrayCache::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t uint8TreeSize  = mUint8Tree  ? mUint8Tree->SizeOfIncludingThis(aMallocSizeOf) : 0;
  size_t uint16TreeSize = mUint16Tree ? mUint16Tree->SizeOfIncludingThis(aMallocSizeOf) : 0;
  size_t uint32TreeSize = mUint32Tree ? mUint32Tree->SizeOfIncludingThis(aMallocSizeOf) : 0;
  return aMallocSizeOf(this) +
          mByteSize +
          uint8TreeSize +
          uint16TreeSize +
          uint32TreeSize;
}

} 

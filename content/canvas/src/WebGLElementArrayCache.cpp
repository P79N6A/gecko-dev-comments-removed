




#include "WebGLElementArrayCache.h"

#include "nsTArray.h"
#include "mozilla/Assertions.h"

#include <cstdlib>
#include <cstring>
#include <limits>

namespace mozilla {











































































































template<typename T>
struct WebGLElementArrayCacheTree
{
  
  
  
  static const size_t sSkippedBottomTreeLevels = 3;
  static const size_t sElementsPerLeaf = 1 << sSkippedBottomTreeLevels;
  static const size_t sElementsPerLeafMask = sElementsPerLeaf - 1; 

private:
  WebGLElementArrayCache& mParent;
  nsTArray<T> mTreeData;
  size_t mNumLeaves;
  bool mInvalidated;
  size_t mFirstInvalidatedLeaf;
  size_t mLastInvalidatedLeaf;

public:
  WebGLElementArrayCacheTree(WebGLElementArrayCache& p)
    : mParent(p)
    , mNumLeaves(0)
    , mInvalidated(false)
    , mFirstInvalidatedLeaf(0)
    , mLastInvalidatedLeaf(0)
  {
    ResizeToParentSize();
  }

  T GlobalMaximum() const {
    MOZ_ASSERT(!mInvalidated);
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
    return ((numElements - 1) | sElementsPerLeafMask) + 1;
  }

  bool Validate(T maxAllowed, size_t firstLeaf, size_t lastLeaf) {
    MOZ_ASSERT(!mInvalidated);

    size_t firstTreeIndex = TreeIndexForLeaf(firstLeaf);
    size_t lastTreeIndex  = TreeIndexForLeaf(lastLeaf);

    while (true) {
      
      
      MOZ_ASSERT(firstTreeIndex <= lastTreeIndex);

      
      if (lastTreeIndex == firstTreeIndex) {
        return mTreeData[firstTreeIndex] <= maxAllowed;
      }

      
      
      if (IsRightNode(firstTreeIndex)) {
        if (mTreeData[firstTreeIndex] > maxAllowed)
          return false;
        firstTreeIndex = RightNeighborNode(firstTreeIndex);
      }

      
      
      if (IsLeftNode(lastTreeIndex)) {
        if (mTreeData[lastTreeIndex] > maxAllowed)
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

  bool ResizeToParentSize()
  {
    size_t numberOfElements = mParent.ByteSize() / sizeof(T);
    size_t requiredNumLeaves = (numberOfElements + sElementsPerLeaf - 1) / sElementsPerLeaf;

    size_t oldNumLeaves = mNumLeaves;
    mNumLeaves = NextPowerOfTwo(requiredNumLeaves);
    Invalidate(0, mParent.ByteSize() - 1);

    
    if (!mTreeData.SetLength(2 * mNumLeaves)) {
      return false;
    }
    if (mNumLeaves != oldNumLeaves) {
      memset(mTreeData.Elements(), 0, mTreeData.Length() * sizeof(mTreeData[0]));
    }
    return true;
  }

  void Invalidate(size_t firstByte, size_t lastByte);

  void Update();

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
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



template<typename T>
void WebGLElementArrayCacheTree<T>::Invalidate(size_t firstByte, size_t lastByte)
{
  lastByte = NS_MIN(lastByte, mNumLeaves * sElementsPerLeaf * sizeof(T) - 1);
  if (firstByte > lastByte) {
    return;
  }

  size_t firstLeaf = LeafForByte(firstByte);
  size_t lastLeaf = LeafForByte(lastByte);

  if (mInvalidated) {
    mFirstInvalidatedLeaf = NS_MIN(firstLeaf, mFirstInvalidatedLeaf);
    mLastInvalidatedLeaf = NS_MAX(lastLeaf, mLastInvalidatedLeaf);
  } else {
    mInvalidated = true;
    mFirstInvalidatedLeaf = firstLeaf;
    mLastInvalidatedLeaf = lastLeaf;
  }
}





template<typename T>
void WebGLElementArrayCacheTree<T>::Update()
{
  if (!mInvalidated) {
    return;
  }

  MOZ_ASSERT(mLastInvalidatedLeaf < mNumLeaves);

  size_t firstTreeIndex = TreeIndexForLeaf(mFirstInvalidatedLeaf);
  size_t lastTreeIndex = TreeIndexForLeaf(mLastInvalidatedLeaf);

  
  
  
  
  {
    
    size_t treeIndex = firstTreeIndex;
    
    size_t srcIndex = mFirstInvalidatedLeaf * sElementsPerLeaf;
    size_t numberOfElements = mParent.ByteSize() / sizeof(T);
    while (treeIndex <= lastTreeIndex) {
      T m = 0;
      size_t a = srcIndex;
      size_t srcIndexNextLeaf = NS_MIN(a + sElementsPerLeaf, numberOfElements);
      for (; srcIndex < srcIndexNextLeaf; srcIndex++) {
        m = NS_MAX(m, mParent.Element<T>(srcIndex));
      }
      mTreeData[treeIndex] = m;
      treeIndex++;
    }
  }

  
  
  while (firstTreeIndex > 1) {

    
    firstTreeIndex = ParentNode(firstTreeIndex);
    lastTreeIndex = ParentNode(lastTreeIndex);

    
    if (firstTreeIndex == lastTreeIndex) {
      mTreeData[firstTreeIndex] = NS_MAX(mTreeData[LeftChildNode(firstTreeIndex)], mTreeData[RightChildNode(firstTreeIndex)]);
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
        mTreeData[parent] = NS_MAX(a, b);
        parent = RightNeighborNode(parent);
      }
    }
    
    while (parent <= lastTreeIndex)
    {
      T a = mTreeData[child];
      child = RightNeighborNode(child);
      T b = mTreeData[child];
      child = RightNeighborNode(child);
      mTreeData[parent] = NS_MAX(a, b);
      parent = RightNeighborNode(parent);
    }
  }

  mInvalidated = false;
}

WebGLElementArrayCache::~WebGLElementArrayCache() {
  delete mUint8Tree;
  delete mUint16Tree;
  free(mUntypedData);
}

bool WebGLElementArrayCache::BufferData(const void* ptr, size_t byteSize) {
  mByteSize = byteSize;
  if (mUint8Tree)
    if (!mUint8Tree->ResizeToParentSize())
      return false;
  if (mUint16Tree)
    if (!mUint16Tree->ResizeToParentSize())
      return false;
  mUntypedData = realloc(mUntypedData, byteSize);
  if (!mUntypedData)
    return false;
  BufferSubData(0, ptr, byteSize);
  return true;
}

void WebGLElementArrayCache::BufferSubData(size_t pos, const void* ptr, size_t updateByteSize) {
  if (!updateByteSize) return;
  if (ptr)
      memcpy(static_cast<uint8_t*>(mUntypedData) + pos, ptr, updateByteSize);
  else
      memset(static_cast<uint8_t*>(mUntypedData) + pos, 0, updateByteSize);
  InvalidateTrees(pos, pos + updateByteSize - 1);
}

void WebGLElementArrayCache::InvalidateTrees(size_t firstByte, size_t lastByte)
{
  if (mUint8Tree)
    mUint8Tree->Invalidate(firstByte, lastByte);
  if (mUint16Tree)
    mUint16Tree->Invalidate(firstByte, lastByte);
}

template<typename T>
bool WebGLElementArrayCache::Validate(uint32_t maxAllowed, size_t firstElement, size_t countElements) {
  
  if (maxAllowed >= std::numeric_limits<T>::max())
    return true;

  T maxAllowedT(maxAllowed);

  
  
  MOZ_ASSERT(uint32_t(maxAllowedT) == maxAllowed);

  if (!mByteSize || !countElements)
    return true;

  WebGLElementArrayCacheTree<T>*& tree = TreeForType<T>::Run(this);
  if (!tree) {
    tree = new WebGLElementArrayCacheTree<T>(*this);
  }

  size_t lastElement = firstElement + countElements - 1;

  tree->Update();

  
  
  if (tree->GlobalMaximum() <= maxAllowedT)
  {
    return true;
  }

  const T* elements = Elements<T>();

  
  
  size_t firstElementAdjustmentEnd = NS_MIN(lastElement,
                                            tree->LastElementUnderSameLeaf(firstElement));
  while (firstElement <= firstElementAdjustmentEnd) {
    if (elements[firstElement] > maxAllowedT)
      return false;
    firstElement++;
  }
  size_t lastElementAdjustmentEnd = NS_MAX(firstElement,
                                           tree->FirstElementUnderSameLeaf(lastElement));
  while (lastElement >= lastElementAdjustmentEnd) {
    if (elements[lastElement] > maxAllowedT)
      return false;
    lastElement--;
  }

  
  if (firstElement > lastElement)
    return true;

  
  return tree->Validate(maxAllowedT,
                        tree->LeafForElement(firstElement),
                        tree->LeafForElement(lastElement));
}

bool WebGLElementArrayCache::Validate(GLenum type, uint32_t maxAllowed, size_t firstElement, size_t countElements) {
  if (type == LOCAL_GL_UNSIGNED_BYTE)
    return Validate<uint8_t>(maxAllowed, firstElement, countElements);
  if (type == LOCAL_GL_UNSIGNED_SHORT)
    return Validate<uint16_t>(maxAllowed, firstElement, countElements);
  return false;
}

size_t WebGLElementArrayCache::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
  size_t uint8TreeSize  = mUint8Tree  ? mUint8Tree->SizeOfIncludingThis(aMallocSizeOf) : 0;
  size_t uint16TreeSize = mUint16Tree ? mUint16Tree->SizeOfIncludingThis(aMallocSizeOf) : 0;
  return aMallocSizeOf(this) +
          mByteSize +
          uint8TreeSize +
          uint16TreeSize;
}

} 

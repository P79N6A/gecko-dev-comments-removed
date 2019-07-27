




#include "WebGLElementArrayCache.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include "mozilla/Assertions.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {

static void
UpdateUpperBound(uint32_t* const out_upperBound, uint32_t newBound)
{
    MOZ_ASSERT(out_upperBound);
    
    
    uint32_t upperBound = *out_upperBound;
    *out_upperBound = std::max(upperBound, newBound);
}














































































































template<typename T>
struct WebGLElementArrayCacheTree
{
    




    static const size_t kSkippedBottomTreeLevels = 3;
    static const size_t kElementsPerLeaf = 1 << kSkippedBottomTreeLevels;
    
    static const size_t kElementsPerLeafMask = kElementsPerLeaf - 1;

private:
    
    WebGLElementArrayCache& mParent;

    
    
    FallibleTArray<T> mTreeData;

public:
    
    
    
    explicit WebGLElementArrayCacheTree(WebGLElementArrayCache& value)
        : mParent(value)
    {
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

    size_t NumLeaves() const {
        
        return mTreeData.Length() >> 1;
    }

    size_t LeafForElement(size_t element) const {
        size_t leaf = element / kElementsPerLeaf;
        MOZ_ASSERT(leaf < NumLeaves());
        return leaf;
    }

    size_t LeafForByte(size_t byte) const {
        return LeafForElement(byte / sizeof(T));
    }

    
    size_t TreeIndexForLeaf(size_t leaf) const {
        
        
        return leaf + NumLeaves();
    }

    static size_t LastElementUnderSameLeaf(size_t element) {
        return element | kElementsPerLeafMask;
    }

    static size_t FirstElementUnderSameLeaf(size_t element) {
        return element & ~kElementsPerLeafMask;
    }

    static size_t NextMultipleOfElementsPerLeaf(size_t numElements) {
        MOZ_ASSERT(numElements >= 1);
        return ((numElements - 1) | kElementsPerLeafMask) + 1;
    }

    bool Validate(T maxAllowed, size_t firstLeaf, size_t lastLeaf,
                  uint32_t* const out_upperBound)
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

            







            if (lastTreeIndex == LeftNeighborNode(firstTreeIndex))
                return true;

            
            firstTreeIndex = ParentNode(firstTreeIndex);
            lastTreeIndex = ParentNode(lastTreeIndex);
        }
    }

    
    
    bool Update(size_t firstByte, size_t lastByte);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
    {
        return mallocSizeOf(this) + mTreeData.SizeOfExcludingThis(mallocSizeOf);
    }
};



template<typename T>
struct TreeForType {};

template<>
struct TreeForType<uint8_t>
{
    static ScopedDeletePtr<WebGLElementArrayCacheTree<uint8_t>>&
    Value(WebGLElementArrayCache* b) {
        return b->mUint8Tree;
    }
};

template<>
struct TreeForType<uint16_t>
{
    static ScopedDeletePtr<WebGLElementArrayCacheTree<uint16_t>>&
    Value(WebGLElementArrayCache* b) {
        return b->mUint16Tree;
    }
};

template<>
struct TreeForType<uint32_t>
{
    static ScopedDeletePtr<WebGLElementArrayCacheTree<uint32_t>>&
    Value(WebGLElementArrayCache* b) {
        return b->mUint32Tree;
    }
};



template<typename T>
bool
WebGLElementArrayCacheTree<T>::Update(size_t firstByte, size_t lastByte)
{
    MOZ_ASSERT(firstByte <= lastByte);
    MOZ_ASSERT(lastByte < mParent.mBytes.Length());

    size_t numberOfElements = mParent.mBytes.Length() / sizeof(T);
    size_t requiredNumLeaves = 0;
    if (numberOfElements > 0) {
        








        size_t numLeavesNonPOT = (numberOfElements + kElementsPerLeaf - 1) / kElementsPerLeaf;
        
        requiredNumLeaves = RoundUpPow2(numLeavesNonPOT);
    }

    
    if (requiredNumLeaves != NumLeaves()) {
        
        if (!mTreeData.SetLength(2 * requiredNumLeaves)) {
            mTreeData.SetLength(0);
            return false;
        }
        MOZ_ASSERT(NumLeaves() == requiredNumLeaves);

        if (NumLeaves()) {
            
            
            memset(mTreeData.Elements(), 0, mTreeData.Length() * sizeof(T));
            firstByte = 0;
            lastByte = mParent.mBytes.Length() - 1;
        }
    }

    if (NumLeaves() == 0)
        return true;

    lastByte = std::min(lastByte, NumLeaves() * kElementsPerLeaf * sizeof(T) - 1);
    if (firstByte > lastByte)
        return true;

    size_t firstLeaf = LeafForByte(firstByte);
    size_t lastLeaf = LeafForByte(lastByte);

    MOZ_ASSERT(firstLeaf <= lastLeaf && lastLeaf < NumLeaves());

    size_t firstTreeIndex = TreeIndexForLeaf(firstLeaf);
    size_t lastTreeIndex = TreeIndexForLeaf(lastLeaf);

    
    
    

    
    
    {
        
        
        size_t treeIndex = firstTreeIndex;
        
        size_t srcIndex = firstLeaf * kElementsPerLeaf;
        while (treeIndex <= lastTreeIndex) {
            T m = 0;
            size_t a = srcIndex;
            size_t srcIndexNextLeaf = std::min(a + kElementsPerLeaf, numberOfElements);
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
        while (parent <= lastTreeIndex) {
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

WebGLElementArrayCache::WebGLElementArrayCache()
{
}

WebGLElementArrayCache::~WebGLElementArrayCache()
{
}

bool
WebGLElementArrayCache::BufferData(const void* ptr, size_t byteLength)
{
    if (mBytes.Length() != byteLength) {
        if (!mBytes.SetLength(byteLength)) {
            mBytes.SetLength(0);
            return false;
        }
    }
    MOZ_ASSERT(mBytes.Length() == byteLength);
    return BufferSubData(0, ptr, byteLength);
}

bool
WebGLElementArrayCache::BufferSubData(size_t pos, const void* ptr,
                                      size_t updateByteLength)
{
    MOZ_ASSERT(pos + updateByteLength <= mBytes.Length());
    if (!updateByteLength)
        return true;

    if (ptr)
        memcpy(mBytes.Elements() + pos, ptr, updateByteLength);
    else
        memset(mBytes.Elements() + pos, 0, updateByteLength);
    return UpdateTrees(pos, pos + updateByteLength - 1);
}

bool
WebGLElementArrayCache::UpdateTrees(size_t firstByte, size_t lastByte)
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
                                 size_t countElements,
                                 uint32_t* const out_upperBound)
{
    *out_upperBound = 0;

    
    
    uint32_t maxTSize = std::numeric_limits<T>::max();
    if (maxAllowed >= maxTSize) {
        UpdateUpperBound(out_upperBound, maxTSize);
        return true;
    }

    T maxAllowedT(maxAllowed);

    
    
    MOZ_ASSERT(uint32_t(maxAllowedT) == maxAllowed);

    if (!mBytes.Length() || !countElements)
      return true;

    ScopedDeletePtr<WebGLElementArrayCacheTree<T>>& tree = TreeForType<T>::Value(this);
    if (!tree) {
        tree = new WebGLElementArrayCacheTree<T>(*this);
        if (mBytes.Length()) {
            bool valid = tree->Update(0, mBytes.Length() - 1);
            if (!valid) {
                
                
                
                tree = nullptr;
                return false;
            }
        }
    }

    size_t lastElement = firstElement + countElements - 1;

    
    
    T globalMax = tree->GlobalMaximum();
    if (globalMax <= maxAllowedT) {
        UpdateUpperBound(out_upperBound, globalMax);
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

    
    return tree->Validate(maxAllowedT, tree->LeafForElement(firstElement),
                          tree->LeafForElement(lastElement), out_upperBound);
}

bool
WebGLElementArrayCache::Validate(GLenum type, uint32_t maxAllowed,
                                 size_t firstElement, size_t countElements,
                                 uint32_t* const out_upperBound)
{
    if (type == LOCAL_GL_UNSIGNED_BYTE)
        return Validate<uint8_t>(maxAllowed, firstElement, countElements,
                                 out_upperBound);
    if (type == LOCAL_GL_UNSIGNED_SHORT)
        return Validate<uint16_t>(maxAllowed, firstElement, countElements,
                                  out_upperBound);
    if (type == LOCAL_GL_UNSIGNED_INT)
        return Validate<uint32_t>(maxAllowed, firstElement, countElements,
                                  out_upperBound);

    MOZ_ASSERT(false, "Invalid type.");
    return false;
}

template<typename T>
static size_t
SizeOfNullable(mozilla::MallocSizeOf mallocSizeOf, const T& obj)
{
    if (!obj)
        return 0;
    return obj->SizeOfIncludingThis(mallocSizeOf);
}

size_t
WebGLElementArrayCache::SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
    return mallocSizeOf(this) +
           mBytes.SizeOfExcludingThis(mallocSizeOf) +
           SizeOfNullable(mallocSizeOf, mUint8Tree) +
           SizeOfNullable(mallocSizeOf, mUint16Tree) +
           SizeOfNullable(mallocSizeOf, mUint32Tree);
}

bool
WebGLElementArrayCache::BeenUsedWithMultipleTypes() const
{
  
  
  
  const int num_types_used = (mUint8Tree  != nullptr) +
                             (mUint16Tree != nullptr) +
                             (mUint32Tree != nullptr);
  return num_types_used > 1;
}

} 

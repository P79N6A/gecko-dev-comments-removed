










#ifndef mozilla_SplayTree_h
#define mozilla_SplayTree_h

#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"

namespace mozilla {

template<class T, class C>
class SplayTree;

template<typename T>
class SplayTreeNode
{
public:
  template<class A, class B>
  friend class SplayTree;

  SplayTreeNode()
    : mLeft(nullptr)
    , mRight(nullptr)
    , mParent(nullptr)
  {}

private:
  T* mLeft;
  T* mRight;
  T* mParent;
};












template<typename T, class Comparator>
class SplayTree
{
  T* mRoot;

public:
  SplayTree()
    : mRoot(nullptr)
  {}

  bool empty() const
  {
    return !mRoot;
  }

  T* find(const T& aValue)
  {
    if (empty()) {
      return nullptr;
    }

    T* last = lookup(aValue);
    splay(last);
    checkCoherency(mRoot, nullptr);
    return Comparator::compare(aValue, *last) == 0 ? last : nullptr;
  }

  bool insert(T* aValue)
  {
    MOZ_ASSERT(!find(*aValue), "Duplicate elements are not allowed.");

    if (!mRoot) {
      mRoot = aValue;
      return true;
    }
    T* last = lookup(*aValue);
    int cmp = Comparator::compare(*aValue, *last);

    T** parentPointer = (cmp < 0) ? &last->mLeft : &last->mRight;
    MOZ_ASSERT(!*parentPointer);
    *parentPointer = aValue;
    aValue->mParent = last;

    splay(aValue);
    checkCoherency(mRoot, nullptr);
    return true;
  }

  T* remove(const T& aValue)
  {
    T* last = lookup(aValue);
    MOZ_ASSERT(last, "This tree must contain the element being removed.");
    MOZ_ASSERT(Comparator::compare(aValue, *last) == 0);

    
    splay(last);
    MOZ_ASSERT(last == mRoot);

    
    
    
    T* swap;
    T* swapChild;
    if (mRoot->mLeft) {
      swap = mRoot->mLeft;
      while (swap->mRight) {
        swap = swap->mRight;
      }
      swapChild = swap->mLeft;
    } else if (mRoot->mRight) {
      swap = mRoot->mRight;
      while (swap->mLeft) {
        swap = swap->mLeft;
      }
      swapChild = swap->mRight;
    } else {
      T* result = mRoot;
      mRoot = nullptr;
      return result;
    }

    
    
    if (swap == swap->mParent->mLeft) {
      swap->mParent->mLeft = swapChild;
    } else {
      swap->mParent->mRight = swapChild;
    }
    if (swapChild) {
      swapChild->mParent = swap->mParent;
    }

    
    mRoot = swap;
    mRoot->mParent = nullptr;
    mRoot->mLeft = last->mLeft;
    mRoot->mRight = last->mRight;
    if (mRoot->mLeft) {
      mRoot->mLeft->mParent = mRoot;
    }
    if (mRoot->mRight) {
      mRoot->mRight->mParent = mRoot;
    }

    checkCoherency(mRoot, nullptr);
    return last;
  }

  T* removeMin()
  {
    MOZ_ASSERT(mRoot, "No min to remove!");

    T* min = mRoot;
    while (min->mLeft) {
      min = min->mLeft;
    }
    return remove(*min);
  }

  
  void checkCoherency()
  {
    checkCoherency(mRoot, nullptr);
  }

private:
  



  T* lookup(const T& aValue)
  {
    MOZ_ASSERT(!empty());

    T* node = mRoot;
    T* parent;
    do {
      parent = node;
      int c = Comparator::compare(aValue, *node);
      if (c == 0) {
        return node;
      } else if (c < 0) {
        node = node->mLeft;
      } else {
        node = node->mRight;
      }
    } while (node);
    return parent;
  }

  




  void splay(T* aNode)
  {
    MOZ_ASSERT(aNode);

    while (aNode != mRoot) {
      T* parent = aNode->mParent;
      if (parent == mRoot) {
        
        rotate(aNode);
        MOZ_ASSERT(aNode == mRoot);
        return;
      }
      T* grandparent = parent->mParent;
      if ((parent->mLeft == aNode) == (grandparent->mLeft == parent)) {
        
        rotate(parent);
        rotate(aNode);
      } else {
        
        rotate(aNode);
        rotate(aNode);
      }
    }
  }

  void rotate(T* aNode)
  {
    
    
    T* parent = aNode->mParent;
    if (parent->mLeft == aNode) {
      
      
      
      parent->mLeft = aNode->mRight;
      if (aNode->mRight) {
        aNode->mRight->mParent = parent;
      }
      aNode->mRight = parent;
    } else {
      MOZ_ASSERT(parent->mRight == aNode);
      
      
      
      parent->mRight = aNode->mLeft;
      if (aNode->mLeft) {
        aNode->mLeft->mParent = parent;
      }
      aNode->mLeft = parent;
    }
    aNode->mParent = parent->mParent;
    parent->mParent = aNode;
    if (T* grandparent = aNode->mParent) {
      if (grandparent->mLeft == parent) {
        grandparent->mLeft = aNode;
      } else {
        grandparent->mRight = aNode;
      }
    } else {
      mRoot = aNode;
    }
  }

  T* checkCoherency(T* aNode, T* aMinimum)
  {
#ifdef DEBUG
    MOZ_ASSERT_IF(mRoot, !mRoot->mParent);
    if (!aNode) {
      MOZ_ASSERT(!mRoot);
      return nullptr;
    }
    MOZ_ASSERT_IF(!aNode->mParent, aNode == mRoot);
    MOZ_ASSERT_IF(aMinimum, Comparator::compare(*aMinimum, *aNode) < 0);
    if (aNode->mLeft) {
      MOZ_ASSERT(aNode->mLeft->mParent == aNode);
      T* leftMaximum = checkCoherency(aNode->mLeft, aMinimum);
      MOZ_ASSERT(Comparator::compare(*leftMaximum, *aNode) < 0);
    }
    if (aNode->mRight) {
      MOZ_ASSERT(aNode->mRight->mParent == aNode);
      return checkCoherency(aNode->mRight, aNode);
    }
    return aNode;
#else
    return nullptr;
#endif
  }

  SplayTree(const SplayTree&) MOZ_DELETE;
  void operator=(const SplayTree&) MOZ_DELETE;
};

}  

#endif 

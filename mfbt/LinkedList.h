






























































#ifndef mozilla_LinkedList_h
#define mozilla_LinkedList_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"

#ifdef __cplusplus

namespace mozilla {

template<typename T>
class LinkedList;

template<typename T>
class LinkedListElement
{
  

































private:
  LinkedListElement* mNext;
  LinkedListElement* mPrev;
  const bool mIsSentinel;

public:
  LinkedListElement()
    : mNext(this),
      mPrev(this),
      mIsSentinel(false)
  { }

  LinkedListElement(LinkedListElement<T>&& other)
    : mIsSentinel(other.mIsSentinel)
  {
    if (!other.isInList()) {
      mNext = this;
      mPrev = this;
      return;
    }

    MOZ_ASSERT(other.mNext->mPrev == &other);
    MOZ_ASSERT(other.mPrev->mNext == &other);

    



    mNext = other.mNext;
    mPrev = other.mPrev;

    mNext->mPrev = this;
    mPrev->mNext = this;

    



    other.mNext = &other;
    other.mPrev = &other;
  }

  ~LinkedListElement()
  {
    if (!mIsSentinel && isInList()) {
      remove();
    }
  }

  



  T* getNext()             { return mNext->asT(); }
  const T* getNext() const { return mNext->asT(); }

  



  T* getPrevious()             { return mPrev->asT(); }
  const T* getPrevious() const { return mPrev->asT(); }

  



  void setNext(T* aElem)
  {
    MOZ_ASSERT(isInList());
    setNextUnsafe(aElem);
  }

  




  void setPrevious(T* aElem)
  {
    MOZ_ASSERT(isInList());
    setPreviousUnsafe(aElem);
  }

  



  void remove()
  {
    MOZ_ASSERT(isInList());

    mPrev->mNext = mNext;
    mNext->mPrev = mPrev;
    mNext = this;
    mPrev = this;
  }

  



  void removeFrom(const LinkedList<T>& aList)
  {
    aList.assertContains(asT());
    remove();
  }

  


  bool isInList() const
  {
    MOZ_ASSERT((mNext == this) == (mPrev == this));
    return mNext != this;
  }

private:
  friend class LinkedList<T>;

  enum NodeKind {
    NODE_KIND_NORMAL,
    NODE_KIND_SENTINEL
  };

  explicit LinkedListElement(NodeKind nodeKind)
    : mNext(this),
      mPrev(this),
      mIsSentinel(nodeKind == NODE_KIND_SENTINEL)
  { }

  



  T* asT()
  {
    return mIsSentinel ? nullptr : static_cast<T*>(this);
  }
  const T* asT() const
  {
    return mIsSentinel ? nullptr : static_cast<const T*>(this);
  }

  



  void setNextUnsafe(T* aElem)
  {
    LinkedListElement *listElem = static_cast<LinkedListElement*>(aElem);
    MOZ_ASSERT(!listElem->isInList());

    listElem->mNext = this->mNext;
    listElem->mPrev = this;
    this->mNext->mPrev = listElem;
    this->mNext = listElem;
  }

  



  void setPreviousUnsafe(T* aElem)
  {
    LinkedListElement<T>* listElem = static_cast<LinkedListElement<T>*>(aElem);
    MOZ_ASSERT(!listElem->isInList());

    listElem->mNext = this;
    listElem->mPrev = this->mPrev;
    this->mPrev->mNext = listElem;
    this->mPrev = listElem;
  }

private:
  LinkedListElement& operator=(const LinkedListElement<T>& aOther) = delete;
  LinkedListElement(const LinkedListElement<T>& aOther) = delete;
};

template<typename T>
class LinkedList
{
private:
  LinkedListElement<T> sentinel;

public:
  LinkedList() : sentinel(LinkedListElement<T>::NODE_KIND_SENTINEL) { }

  LinkedList(LinkedList<T>&& aOther)
    : sentinel(mozilla::Move(aOther.sentinel))
  { }

  ~LinkedList() { MOZ_ASSERT(isEmpty()); }

  


  void insertFront(T* aElem)
  {
    
    sentinel.setNextUnsafe(aElem);
  }

  


  void insertBack(T* aElem)
  {
    sentinel.setPreviousUnsafe(aElem);
  }

  


  T* getFirst()             { return sentinel.getNext(); }
  const T* getFirst() const { return sentinel.getNext(); }

  


  T* getLast()             { return sentinel.getPrevious(); }
  const T* getLast() const { return sentinel.getPrevious(); }

  



  T* popFirst()
  {
    T* ret = sentinel.getNext();
    if (ret) {
      static_cast<LinkedListElement<T>*>(ret)->remove();
    }
    return ret;
  }

  



  T* popLast()
  {
    T* ret = sentinel.getPrevious();
    if (ret) {
      static_cast<LinkedListElement<T>*>(ret)->remove();
    }
    return ret;
  }

  


  bool isEmpty() const
  {
    return !sentinel.isInList();
  }

  





  void clear()
  {
    while (popFirst()) {
      continue;
    }
  }

  





  size_t sizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t n = 0;
    for (const T* t = getFirst(); t; t = t->getNext()) {
      n += aMallocSizeOf(t);
    }
    return n;
  }

  


  size_t sizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + sizeOfExcludingThis(aMallocSizeOf);
  }

  



  void debugAssertIsSane() const
  {
#ifdef DEBUG
    const LinkedListElement<T>* slow;
    const LinkedListElement<T>* fast1;
    const LinkedListElement<T>* fast2;

    



    for (slow = sentinel.mNext,
         fast1 = sentinel.mNext->mNext,
         fast2 = sentinel.mNext->mNext->mNext;
         slow != &sentinel && fast1 != &sentinel && fast2 != &sentinel;
         slow = slow->mNext, fast1 = fast2->mNext, fast2 = fast1->mNext) {
      MOZ_ASSERT(slow != fast1);
      MOZ_ASSERT(slow != fast2);
    }

    
    for (slow = sentinel.mPrev,
         fast1 = sentinel.mPrev->mPrev,
         fast2 = sentinel.mPrev->mPrev->mPrev;
         slow != &sentinel && fast1 != &sentinel && fast2 != &sentinel;
         slow = slow->mPrev, fast1 = fast2->mPrev, fast2 = fast1->mPrev) {
      MOZ_ASSERT(slow != fast1);
      MOZ_ASSERT(slow != fast2);
    }

    



    for (const LinkedListElement<T>* elem = sentinel.mNext;
         elem != &sentinel;
         elem = elem->mNext) {
      MOZ_ASSERT(!elem->mIsSentinel);
    }

    
    const LinkedListElement<T>* prev = &sentinel;
    const LinkedListElement<T>* cur = sentinel.mNext;
    do {
        MOZ_ASSERT(cur->mPrev == prev);
        MOZ_ASSERT(prev->mNext == cur);

        prev = cur;
        cur = cur->mNext;
    } while (cur != &sentinel);
#endif 
  }

private:
  friend class LinkedListElement<T>;

  void assertContains(const T* aValue) const
  {
#ifdef DEBUG
    for (const T* elem = getFirst(); elem; elem = elem->getNext()) {
      if (elem == aValue) {
        return;
      }
    }
    MOZ_CRASH("element wasn't found in this list!");
#endif
  }

  LinkedList& operator=(const LinkedList<T>& aOther) = delete;
  LinkedList(const LinkedList<T>& aOther) = delete;
};

template <typename T>
class AutoCleanLinkedList : public LinkedList<T>
{
public:
  ~AutoCleanLinkedList()
  {
    while (T* element = this->popFirst()) {
      delete element;
    }
  }
};

} 

#endif 

#endif 

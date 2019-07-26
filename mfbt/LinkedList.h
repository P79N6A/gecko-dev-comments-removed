






















































#ifndef mozilla_LinkedList_h
#define mozilla_LinkedList_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "mozilla/NullPtr.h"

#ifdef __cplusplus

namespace mozilla {

template<typename T>
class LinkedList;

template<typename T>
class LinkedListElement
{
    

































  private:
    LinkedListElement* next;
    LinkedListElement* prev;
    const bool isSentinel;

  public:
    LinkedListElement()
      : next(MOZ_THIS_IN_INITIALIZER_LIST()),
        prev(MOZ_THIS_IN_INITIALIZER_LIST()),
        isSentinel(false)
    { }

    LinkedListElement(LinkedListElement<T>&& other)
      : isSentinel(other.isSentinel)
    {
      if (!other.isInList()) {
        next = this;
        prev = this;
        return;
      }

      MOZ_ASSERT(other.next->prev == &other);
      MOZ_ASSERT(other.prev->next == &other);

      



      next = other.next;
      prev = other.prev;

      next->prev = this;
      prev->next = this;

      



      other.next = &other;
      other.prev = &other;
    }

    ~LinkedListElement() {
      if (!isSentinel && isInList())
        remove();
    }

    



    T* getNext() {
      return next->asT();
    }
    const T* getNext() const {
      return next->asT();
    }

    



    T* getPrevious() {
      return prev->asT();
    }
    const T* getPrevious() const {
      return prev->asT();
    }

    



    void setNext(T* elem) {
      MOZ_ASSERT(isInList());
      setNextUnsafe(elem);
    }

    




    void setPrevious(T* elem) {
      MOZ_ASSERT(isInList());
      setPreviousUnsafe(elem);
    }

    



    void remove() {
      MOZ_ASSERT(isInList());

      prev->next = next;
      next->prev = prev;
      next = this;
      prev = this;
    }

    



    void removeFrom(const LinkedList<T>& list) {
      list.assertContains(asT());
      remove();
    }

    


    bool isInList() const {
      MOZ_ASSERT((next == this) == (prev == this));
      return next != this;
    }

  private:
    friend class LinkedList<T>;

    enum NodeKind {
      NODE_KIND_NORMAL,
      NODE_KIND_SENTINEL
    };

    explicit LinkedListElement(NodeKind nodeKind)
      : next(MOZ_THIS_IN_INITIALIZER_LIST()),
        prev(MOZ_THIS_IN_INITIALIZER_LIST()),
        isSentinel(nodeKind == NODE_KIND_SENTINEL)
    { }

    



    T* asT() {
      if (isSentinel)
        return nullptr;

      return static_cast<T*>(this);
    }
    const T* asT() const {
      if (isSentinel)
        return nullptr;

      return static_cast<const T*>(this);
    }

    



    void setNextUnsafe(T* elem) {
      LinkedListElement *listElem = static_cast<LinkedListElement*>(elem);
      MOZ_ASSERT(!listElem->isInList());

      listElem->next = this->next;
      listElem->prev = this;
      this->next->prev = listElem;
      this->next = listElem;
    }

    



    void setPreviousUnsafe(T* elem) {
      LinkedListElement<T>* listElem = static_cast<LinkedListElement<T>*>(elem);
      MOZ_ASSERT(!listElem->isInList());

      listElem->next = this;
      listElem->prev = this->prev;
      this->prev->next = listElem;
      this->prev = listElem;
    }

  private:
    LinkedListElement& operator=(const LinkedListElement<T>& other) MOZ_DELETE;
    LinkedListElement(const LinkedListElement<T>& other) MOZ_DELETE;
};

template<typename T>
class LinkedList
{
  private:
    LinkedListElement<T> sentinel;

  public:
    LinkedList() : sentinel(LinkedListElement<T>::NODE_KIND_SENTINEL) { }

    LinkedList(LinkedList<T>&& other)
      : sentinel(mozilla::Move(other.sentinel))
    { }

    ~LinkedList() {
      MOZ_ASSERT(isEmpty());
    }

    


    void insertFront(T* elem) {
      
      sentinel.setNextUnsafe(elem);
    }

    


    void insertBack(T* elem) {
      sentinel.setPreviousUnsafe(elem);
    }

    


    T* getFirst() {
      return sentinel.getNext();
    }
    const T* getFirst() const {
      return sentinel.getNext();
    }

    


    T* getLast() {
      return sentinel.getPrevious();
    }
    const T* getLast() const {
      return sentinel.getPrevious();
    }

    



    T* popFirst() {
      T* ret = sentinel.getNext();
      if (ret)
        static_cast<LinkedListElement<T>*>(ret)->remove();
      return ret;
    }

    



    T* popLast() {
      T* ret = sentinel.getPrevious();
      if (ret)
        static_cast<LinkedListElement<T>*>(ret)->remove();
      return ret;
    }

    


    bool isEmpty() const {
      return !sentinel.isInList();
    }

    





    void clear() {
      while (popFirst())
        continue;
    }

    





    size_t sizeOfExcludingThis(MallocSizeOf mallocSizeOf) const {
      size_t n = 0;
      for (const T* t = getFirst(); t; t = t->getNext())
        n += mallocSizeOf(t);
      return n;
    }

    


    size_t sizeOfIncludingThis(MallocSizeOf mallocSizeOf) const {
      return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
    }

    



    void debugAssertIsSane() const {
#ifdef DEBUG
      const LinkedListElement<T>* slow;
      const LinkedListElement<T>* fast1;
      const LinkedListElement<T>* fast2;

      



      for (slow = sentinel.next,
           fast1 = sentinel.next->next,
           fast2 = sentinel.next->next->next;
           slow != &sentinel && fast1 != &sentinel && fast2 != &sentinel;
           slow = slow->next, fast1 = fast2->next, fast2 = fast1->next)
      {
        MOZ_ASSERT(slow != fast1);
        MOZ_ASSERT(slow != fast2);
      }

      
      for (slow = sentinel.prev,
           fast1 = sentinel.prev->prev,
           fast2 = sentinel.prev->prev->prev;
           slow != &sentinel && fast1 != &sentinel && fast2 != &sentinel;
           slow = slow->prev, fast1 = fast2->prev, fast2 = fast1->prev)
      {
        MOZ_ASSERT(slow != fast1);
        MOZ_ASSERT(slow != fast2);
      }

      



      for (const LinkedListElement<T>* elem = sentinel.next;
           elem != &sentinel;
           elem = elem->next)
      {
        MOZ_ASSERT(!elem->isSentinel);
      }

      
      const LinkedListElement<T>* prev = &sentinel;
      const LinkedListElement<T>* cur = sentinel.next;
      do {
          MOZ_ASSERT(cur->prev == prev);
          MOZ_ASSERT(prev->next == cur);

          prev = cur;
          cur = cur->next;
      } while (cur != &sentinel);
#endif 
    }

  private:
    friend class LinkedListElement<T>;

    void assertContains(const T* t) const {
#ifdef DEBUG
      for (const T* elem = getFirst();
           elem;
           elem = elem->getNext())
      {
        if (elem == t)
          return;
      }
      MOZ_CRASH("element wasn't found in this list!");
#endif
    }

    LinkedList& operator=(const LinkedList<T>& other) MOZ_DELETE;
    LinkedList(const LinkedList<T>& other) MOZ_DELETE;
};

} 

#endif 

#endif 


























































































#ifndef mozilla_LinkedList_h_
#define mozilla_LinkedList_h_

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

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
        : next(this)
        , prev(this)
        , isSentinel(false)
    {
    }

    



    T* getNext()
    {
        return next->asT();
    }

    



    T* getPrevious()
    {
        return prev->asT();
    }

    



    void setNext(T* elem)
    {
        MOZ_ASSERT(isInList());
        setNextUnsafe(elem);
    }

    




    void setPrevious(T* elem)
    {
        MOZ_ASSERT(isInList());
        setPreviousUnsafe(elem);
    }

    



    void remove()
    {
        MOZ_ASSERT(isInList());

        prev->next = next;
        next->prev = prev;
        next = this;
        prev = this;
    }

    


    bool isInList()
    {
        MOZ_ASSERT((next == this) == (prev == this));
        return next != this;
    }

private:
    LinkedListElement& operator=(const LinkedList<T>& other) MOZ_DELETE;
    LinkedListElement(const LinkedList<T>& other) MOZ_DELETE;

    friend class LinkedList<T>;

    enum NodeKind {
        NODE_KIND_NORMAL,
        NODE_KIND_SENTINEL
    };

    LinkedListElement(NodeKind nodeKind)
        : next(this)
        , prev(this)
        , isSentinel(nodeKind == NODE_KIND_SENTINEL)
    {
    }

    



    T* asT()
    {
        if (isSentinel)
            return NULL;

        return static_cast<T*>(this);
    }

    



    void setNextUnsafe(T* elem)
    {
        LinkedListElement *listElem = static_cast<LinkedListElement*>(elem);
        MOZ_ASSERT(!listElem->isInList());

        listElem->next = this->next;
        listElem->prev = this;
        this->next->prev = listElem;
        this->next = listElem;
    }

    



    void setPreviousUnsafe(T* elem)
    {
        LinkedListElement<T>* listElem = static_cast<LinkedListElement<T>*>(elem);
        MOZ_ASSERT(!listElem->isInList());

        listElem->next = this;
        listElem->prev = this->prev;
        this->prev->next = listElem;
        this->prev = listElem;
    }
};

template<typename T>
class LinkedList
{
private:
    LinkedListElement<T> sentinel;

public:
    LinkedList& operator=(const LinkedList<T>& other) MOZ_DELETE;
    LinkedList(const LinkedList<T>& other) MOZ_DELETE;

    LinkedList()
        : sentinel(LinkedListElement<T>::NODE_KIND_SENTINEL)
    {
    }

    


    void insertFront(T* elem)
    {
        
        sentinel.setNextUnsafe(elem);
    }

    


    void insertBack(T* elem)
    {
        sentinel.setPreviousUnsafe(elem);
    }

    


    T* getFirst()
    {
        return sentinel.getNext();
    }

    


    T* getLast()
    {
        return sentinel.getPrevious();
    }

    



    T* popFirst()
    {
        T* ret = sentinel.getNext();
        if (ret)
            static_cast<LinkedListElement<T>*>(ret)->remove();

        return ret;
    }

    



    T* popLast()
    {
        T* ret = sentinel.getPrevious();
        if (ret)
            static_cast<LinkedListElement<T>*>(ret)->remove();

        return ret;
    }

    


    bool isEmpty()
    {
        return !sentinel.isInList();
    }

    



    void debugAssertIsSane()
    {
#ifdef DEBUG
        



        for (LinkedListElement<T>* slow = sentinel.next,
                                 * fast1 = sentinel.next->next,
                                 * fast2 = sentinel.next->next->next;
             slow != sentinel && fast1 != sentinel && fast2 != sentinel;
             slow = slow->next,
             fast1 = fast2->next,
             fast2 = fast1->next) {

            MOZ_ASSERT(slow != fast1);
            MOZ_ASSERT(slow != fast2);
        }

        
        for (LinkedListElement<T>* slow = sentinel.prev,
                                 * fast1 = sentinel.prev->prev,
                                 * fast2 = sentinel.prev->prev->prev;
             slow != sentinel && fast1 != sentinel && fast2 != sentinel;
             slow = slow->prev,
             fast1 = fast2->prev,
             fast2 = fast1->prev) {

            MOZ_ASSERT(slow != fast1);
            MOZ_ASSERT(slow != fast2);
        }

        



        for (LinkedListElement<T>* elem = sentinel.next;
             elem != sentinel;
             elem = elem->next) {

          MOZ_ASSERT(!elem->isSentinel);
        }

        
        LinkedListElement<T>* prev = sentinel;
        LinkedListElement<T>* cur = sentinel.next;
        do {
            MOZ_ASSERT(cur->prev == prev);
            MOZ_ASSERT(prev->next == cur);

            prev = cur;
            cur = cur->next;
        } while (cur != sentinel);
#endif 
    }
};

} 

#endif 
#endif 

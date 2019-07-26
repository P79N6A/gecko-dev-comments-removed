

























#ifndef _SYSUTILS_LIST_H
#define _SYSUTILS_LIST_H

#include <stddef.h>
#include <stdint.h>

namespace stagefright {
namespace sysutils {







template<typename T> 
class List 
{
protected:
    


    class _Node {
    public:
        explicit _Node(const T& val) : mVal(val) {}
        ~_Node() {}
        inline T& getRef() { return mVal; }
        inline const T& getRef() const { return mVal; }
        inline _Node* getPrev() const { return mpPrev; }
        inline _Node* getNext() const { return mpNext; }
        inline void setVal(const T& val) { mVal = val; }
        inline void setPrev(_Node* ptr) { mpPrev = ptr; }
        inline void setNext(_Node* ptr) { mpNext = ptr; }
    private:
        friend class List;
        friend class _ListIterator;
        T           mVal;
        _Node*      mpPrev;
        _Node*      mpNext;
    };

    


    
    template <typename TYPE>
    struct CONST_ITERATOR {
        typedef _Node const * NodePtr;
        typedef const TYPE Type;
    };
    
    template <typename TYPE>
    struct NON_CONST_ITERATOR {
        typedef _Node* NodePtr;
        typedef TYPE Type;
    };
    
    template<
        typename U,
        template <class> class Constness
    > 
    class _ListIterator {
        typedef _ListIterator<U, Constness>     _Iter;
        typedef typename Constness<U>::NodePtr  _NodePtr;
        typedef typename Constness<U>::Type     _Type;

        explicit _ListIterator(_NodePtr ptr) : mpNode(ptr) {}

    public:
        _ListIterator() {}
        _ListIterator(const _Iter& rhs) : mpNode(rhs.mpNode) {}
        ~_ListIterator() {}
        
        
        
        
        
        template<typename V> explicit 
        _ListIterator(const V& rhs) : mpNode(rhs.mpNode) {}
        

        


        _Type& operator*() const { return mpNode->getRef(); }
        _Type* operator->() const { return &(mpNode->getRef()); }

        


        inline bool operator==(const _Iter& right) const { 
            return mpNode == right.mpNode; }
        
        inline bool operator!=(const _Iter& right) const { 
            return mpNode != right.mpNode; }

        


        template<typename OTHER>
        inline bool operator==(const OTHER& right) const { 
            return mpNode == right.mpNode; }
        
        template<typename OTHER>
        inline bool operator!=(const OTHER& right) const { 
            return mpNode != right.mpNode; }

        


        inline _Iter& operator++() {     
            mpNode = mpNode->getNext();
            return *this;
        }
        const _Iter operator++(int) {    
            _Iter tmp(*this);
            mpNode = mpNode->getNext();
            return tmp;
        }
        inline _Iter& operator--() {     
            mpNode = mpNode->getPrev();
            return *this;
        }
        const _Iter operator--(int) {   
            _Iter tmp(*this);
            mpNode = mpNode->getPrev();
            return tmp;
        }

        inline _NodePtr getNode() const { return mpNode; }

        _NodePtr mpNode;    
    private:
        friend class List;
    };

public:
    List() {
        prep();
    }
    List(const List<T>& src) {      
        prep();
        insert(begin(), src.begin(), src.end());
    }
    virtual ~List() {
        clear();
        delete[] (unsigned char*) mpMiddle;
    }

    typedef _ListIterator<T, NON_CONST_ITERATOR> iterator;
    typedef _ListIterator<T, CONST_ITERATOR> const_iterator;

    List<T>& operator=(const List<T>& right);

    
    inline bool empty() const { return mpMiddle->getNext() == mpMiddle; }

    
    size_t size() const {
        return size_t(distance(begin(), end()));
    }

    




    inline iterator begin() { 
        return iterator(mpMiddle->getNext()); 
    }
    inline const_iterator begin() const { 
        return const_iterator(const_cast<_Node const*>(mpMiddle->getNext())); 
    }
    inline iterator end() { 
        return iterator(mpMiddle); 
    }
    inline const_iterator end() const { 
        return const_iterator(const_cast<_Node const*>(mpMiddle)); 
    }

    
    void push_front(const T& val) { insert(begin(), val); }
    void push_back(const T& val) { insert(end(), val); }

    
    iterator insert(iterator posn, const T& val) 
    {
        _Node* newNode = new _Node(val);        
        newNode->setNext(posn.getNode());
        newNode->setPrev(posn.getNode()->getPrev());
        posn.getNode()->getPrev()->setNext(newNode);
        posn.getNode()->setPrev(newNode);
        return iterator(newNode);
    }

    
    void insert(iterator posn, const_iterator first, const_iterator last) {
        for ( ; first != last; ++first)
            insert(posn, *first);
    }

    
    iterator erase(iterator posn) {
        _Node* pNext = posn.getNode()->getNext();
        _Node* pPrev = posn.getNode()->getPrev();
        pPrev->setNext(pNext);
        pNext->setPrev(pPrev);
        delete posn.getNode();
        return iterator(pNext);
    }

    
    iterator erase(iterator first, iterator last) {
        while (first != last)
            erase(first++);     
        return iterator(last);
    }

    
    void clear() {
        _Node* pCurrent = mpMiddle->getNext();
        _Node* pNext;

        while (pCurrent != mpMiddle) {
            pNext = pCurrent->getNext();
            delete pCurrent;
            pCurrent = pNext;
        }
        mpMiddle->setPrev(mpMiddle);
        mpMiddle->setNext(mpMiddle);
    }

    









    template<
        typename U,
        template <class> class CL,
        template <class> class CR
    > 
    ptrdiff_t distance(
            _ListIterator<U, CL> first, _ListIterator<U, CR> last) const 
    {
        ptrdiff_t count = 0;
        while (first != last) {
            ++first;
            ++count;
        }
        return count;
    }

private:
    





    void prep() {
        mpMiddle = (_Node*) new unsigned char[sizeof(_Node)];
        mpMiddle->setPrev(mpMiddle);
        mpMiddle->setNext(mpMiddle);
    }

    




    _Node*      mpMiddle;
};








template<class T>
List<T>& List<T>::operator=(const List<T>& right)
{
    if (this == &right)
        return *this;       
    iterator firstDst = begin();
    iterator lastDst = end();
    const_iterator firstSrc = right.begin();
    const_iterator lastSrc = right.end();
    while (firstSrc != lastSrc && firstDst != lastDst)
        *firstDst++ = *firstSrc++;
    if (firstSrc == lastSrc)        
        erase(firstDst, lastDst);   
    else
        insert(lastDst, firstSrc, lastSrc);     
    return *this;
}

}; 
}; 

#endif 

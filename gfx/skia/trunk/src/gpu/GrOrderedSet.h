






#ifndef GrOrderedSet_DEFINED
#define GrOrderedSet_DEFINED

#include "GrRedBlackTree.h"

template <typename T, typename C = GrLess<T> >
class GrOrderedSet : SkNoncopyable {
public:
    


    GrOrderedSet() : fComp() {}
    ~GrOrderedSet() {}

    class Iter;

    


    bool empty() const { return fRBTree.empty(); }

    


    int count() const { return fRBTree.count(); }

    


    void reset() { fRBTree.reset(); }

    




    Iter insert(const T& t);

    




    void remove(const Iter& iter);

    


    Iter begin();

    




    Iter end();

    



    Iter last();

    




    Iter find(const T& t);

private:
    GrRedBlackTree<T, C> fRBTree;

    const C fComp;
};

template <typename T, typename C>
class GrOrderedSet<T,C>::Iter {
public:
    Iter() {}
    Iter(const Iter& i) { fTreeIter = i.fTreeIter; }
    Iter& operator =(const Iter& i) {
        fTreeIter = i.fTreeIter;
        return *this;
    }
    const T& operator *() const { return *fTreeIter; }
    bool operator ==(const Iter& i) const {
        return fTreeIter == i.fTreeIter;
    }
    bool operator !=(const Iter& i) const { return !(*this == i); }
    Iter& operator ++() {
        ++fTreeIter;
        return *this;
    }
    Iter& operator --() {
        --fTreeIter;
        return *this;
    }
    const typename GrRedBlackTree<T,C>::Iter& getTreeIter() const {
        return fTreeIter;
    }

private:
    friend class GrOrderedSet;
    explicit Iter(typename GrRedBlackTree<T, C>::Iter iter) {
        fTreeIter = iter;
    }
    typename GrRedBlackTree<T,C>::Iter fTreeIter;
};

template <typename T, typename C>
typename GrOrderedSet<T,C>::Iter GrOrderedSet<T,C>::begin() {
    return Iter(fRBTree.begin());
}

template <typename T, typename C>
typename GrOrderedSet<T,C>::Iter GrOrderedSet<T,C>::end() {
    return Iter(fRBTree.end());
}

template <typename T, typename C>
typename GrOrderedSet<T,C>::Iter GrOrderedSet<T,C>::last() {
    return Iter(fRBTree.last());
}

template <typename T, typename C>
typename GrOrderedSet<T,C>::Iter GrOrderedSet<T,C>::find(const T& t) {
    return Iter(fRBTree.find(t));
}

template <typename T, typename C>
typename GrOrderedSet<T,C>::Iter GrOrderedSet<T,C>::insert(const T& t) {
    if (fRBTree.find(t) == fRBTree.end()) {
        return Iter(fRBTree.insert(t));
    } else {
        return Iter(fRBTree.find(t));
    }
}

template <typename T, typename C>
void GrOrderedSet<T,C>::remove(const typename GrOrderedSet<T,C>::Iter& iter) {
    if (this->end() != iter) {
        fRBTree.remove(iter.getTreeIter());
    }
}

#endif

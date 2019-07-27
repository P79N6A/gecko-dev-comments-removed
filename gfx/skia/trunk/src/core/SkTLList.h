






#ifndef SkTLList_DEFINED
#define SkTLList_DEFINED

#include "SkTInternalLList.h"
#include "SkTemplates.h"

template <typename T> class SkTLList;
template <typename T>
inline void* operator new(size_t, SkTLList<T>* list,
                          typename SkTLList<T>::Placement placement,
                          const typename SkTLList<T>::Iter& location);











template <typename T>
class SkTLList : SkNoncopyable {
private:
    struct Block;
    struct Node {
        char fObj[sizeof(T)];
        SK_DECLARE_INTERNAL_LLIST_INTERFACE(Node);
        Block* fBlock; 
    };
    typedef SkTInternalLList<Node> NodeList;

public:

    class Iter;

    

    SkTLList(int allocCnt = 1) : fCount(0), fAllocCnt(allocCnt) {
        SkASSERT(allocCnt > 0);
        this->validate();
    }

    ~SkTLList() {
        this->validate();
        typename NodeList::Iter iter;
        Node* node = iter.init(fList, Iter::kHead_IterStart);
        while (NULL != node) {
            SkTCast<T*>(node->fObj)->~T();
            Block* block = node->fBlock;
            node = iter.next();
            if (0 == --block->fNodesInUse) {
                for (int i = 0; i < fAllocCnt; ++i) {
                    block->fNodes[i].~Node();
                }
                sk_free(block);
            }
        }
    }

    T* addToHead(const T& t) {
        this->validate();
        Node* node = this->createNode();
        fList.addToHead(node);
        SkNEW_PLACEMENT_ARGS(node->fObj, T, (t));
        this->validate();
        return reinterpret_cast<T*>(node->fObj);
    }

    T* addToHead() {
        this->validate();
        Node* node = this->createNode();
        fList.addToHead(node);
        SkNEW_PLACEMENT(node->fObj, T);
        this->validate();
        return reinterpret_cast<T*>(node->fObj);
    }

    T* addToTail(const T& t) {
        this->validate();
        Node* node = this->createNode();
        fList.addToTail(node);
        SkNEW_PLACEMENT_ARGS(node->fObj, T, (t));
        this->validate();
        return reinterpret_cast<T*>(node->fObj);
    }

    T* addToTail() {
        this->validate();
        Node* node = this->createNode();
        fList.addToTail(node);
        SkNEW_PLACEMENT(node->fObj, T);
        this->validate();
        return reinterpret_cast<T*>(node->fObj);
    }

    

    T* addBefore(const T& t, const Iter& location) {
        return SkNEW_PLACEMENT_ARGS(this->internalAddBefore(location), T, (t));
    }

    

    T* addAfter(const T& t, const Iter& location) {
        return SkNEW_PLACEMENT_ARGS(this->internalAddAfter(location), T, (t));
    }

    
    Iter headIter() const { return Iter(*this, Iter::kHead_IterStart); }
    Iter tailIter() const { return Iter(*this, Iter::kTail_IterStart); }

    T* head() { return Iter(*this, Iter::kHead_IterStart).get(); }
    T* tail() { return Iter(*this, Iter::kTail_IterStart).get(); }
    const T* head() const { return Iter(*this, Iter::kHead_IterStart).get(); }
    const T* tail() const { return Iter(*this, Iter::kTail_IterStart).get(); }

    void popHead() {
        this->validate();
        Node* node = fList.head();
        if (NULL != node) {
            this->removeNode(node);
        }
        this->validate();
    }

    void popTail() {
        this->validate();
        Node* node = fList.head();
        if (NULL != node) {
            this->removeNode(node);
        }
        this->validate();
    }

    void remove(T* t) {
        this->validate();
        Node* node = reinterpret_cast<Node*>(t);
        SkASSERT(reinterpret_cast<T*>(node->fObj) == t);
        this->removeNode(node);
        this->validate();
    }

    void reset() {
        this->validate();
        Iter iter(*this, Iter::kHead_IterStart);
        while (iter.get()) {
            Iter next = iter;
            next.next();
            this->remove(iter.get());
            iter = next;
        }
        SkASSERT(0 == fCount);
        this->validate();
    }

    int count() const { return fCount; }
    bool isEmpty() const { this->validate(); return 0 == fCount; }

    bool operator== (const SkTLList& list) const {
        if (this == &list) {
            return true;
        }
        if (fCount != list.fCount) {
            return false;
        }
        for (Iter a(*this, Iter::kHead_IterStart), b(list, Iter::kHead_IterStart);
             a.get();
             a.next(), b.next()) {
            SkASSERT(NULL != b.get()); 
            if (!(*a.get() == *b.get())) {
                return false;
            }
        }
        return true;
    }
    bool operator!= (const SkTLList& list) const { return !(*this == list); }

    
    class Iter : private NodeList::Iter {
    private:
        typedef typename NodeList::Iter INHERITED;

    public:
        typedef typename INHERITED::IterStart IterStart;
        
        static const IterStart kHead_IterStart = INHERITED::kHead_IterStart;
        
        static const IterStart kTail_IterStart = INHERITED::kTail_IterStart;

        Iter() {}

        Iter(const SkTLList& list, IterStart start = kHead_IterStart) {
            INHERITED::init(list.fList, start);
        }

        T* init(const SkTLList& list, IterStart start = kHead_IterStart) {
            return this->nodeToObj(INHERITED::init(list.fList, start));
        }

        T* get() { return this->nodeToObj(INHERITED::get()); }

        T* next() { return this->nodeToObj(INHERITED::next()); }

        T* prev() { return this->nodeToObj(INHERITED::prev()); }

        Iter& operator= (const Iter& iter) { INHERITED::operator=(iter); return *this; }

    private:
        friend class SkTLList;
        Node* getNode() { return INHERITED::get(); }

        T* nodeToObj(Node* node) {
            if (NULL != node) {
                return reinterpret_cast<T*>(node->fObj);
            } else {
                return NULL;
            }
        }
    };

    
    enum Placement {
        kBefore_Placement,
        kAfter_Placement,
    };

private:
    struct Block {
        int fNodesInUse;
        Node fNodes[1];
    };

    size_t blockSize() const { return sizeof(Block) + sizeof(Node) * (fAllocCnt-1); }

    Node* createNode() {
        Node* node = fFreeList.head();
        if (NULL != node) {
            fFreeList.remove(node);
            ++node->fBlock->fNodesInUse;
        } else {
            Block* block = reinterpret_cast<Block*>(sk_malloc_flags(this->blockSize(), 0));
            node = &block->fNodes[0];
            SkNEW_PLACEMENT(node, Node);
            node->fBlock = block;
            block->fNodesInUse = 1;
            for (int i = 1; i < fAllocCnt; ++i) {
                SkNEW_PLACEMENT(block->fNodes + i, Node);
                fFreeList.addToHead(block->fNodes + i);
                block->fNodes[i].fBlock = block;
            }
        }
        ++fCount;
        return node;
    }

    void removeNode(Node* node) {
        SkASSERT(NULL != node);
        fList.remove(node);
        SkTCast<T*>(node->fObj)->~T();
        if (0 == --node->fBlock->fNodesInUse) {
            Block* block = node->fBlock;
            for (int i = 0; i < fAllocCnt; ++i) {
                if (block->fNodes + i != node) {
                    fFreeList.remove(block->fNodes + i);
                }
                block->fNodes[i].~Node();
            }
            sk_free(block);
        } else {
            fFreeList.addToHead(node);
        }
        --fCount;
        this->validate();
    }

    void validate() const {
#ifdef SK_DEBUG
        SkASSERT((0 == fCount) == fList.isEmpty());
        SkASSERT((0 != fCount) || fFreeList.isEmpty());

        fList.validate();
        fFreeList.validate();
        typename NodeList::Iter iter;
        Node* freeNode = iter.init(fFreeList, Iter::kHead_IterStart);
        while (freeNode) {
            SkASSERT(fFreeList.isInList(freeNode));
            Block* block = freeNode->fBlock;
            SkASSERT(block->fNodesInUse > 0 && block->fNodesInUse < fAllocCnt);

            int activeCnt = 0;
            int freeCnt = 0;
            for (int i = 0; i < fAllocCnt; ++i) {
                bool free = fFreeList.isInList(block->fNodes + i);
                bool active = fList.isInList(block->fNodes + i);
                SkASSERT(free != active);
                activeCnt += active;
                freeCnt += free;
            }
            SkASSERT(activeCnt == block->fNodesInUse);
            freeNode = iter.next();
        }

        int count = 0;
        Node* activeNode = iter.init(fList, Iter::kHead_IterStart);
        while (activeNode) {
            ++count;
            SkASSERT(fList.isInList(activeNode));
            Block* block = activeNode->fBlock;
            SkASSERT(block->fNodesInUse > 0 && block->fNodesInUse <= fAllocCnt);

            int activeCnt = 0;
            int freeCnt = 0;
            for (int i = 0; i < fAllocCnt; ++i) {
                bool free = fFreeList.isInList(block->fNodes + i);
                bool active = fList.isInList(block->fNodes + i);
                SkASSERT(free != active);
                activeCnt += active;
                freeCnt += free;
            }
            SkASSERT(activeCnt == block->fNodesInUse);
            activeNode = iter.next();
        }
        SkASSERT(count == fCount);
#endif
    }

    
    friend void* operator new<T>(size_t,
                                 SkTLList* list,
                                 Placement placement,
                                 const Iter& location);


    
    void* internalAddBefore(Iter location) {
        this->validate();
        Node* node = this->createNode();
        fList.addBefore(node, location.getNode());
        this->validate();
        return node->fObj;
    }

    void* internalAddAfter(Iter location) {
        this->validate();
        Node* node = this->createNode();
        fList.addAfter(node, location.getNode());
        this->validate();
        return node->fObj;
    }

    NodeList fList;
    NodeList fFreeList;
    int fCount;
    int fAllocCnt;

};


template <typename T>
void *operator new(size_t, SkTLList<T>* list,
                   typename SkTLList<T>::Placement placement,
                   const typename SkTLList<T>::Iter& location) {
    SkASSERT(NULL != list);
    if (SkTLList<T>::kBefore_Placement == placement) {
        return list->internalAddBefore(location);
    } else {
        return list->internalAddAfter(location);
    }
}




template <typename T>
void operator delete(void*,
                     SkTLList<T>*,
                     typename SkTLList<T>::Placement,
                     const typename SkTLList<T>::Iter&) {
    SK_CRASH();
}

#define SkNEW_INSERT_IN_LLIST_BEFORE(list, location, type_name, args) \
    (new ((list), SkTLList< type_name >::kBefore_Placement, (location)) type_name args)

#define SkNEW_INSERT_IN_LLIST_AFTER(list, location, type_name, args) \
    (new ((list), SkTLList< type_name >::kAfter_Placement, (location)) type_name args)

#define SkNEW_INSERT_AT_LLIST_HEAD(list, type_name, args) \
    SkNEW_INSERT_IN_LLIST_BEFORE((list), (list)->headIter(), type_name, args)

#define SkNEW_INSERT_AT_LLIST_TAIL(list, type_name, args) \
    SkNEW_INSERT_IN_LLIST_AFTER((list), (list)->tailIter(), type_name, args)

#endif

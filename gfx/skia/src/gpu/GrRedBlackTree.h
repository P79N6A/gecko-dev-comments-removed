








#ifndef GrRedBlackTree_DEFINED
#define GrRedBlackTree_DEFINED

#include "GrNoncopyable.h"

template <typename T>
class GrLess {
public:
    bool operator()(const T& a, const T& b) const { return a < b; }
};

template <typename T>
class GrLess<T*> {
public:
    bool operator()(const T* a, const T* b) const { return *a < *b; }
};





#define DEEP_VALIDATE 0






template <typename T, typename C = GrLess<T> >
class GrRedBlackTree : public GrNoncopyable {
public:
    


    GrRedBlackTree();
    virtual ~GrRedBlackTree();

    






    class Iter;

    




    Iter insert(const T& t);

    


    void reset();

    


    bool empty() const {return 0 == fCount;}

    


    int  count() const {return fCount;}

    


    Iter begin();
    




    Iter end();
    



    Iter last();

    




    Iter find(const T& t);
    





    Iter findFirst(const T& t);
    





    Iter findLast(const T& t);
    




    int countOf(const T& t) const;

    





    void remove(const Iter& iter) { deleteAtNode(iter.fN); }

    static void UnitTest();

private:
    enum Color {
        kRed_Color,
        kBlack_Color
    };

    enum Child {
        kLeft_Child  = 0,
        kRight_Child = 1
    };

    struct Node {
        T       fItem;
        Color   fColor;

        Node*   fParent;
        Node*   fChildren[2];
    };

    void rotateRight(Node* n);
    void rotateLeft(Node* n);

    static Node* SuccessorNode(Node* x);
    static Node* PredecessorNode(Node* x);

    void deleteAtNode(Node* x);
    static void RecursiveDelete(Node* x);

    int onCountOf(const Node* n, const T& t) const;

#if GR_DEBUG
    void validate() const;
    int checkNode(Node* n, int* blackHeight) const;
    
    
    bool validateChildRelations(const Node* n, bool allowRedRed) const;
    
    bool validateChildRelationsFailed() const { return false; }
#else
    void validate() const {}
#endif

    int     fCount;
    Node*   fRoot;
    Node*   fFirst;
    Node*   fLast;

    const C fComp;
};

template <typename T, typename C>
class GrRedBlackTree<T,C>::Iter {
public:
    Iter() {};
    Iter(const Iter& i) {fN = i.fN; fTree = i.fTree;}
    Iter& operator =(const Iter& i) {
        fN = i.fN;
        fTree = i.fTree;
        return *this;
    }
    
    
    T& operator *() const { return fN->fItem; }
    bool operator ==(const Iter& i) const {
        return fN == i.fN && fTree == i.fTree;
    }
    bool operator !=(const Iter& i) const { return !(*this == i); }
    Iter& operator ++() {
        GrAssert(*this != fTree->end());
        fN = SuccessorNode(fN);
        return *this;
    }
    Iter& operator --() {
        GrAssert(*this != fTree->begin());
        if (NULL != fN) {
            fN = PredecessorNode(fN);
        } else {
            *this = fTree->last();
        }
        return *this;
    }

private:
    friend class GrRedBlackTree;
    explicit Iter(Node* n, GrRedBlackTree* tree) {
        fN = n;
        fTree = tree;
    }
    Node* fN;
    GrRedBlackTree* fTree;
};

template <typename T, typename C>
GrRedBlackTree<T,C>::GrRedBlackTree() : fComp() {
    fRoot = NULL;
    fFirst = NULL;
    fLast = NULL;
    fCount = 0;
    validate();
}

template <typename T, typename C>
GrRedBlackTree<T,C>::~GrRedBlackTree() {
    RecursiveDelete(fRoot);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::begin() {
    return Iter(fFirst, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::end() {
    return Iter(NULL, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::last() {
    return Iter(fLast, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::find(const T& t) {
    Node* n = fRoot;
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                return Iter(n, this);
            }
            n = n->fChildren[kRight_Child];
        }
    }
    return end();
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::findFirst(const T& t) {
    Node* n = fRoot;
    Node* leftMost = NULL;
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                
                leftMost = n;
                n = n->fChildren[kLeft_Child];
            } else {
                n = n->fChildren[kRight_Child];
            }
        }
    }
    return Iter(leftMost, this);
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::findLast(const T& t) {
    Node* n = fRoot;
    Node* rightMost = NULL;
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                
                rightMost = n;
            }
            n = n->fChildren[kRight_Child];
        }
    }
    return Iter(rightMost, this);
}

template <typename T, typename C>
int GrRedBlackTree<T,C>::countOf(const T& t) const {
    return onCountOf(fRoot, t);
}

template <typename T, typename C>
int GrRedBlackTree<T,C>::onCountOf(const Node* n, const T& t) const {
    
    while (NULL != n) {
        if (fComp(t, n->fItem)) {
            n = n->fChildren[kLeft_Child];
        } else {
            if (!fComp(n->fItem, t)) {
                int count = 1;
                count += onCountOf(n->fChildren[kLeft_Child], t);
                count += onCountOf(n->fChildren[kRight_Child], t);
                return count;
            }
            n = n->fChildren[kRight_Child];
        }
    }
    return 0;

}

template <typename T, typename C>
void GrRedBlackTree<T,C>::reset() {
    RecursiveDelete(fRoot);
    fRoot = NULL;
    fFirst = NULL;
    fLast = NULL;
    fCount = 0;
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Iter GrRedBlackTree<T,C>::insert(const T& t) {
    validate();

    ++fCount;

    Node* x = new Node;
    x->fChildren[kLeft_Child] = NULL;
    x->fChildren[kRight_Child] = NULL;
    x->fItem = t;

    Node* returnNode = x;

    Node* gp = NULL;
    Node* p = NULL;
    Node* n = fRoot;
    Child pc = kLeft_Child; 
    Child gpc = kLeft_Child;

    bool first = true;
    bool last = true;
    while (NULL != n) {
        gpc = pc;
        pc = fComp(x->fItem, n->fItem) ? kLeft_Child : kRight_Child;
        first = first && kLeft_Child == pc;
        last = last && kRight_Child == pc;
        gp = p;
        p = n;
        n = p->fChildren[pc];
    }
    if (last) {
        fLast = x;
    }
    if (first) {
        fFirst = x;
    }

    if (NULL == p) {
        fRoot = x;
        x->fColor = kBlack_Color;
        x->fParent = NULL;
        GrAssert(1 == fCount);
        return Iter(returnNode, this);
    }
    p->fChildren[pc] = x;
    x->fColor = kRed_Color;
    x->fParent = p;

    do {
        
        GrAssert(NULL != x);
        GrAssert(kRed_Color == x->fColor);
        
        GrAssert(!(NULL != gp && NULL == p));
        
        GrAssert(NULL == p  || p->fChildren[pc] == x);
        GrAssert(NULL == gp || gp->fChildren[gpc] == p);

        
        
        if (kBlack_Color == p->fColor) {
            return Iter(returnNode, this);
        }
        
        GrAssert(NULL != gp);
        
        GrAssert(kBlack_Color == gp->fColor);


        
        Node* u = gp->fChildren[1-gpc];
        
        
        
        if (NULL != u && kRed_Color == u->fColor) {
            p->fColor = kBlack_Color;
            u->fColor = kBlack_Color;
            gp->fColor = kRed_Color;
            x = gp;
            p = x->fParent;
            if (NULL == p) {
                
                GrAssert(fRoot == x);
                x->fColor = kBlack_Color;
                validate();
                return Iter(returnNode, this);
            }
            gp = p->fParent;
            pc = (p->fChildren[kLeft_Child] == x) ? kLeft_Child :
                                                    kRight_Child;
            if (NULL != gp) {
                gpc = (gp->fChildren[kLeft_Child] == p) ? kLeft_Child :
                                                          kRight_Child;
            }
            continue;
        } break;
    } while (true);
    
    
    GrAssert(NULL == gp->fChildren[1-gpc] || kBlack_Color == gp->fChildren[1-gpc]->fColor);
    GrAssert(kRed_Color == x->fColor);
    GrAssert(kRed_Color == p->fColor);
    GrAssert(kBlack_Color == gp->fColor);

    
    
    if (pc != gpc) {
        if (kRight_Child == pc) {
            rotateLeft(p);
            Node* temp = p;
            p = x;
            x = temp;
            pc = kLeft_Child;
        } else {
            rotateRight(p);
            Node* temp = p;
            p = x;
            x = temp;
            pc = kRight_Child;
        }
    }
    
    
    
    
    GrAssert(NULL == p->fChildren[1-pc] ||
             kBlack_Color == p->fChildren[1-pc]->fColor);
    
    
    
    p->fColor = kBlack_Color;
    gp->fColor = kRed_Color;
    if (kLeft_Child == pc) {
        rotateRight(gp);
    } else {
        rotateLeft(gp);
    }
    validate();
    return Iter(returnNode, this);
}


template <typename T, typename C>
void GrRedBlackTree<T,C>::rotateRight(Node* n) {
    







    Node* d = n->fParent;
    Node* s = n->fChildren[kLeft_Child];
    GrAssert(NULL != s);
    Node* b = s->fChildren[kRight_Child];

    if (NULL != d) {
        Child c = d->fChildren[kLeft_Child] == n ? kLeft_Child :
                                             kRight_Child;
        d->fChildren[c] = s;
    } else {
        GrAssert(fRoot == n);
        fRoot = s;
    }
    s->fParent = d;
    s->fChildren[kRight_Child] = n;
    n->fParent = s;
    n->fChildren[kLeft_Child] = b;
    if (NULL != b) {
        b->fParent = n;
    }

    GR_DEBUGASSERT(validateChildRelations(d, true));
    GR_DEBUGASSERT(validateChildRelations(s, true));
    GR_DEBUGASSERT(validateChildRelations(n, false));
    GR_DEBUGASSERT(validateChildRelations(n->fChildren[kRight_Child], true));
    GR_DEBUGASSERT(validateChildRelations(b, true));
    GR_DEBUGASSERT(validateChildRelations(s->fChildren[kLeft_Child], true));
}

template <typename T, typename C>
void GrRedBlackTree<T,C>::rotateLeft(Node* n) {

    Node* d = n->fParent;
    Node* s = n->fChildren[kRight_Child];
    GrAssert(NULL != s);
    Node* b = s->fChildren[kLeft_Child];

    if (NULL != d) {
        Child c = d->fChildren[kRight_Child] == n ? kRight_Child :
                                                   kLeft_Child;
        d->fChildren[c] = s;
    } else {
        GrAssert(fRoot == n);
        fRoot = s;
    }
    s->fParent = d;
    s->fChildren[kLeft_Child] = n;
    n->fParent = s;
    n->fChildren[kRight_Child] = b;
    if (NULL != b) {
        b->fParent = n;
    }

    GR_DEBUGASSERT(validateChildRelations(d, true));
    GR_DEBUGASSERT(validateChildRelations(s, true));
    GR_DEBUGASSERT(validateChildRelations(n, true));
    GR_DEBUGASSERT(validateChildRelations(n->fChildren[kLeft_Child], true));
    GR_DEBUGASSERT(validateChildRelations(b, true));
    GR_DEBUGASSERT(validateChildRelations(s->fChildren[kRight_Child], true));
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Node* GrRedBlackTree<T,C>::SuccessorNode(Node* x) {
    GrAssert(NULL != x);
    if (NULL != x->fChildren[kRight_Child]) {
        x = x->fChildren[kRight_Child];
        while (NULL != x->fChildren[kLeft_Child]) {
            x = x->fChildren[kLeft_Child];
        }
        return x;
    }
    while (NULL != x->fParent && x == x->fParent->fChildren[kRight_Child]) {
        x = x->fParent;
    }
    return x->fParent;
}

template <typename T, typename C>
typename GrRedBlackTree<T,C>::Node* GrRedBlackTree<T,C>::PredecessorNode(Node* x) {
    GrAssert(NULL != x);
    if (NULL != x->fChildren[kLeft_Child]) {
        x = x->fChildren[kLeft_Child];
        while (NULL != x->fChildren[kRight_Child]) {
            x = x->fChildren[kRight_Child];
        }
        return x;
    }
    while (NULL != x->fParent && x == x->fParent->fChildren[kLeft_Child]) {
        x = x->fParent;
    }
    return x->fParent;
}

template <typename T, typename C>
void GrRedBlackTree<T,C>::deleteAtNode(Node* x) {
    GrAssert(NULL != x);
    validate();
    --fCount;

    bool hasLeft =  NULL != x->fChildren[kLeft_Child];
    bool hasRight = NULL != x->fChildren[kRight_Child];
    Child c = hasLeft ? kLeft_Child : kRight_Child;

    if (hasLeft && hasRight) {
        
        GrAssert(fFirst != x);
        GrAssert(fLast != x);
        
        
        Node* s = x->fChildren[kRight_Child];
        while (NULL != s->fChildren[kLeft_Child]) {
            s = s->fChildren[kLeft_Child];
        }
        GrAssert(NULL != s);
        
        
        x->fItem = s->fItem;
        x = s;
        c = kRight_Child;
    } else if (NULL == x->fParent) {
        
        
        GrAssert(fRoot == x);
        fRoot = x->fChildren[c];
        if (NULL != fRoot) {
            fRoot->fParent = NULL;
            fRoot->fColor = kBlack_Color;
            if (x == fLast) {
                GrAssert(c == kLeft_Child);
                fLast = fRoot;
            } else if (x == fFirst) {
                GrAssert(c == kRight_Child);
                fFirst = fRoot;
            }
        } else {
            GrAssert(fFirst == fLast && x == fFirst);
            fFirst = NULL;
            fLast = NULL;
            GrAssert(0 == fCount);
        }
        delete x;
        validate();
        return;
    }

    Child pc;
    Node* p = x->fParent;
    pc = p->fChildren[kLeft_Child] == x ? kLeft_Child : kRight_Child;

    if (NULL == x->fChildren[c]) {
        if (fLast == x) {
            fLast = p;
            GrAssert(p == PredecessorNode(x));
        } else if (fFirst == x) {
            fFirst = p;
            GrAssert(p == SuccessorNode(x));
        }
        
        Color xcolor = x->fColor;
        p->fChildren[pc] = NULL;
        delete x;
        x = NULL;
        
        
        if (kRed_Color == xcolor) {
            validate();
            return;
        }
        
        Node* s = p->fChildren[1-pc];

        
        
        
        GrAssert(NULL != s);
        GrAssert(p == s->fParent);

        
        Node* sl;
        Node* sr;
        bool slRed;
        bool srRed;

        do {
            
            
            
            
            
            
            
            GrAssert(NULL != s);
            GrAssert(p == s->fParent);
            GrAssert(NULL == x || x->fParent == p);

            
            sl = s->fChildren[kLeft_Child];
            sr = s->fChildren[kRight_Child];

            
            
            if (kRed_Color == s->fColor) {
                
                GrAssert(kBlack_Color == p->fColor);
                
                
                GrAssert(NULL != sl && kBlack_Color == sl->fColor);
                GrAssert(NULL != sr && kBlack_Color == sr->fColor);
                p->fColor = kRed_Color;
                s->fColor = kBlack_Color;
                if (kLeft_Child == pc) {
                    rotateLeft(p);
                    s = sl;
                } else {
                    rotateRight(p);
                    s = sr;
                }
                sl = s->fChildren[kLeft_Child];
                sr = s->fChildren[kRight_Child];
            }
            
            GrAssert(kBlack_Color == s->fColor);
            GrAssert(NULL == x || kBlack_Color == x->fColor);
            GrAssert(p == s->fParent);
            GrAssert(NULL == x || p == x->fParent);

            
            slRed = (NULL != sl && kRed_Color == sl->fColor);
            srRed = (NULL != sr && kRed_Color == sr->fColor);
            if (!slRed && !srRed) {
                
                
                if (kBlack_Color == p->fColor) {
                    s->fColor = kRed_Color;
                    
                    
                    
                    
                    
                    
                    
                    x = p;
                    p = x->fParent;
                    if (NULL == p) {
                        GrAssert(fRoot == x);
                        validate();
                        return;
                    } else {
                        pc = p->fChildren[kLeft_Child] == x ? kLeft_Child :
                                                              kRight_Child;

                    }
                    s = p->fChildren[1-pc];
                    GrAssert(NULL != s);
                    GrAssert(p == s->fParent);
                    continue;
                } else if (kRed_Color == p->fColor) {
                    
                    
                    
                    s->fColor = kRed_Color;
                    p->fColor = kBlack_Color;
                    validate();
                    return;
                }
            }
            break;
        } while (true);
        
        
        
        GrAssert(slRed || srRed);
        if (kLeft_Child == pc && !srRed) {
            s->fColor = kRed_Color;
            sl->fColor = kBlack_Color;
            rotateRight(s);
            sr = s;
            s = sl;
            
        } else if (kRight_Child == pc && !slRed) {
            s->fColor = kRed_Color;
            sr->fColor = kBlack_Color;
            rotateLeft(s);
            sl = s;
            s = sr;
            
        }
        
        
        
        
        
        
        
        
        s->fColor = p->fColor;
        p->fColor = kBlack_Color;
        if (kLeft_Child == pc) {
            GrAssert(NULL != sr && kRed_Color == sr->fColor);
            sr->fColor = kBlack_Color;
            rotateLeft(p);
        } else {
            GrAssert(NULL != sl && kRed_Color == sl->fColor);
            sl->fColor = kBlack_Color;
            rotateRight(p);
        }
    }
    else {
        
        
        
        
        
        GrAssert(kBlack_Color == x->fColor);
        
        
        
        GrAssert(kRed_Color == x->fChildren[c]->fColor);
        
        
        Node* c1 = x->fChildren[c];
        if (x == fFirst) {
            GrAssert(c == kRight_Child);
            fFirst = c1;
            while (NULL != fFirst->fChildren[kLeft_Child]) {
                fFirst = fFirst->fChildren[kLeft_Child];
            }
            GrAssert(fFirst == SuccessorNode(x));
        } else if (x == fLast) {
            GrAssert(c == kLeft_Child);
            fLast = c1;
            while (NULL != fLast->fChildren[kRight_Child]) {
                fLast = fLast->fChildren[kRight_Child];
            }
            GrAssert(fLast == PredecessorNode(x));
        }
        c1->fParent = p;
        p->fChildren[pc] = c1;
        c1->fColor = kBlack_Color;
        delete x;
        validate();
    }
    validate();
}

template <typename T, typename C>
void GrRedBlackTree<T,C>::RecursiveDelete(Node* x) {
    if (NULL != x) {
        RecursiveDelete(x->fChildren[kLeft_Child]);
        RecursiveDelete(x->fChildren[kRight_Child]);
        delete x;
    }
}

#if GR_DEBUG
template <typename T, typename C>
void GrRedBlackTree<T,C>::validate() const {
    if (fCount) {
        GrAssert(NULL == fRoot->fParent);
        GrAssert(NULL != fFirst);
        GrAssert(NULL != fLast);

        GrAssert(kBlack_Color == fRoot->fColor);
        if (1 == fCount) {
            GrAssert(fFirst == fRoot);
            GrAssert(fLast == fRoot);
            GrAssert(0 == fRoot->fChildren[kLeft_Child]);
            GrAssert(0 == fRoot->fChildren[kRight_Child]);
        }
    } else {
        GrAssert(NULL == fRoot);
        GrAssert(NULL == fFirst);
        GrAssert(NULL == fLast);
    }
#if DEEP_VALIDATE
    int bh;
    int count = checkNode(fRoot, &bh);
    GrAssert(count == fCount);
#endif
}

template <typename T, typename C>
int GrRedBlackTree<T,C>::checkNode(Node* n, int* bh) const {
    if (NULL != n) {
        GrAssert(validateChildRelations(n, false));
        if (kBlack_Color == n->fColor) {
            *bh += 1;
        }
        GrAssert(!fComp(n->fItem, fFirst->fItem));
        GrAssert(!fComp(fLast->fItem, n->fItem));
        int leftBh = *bh;
        int rightBh = *bh;
        int cl = checkNode(n->fChildren[kLeft_Child], &leftBh);
        int cr = checkNode(n->fChildren[kRight_Child], &rightBh);
        GrAssert(leftBh == rightBh);
        *bh = leftBh;
        return 1 + cl + cr;
    }
    return 0;
}

template <typename T, typename C>
bool GrRedBlackTree<T,C>::validateChildRelations(const Node* n,
                                                 bool allowRedRed) const {
    if (NULL != n) {
        if (NULL != n->fChildren[kLeft_Child] ||
            NULL != n->fChildren[kRight_Child]) {
            if (n->fChildren[kLeft_Child] == n->fChildren[kRight_Child]) {
                return validateChildRelationsFailed();
            }
            if (n->fChildren[kLeft_Child] == n->fParent &&
                NULL != n->fParent) {
                return validateChildRelationsFailed();
            }
            if (n->fChildren[kRight_Child] == n->fParent &&
                NULL != n->fParent) {
                return validateChildRelationsFailed();
            }
            if (NULL != n->fChildren[kLeft_Child]) {
                if (!allowRedRed &&
                    kRed_Color == n->fChildren[kLeft_Child]->fColor &&
                    kRed_Color == n->fColor) {
                    return validateChildRelationsFailed();
                }
                if (n->fChildren[kLeft_Child]->fParent != n) {
                    return validateChildRelationsFailed();
                }
                if (!(fComp(n->fChildren[kLeft_Child]->fItem, n->fItem) ||
                      (!fComp(n->fChildren[kLeft_Child]->fItem, n->fItem) &&
                       !fComp(n->fItem, n->fChildren[kLeft_Child]->fItem)))) {
                    return validateChildRelationsFailed();
                }
            }
            if (NULL != n->fChildren[kRight_Child]) {
                if (!allowRedRed &&
                    kRed_Color == n->fChildren[kRight_Child]->fColor &&
                    kRed_Color == n->fColor) {
                    return validateChildRelationsFailed();
                }
                if (n->fChildren[kRight_Child]->fParent != n) {
                    return validateChildRelationsFailed();
                }
                if (!(fComp(n->fItem, n->fChildren[kRight_Child]->fItem) ||
                      (!fComp(n->fChildren[kRight_Child]->fItem, n->fItem) &&
                       !fComp(n->fItem, n->fChildren[kRight_Child]->fItem)))) {
                    return validateChildRelationsFailed();
                }
            }
        }
    }
    return true;
}
#endif

#include "GrRandom.h"

template <typename T, typename C>
void GrRedBlackTree<T,C>::UnitTest() {
    GrRedBlackTree<int> tree;
    typedef GrRedBlackTree<int>::Iter iter;

    GrRandom r;

    int count[100] = {0};
    
    for (int i = 0; i < 10000; ++i) {
        int x = r.nextU()%100;
        Iter xi = tree.insert(x);
        GrAssert(*xi == x);
        ++count[x];
    }

    tree.insert(0);
    ++count[0];
    tree.insert(99);
    ++count[99];
    GrAssert(*tree.begin() == 0);
    GrAssert(*tree.last() == 99);
    GrAssert(--(++tree.begin()) == tree.begin());
    GrAssert(--tree.end() == tree.last());
    GrAssert(tree.count() == 10002);

    int c = 0;
    
    
    for (Iter a = tree.begin(); tree.end() != a; ++a) {
        Iter b = a;
        ++b;
        ++c;
        GrAssert(b == tree.end() || *a <= *b);
    }
    GrAssert(c == tree.count());

    
    
    
    for (int i = 0; i < 100; ++i) {
        int c;
        c = tree.countOf(i);
        GrAssert(c == count[i]);
        c = 0;
        Iter iter = tree.findFirst(i);
        while (iter != tree.end() && *iter == i) {
            ++c;
            ++iter;
        }
        GrAssert(count[i] == c);
        c = 0;
        iter = tree.findLast(i);
        if (iter != tree.end()) {
            do {
                if (*iter == i) {
                    ++c;
                } else {
                    break;
                }
                if (iter != tree.begin()) {
                    --iter;
                } else {
                    break;
                }
            } while (true);
        }
        GrAssert(c == count[i]);
    }
    
    
    for (int i = 25; i < 75; ++i) {
        while (0 != tree.countOf(i)) {
            --count[i];
            int x = r.nextU() % 3;
            Iter iter;
            switch (x) {
            case 0:
                iter = tree.findFirst(i);
                break;
            case 1:
                iter = tree.findLast(i);
                break;
            case 2:
            default:
                iter = tree.find(i);
                break;
            }
            tree.remove(iter);
        }
        GrAssert(0 == count[i]);
        GrAssert(tree.findFirst(i) == tree.end());
        GrAssert(tree.findLast(i) == tree.end());
        GrAssert(tree.find(i) == tree.end());
    }
    
    GrAssert(*tree.begin() == 0);
    GrAssert(*(--tree.end()) == 99);
    while (0 != tree.countOf(0)) {
        --count[0];
        tree.remove(tree.find(0));
    }
    GrAssert(0 == count[0]);
    GrAssert(tree.findFirst(0) == tree.end());
    GrAssert(tree.findLast(0) == tree.end());
    GrAssert(tree.find(0) == tree.end());
    GrAssert(0 < *tree.begin());

    
    while (0 != tree.countOf(99)) {
        --count[99];
        tree.remove(tree.find(99));
    }
    GrAssert(0 == count[99]);
    GrAssert(tree.findFirst(99) == tree.end());
    GrAssert(tree.findLast(99) == tree.end());
    GrAssert(tree.find(99) == tree.end());
    GrAssert(99 > *(--tree.end()));
    GrAssert(tree.last() == --tree.end());

    
    
    c = 0;
    for (Iter a = tree.begin(); tree.end() != a; ++a) {
        Iter b = a;
        ++b;
        ++c;
        GrAssert(b == tree.end() || *a <= *b);
    }
    GrAssert(c == tree.count());

    
    
    for (int i = 0; i < 100; ++i) {
        GrAssert(tree.countOf(i) == count[i]);
        int c = 0;
        Iter iter = tree.findFirst(i);
        while (iter != tree.end() && *iter == i) {
            ++c;
            ++iter;
        }
        GrAssert(count[i] == c);
        c = 0;
        iter = tree.findLast(i);
        if (iter != tree.end()) {
            do {
                if (*iter == i) {
                    ++c;
                } else {
                    break;
                }
                if (iter != tree.begin()) {
                    --iter;
                } else {
                    break;
                }
            } while (true);
        }
        GrAssert(count[i] == c);
    }

    
    while (!tree.empty()) {
        tree.remove(tree.begin());
    }

    
    tree.reset();
}

#endif

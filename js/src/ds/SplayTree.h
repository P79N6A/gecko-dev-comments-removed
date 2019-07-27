





#ifndef ds_SplayTree_h
#define ds_SplayTree_h

#include "ds/LifoAlloc.h"

namespace js {










template <class T, class C>
class SplayTree
{
    struct Node {
        T item;
        Node *left, *right, *parent;

        explicit Node(const T &item)
          : item(item), left(nullptr), right(nullptr), parent(nullptr)
        {}
    };

    LifoAlloc *alloc;
    Node *root, *freeList;

#ifdef DEBUG
    bool enableCheckCoherency;
#endif

    SplayTree(const SplayTree &) MOZ_DELETE;
    SplayTree &operator=(const SplayTree &) MOZ_DELETE;

  public:

    explicit SplayTree(LifoAlloc *alloc = nullptr)
      : alloc(alloc), root(nullptr), freeList(nullptr)
#ifdef DEBUG
      , enableCheckCoherency(true)
#endif
    {}

    void setAllocator(LifoAlloc *alloc) {
        this->alloc = alloc;
    }

    void disableCheckCoherency() {
#ifdef DEBUG
        enableCheckCoherency = false;
#endif
    }

    bool empty() const {
        return !root;
    }

    bool contains(const T &v, T *res)
    {
        if (!root)
            return false;
        Node *last = lookup(v);
        splay(last);
        checkCoherency(root, nullptr);
        if (C::compare(v, last->item) == 0) {
            *res = last->item;
            return true;
        }
        return false;
    }

    bool insert(const T &v)
    {
        Node *element = allocateNode(v);
        if (!element)
            return false;

        if (!root) {
            root = element;
            return true;
        }
        Node *last = lookup(v);
        int cmp = C::compare(v, last->item);

        
        JS_ASSERT(cmp);

        Node *&parentPointer = (cmp < 0) ? last->left : last->right;
        JS_ASSERT(!parentPointer);
        parentPointer = element;
        element->parent = last;

        splay(element);
        checkCoherency(root, nullptr);
        return true;
    }

    void remove(const T &v)
    {
        Node *last = lookup(v);
        JS_ASSERT(last && C::compare(v, last->item) == 0);

        splay(last);
        JS_ASSERT(last == root);

        
        
        
        Node *swap, *swapChild;
        if (root->left) {
            swap = root->left;
            while (swap->right)
                swap = swap->right;
            swapChild = swap->left;
        } else if (root->right) {
            swap = root->right;
            while (swap->left)
                swap = swap->left;
            swapChild = swap->right;
        } else {
            freeNode(root);
            root = nullptr;
            return;
        }

        
        
        if (swap == swap->parent->left)
            swap->parent->left = swapChild;
        else
            swap->parent->right = swapChild;
        if (swapChild)
            swapChild->parent = swap->parent;

        root->item = swap->item;
        freeNode(swap);

        checkCoherency(root, nullptr);
    }

    template <class Op>
    void forEach(Op op)
    {
        forEachInner<Op>(op, root);
    }

  private:

    Node *lookup(const T &v)
    {
        JS_ASSERT(root);
        Node *node = root, *parent;
        do {
            parent = node;
            int c = C::compare(v, node->item);
            if (c == 0)
                return node;
            else if (c < 0)
                node = node->left;
            else
                node = node->right;
        } while (node);
        return parent;
    }

    Node *allocateNode(const T &v)
    {
        Node *node = freeList;
        if (node) {
            freeList = node->left;
            new(node) Node(v);
            return node;
        }
        return alloc->new_<Node>(v);
    }

    void freeNode(Node *node)
    {
        node->left = freeList;
        freeList = node;
    }

    void splay(Node *node)
    {
        
        
        
        JS_ASSERT(node);
        while (node != root) {
            Node *parent = node->parent;
            if (parent == root) {
                
                rotate(node);
                JS_ASSERT(node == root);
                return;
            }
            Node *grandparent = parent->parent;
            if ((parent->left == node) == (grandparent->left == parent)) {
                
                rotate(parent);
                rotate(node);
            } else {
                
                rotate(node);
                rotate(node);
            }
        }
    }

    void rotate(Node *node)
    {
        
        
        Node *parent = node->parent;
        if (parent->left == node) {
            
            
            
            parent->left = node->right;
            if (node->right)
                node->right->parent = parent;
            node->right = parent;
        } else {
            JS_ASSERT(parent->right == node);
            
            
            
            parent->right = node->left;
            if (node->left)
                node->left->parent = parent;
            node->left = parent;
        }
        node->parent = parent->parent;
        parent->parent = node;
        if (Node *grandparent = node->parent) {
            if (grandparent->left == parent)
                grandparent->left = node;
            else
                grandparent->right = node;
        } else {
            root = node;
        }
    }

    template <class Op>
    void forEachInner(Op op, Node *node)
    {
        if (!node)
            return;

        forEachInner<Op>(op, node->left);
        op(node->item);
        forEachInner<Op>(op, node->right);
    }

    Node *checkCoherency(Node *node, Node *minimum)
    {
#ifdef DEBUG
        if (!enableCheckCoherency)
            return nullptr;
        if (!node) {
            JS_ASSERT(!root);
            return nullptr;
        }
        JS_ASSERT_IF(!node->parent, node == root);
        JS_ASSERT_IF(minimum, C::compare(minimum->item, node->item) < 0);
        if (node->left) {
            JS_ASSERT(node->left->parent == node);
            Node *leftMaximum = checkCoherency(node->left, minimum);
            JS_ASSERT(C::compare(leftMaximum->item, node->item) < 0);
        }
        if (node->right) {
            JS_ASSERT(node->right->parent == node);
            return checkCoherency(node->right, node);
        }
        return node;
#else
        return nullptr;
#endif
    }
};

}  

#endif 

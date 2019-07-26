











#ifndef mozilla_SplayTree_h_
#define mozilla_SplayTree_h_

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
      : left(nullptr), right(nullptr), parent(nullptr)
    {}

  private:
    T* left;
    T* right;
    T* parent;
};












template<typename T, class Comparator>
class SplayTree
{
    T* root;
    T* freeList;

  public:
    SplayTree()
      : root(nullptr), freeList(nullptr)
    {}

    bool empty() const {
      return !root;
    }

    bool contains(const T& v)
    {
      if (empty())
        return false;

      T* last = lookup(v);
      splay(last);
      checkCoherency(root, nullptr);
      return Comparator::compare(v, *last) == 0;
    }

    bool insert(T* v)
    {
      MOZ_ASSERT(!contains(*v), "Duplicate elements are not allowed.");

      if (!root) {
        root = v;
        return true;
      }
      T* last = lookup(*v);
      int cmp = Comparator::compare(*v, *last);

      T** parentPointer = (cmp < 0) ? &last->left : &last->right;
      MOZ_ASSERT(!*parentPointer);
      *parentPointer = v;
      v->parent = last;

      splay(v);
      checkCoherency(root, nullptr);
      return true;
    }

    T* remove(const T& v)
    {
      T* last = lookup(v);
      MOZ_ASSERT(last, "This tree must contain the element being removed.");
      MOZ_ASSERT(Comparator::compare(v, *last) == 0);

      
      splay(last);
      MOZ_ASSERT(last == root);

      
      
      
      T* swap;
      T* swapChild;
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
        T* result = root;
        root = nullptr;
        return result;
      }

      
      
      if (swap == swap->parent->left)
        swap->parent->left = swapChild;
      else
        swap->parent->right = swapChild;
      if (swapChild)
        swapChild->parent = swap->parent;

      
      root = swap;
      root->parent = nullptr;
      root->left = last->left;
      root->right = last->right;
      if (root->left) {
        root->left->parent = root;
      }
      if (root->right) {
        root->right->parent = root;
      }

      checkCoherency(root, nullptr);
      return last;
    }

    T* removeMin()
    {
      MOZ_ASSERT(root, "No min to remove!");

      T* min = root;
      while (min->left)
        min = min->left;
      return remove(*min);
    }

  private:
    



    T* lookup(const T& v)
    {
      MOZ_ASSERT(!empty());

      T* node = root;
      T* parent;
      do {
        parent = node;
        int c = Comparator::compare(v, *node);
        if (c == 0)
          return node;
        else if (c < 0)
          node = node->left;
        else
          node = node->right;
      } while (node);
      return parent;
    }

    




    void splay(T* node)
    {
      MOZ_ASSERT(node);

      while (node != root) {
        T* parent = node->parent;
        if (parent == root) {
          
          rotate(node);
          MOZ_ASSERT(node == root);
          return;
        }
        T* grandparent = parent->parent;
        if ((parent->left == node) == (grandparent->left == parent)) {
          
          rotate(parent);
          rotate(node);
        } else {
          
          rotate(node);
          rotate(node);
        }
      }
    }

    void rotate(T* node)
    {
      
      
      T* parent = node->parent;
      if (parent->left == node) {
        
        
        
        parent->left = node->right;
        if (node->right)
          node->right->parent = parent;
        node->right = parent;
      } else {
        MOZ_ASSERT(parent->right == node);
        
        
        
        parent->right = node->left;
        if (node->left)
          node->left->parent = parent;
        node->left = parent;
      }
      node->parent = parent->parent;
      parent->parent = node;
      if (T* grandparent = node->parent) {
        if (grandparent->left == parent)
          grandparent->left = node;
        else
          grandparent->right = node;
      } else {
        root = node;
      }
    }

    T* checkCoherency(T* node, T* minimum)
    {
#ifdef DEBUG
      MOZ_ASSERT_IF(root, !root->parent);
      if (!node) {
        MOZ_ASSERT(!root);
        return nullptr;
      }
      MOZ_ASSERT_IF(!node->parent, node == root);
      MOZ_ASSERT_IF(minimum, Comparator::compare(*minimum, *node) < 0);
      if (node->left) {
        MOZ_ASSERT(node->left->parent == node);
        T* leftMaximum = checkCoherency(node->left, minimum);
        MOZ_ASSERT(Comparator::compare(*leftMaximum, *node) < 0);
      }
      if (node->right) {
        MOZ_ASSERT(node->right->parent == node);
        return checkCoherency(node->right, node);
      }
      return node;
#else
      return nullptr;
#endif
    }

    SplayTree(const SplayTree&) MOZ_DELETE;
    void operator=(const SplayTree&) MOZ_DELETE;
};

}  

#endif 
